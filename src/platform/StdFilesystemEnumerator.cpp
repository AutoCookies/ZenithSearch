#include "StdFilesystemEnumerator.hpp"

#include "Glob.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_map>

namespace zenith::platform {
namespace fs = std::filesystem;

namespace {

bool is_hidden_path(const fs::path& path) {
    const auto filename = path.filename().string();
    return !filename.empty() && filename[0] == '.';
}

std::string normalize_extension(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::string normalize_path(const fs::path& path) { return normalize_for_match(path.lexically_normal().generic_string()); }

bool basename_match(const std::string& name, const std::vector<std::string>& patterns) {
#ifdef _WIN32
    std::string lowered = name;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (auto p : patterns) {
        std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (p == lowered) {
            return true;
        }
    }
    return false;
#else
    return std::find(patterns.begin(), patterns.end(), name) != patterns.end();
#endif
}

std::vector<std::string> load_ignore_patterns(const fs::path& dir) {
    std::vector<std::string> patterns;
    const auto ignore_file = dir / ".zenithignore";
    std::ifstream in(ignore_file);
    if (!in) {
        return patterns;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        patterns.push_back(normalize_for_match((dir / line).lexically_normal().generic_string()));
    }
    return patterns;
}

bool match_any(const std::vector<std::string>& globs, const std::string& normalized) {
    for (const auto& g : globs) {
        if (glob_match(g, normalized)) {
            return true;
        }
    }
    return false;
}

} // namespace

std::vector<core::FileItem> StdFilesystemEnumerator::enumerate(const core::SearchRequest& request,
                                                                std::stop_token stop_token,
                                                                const ErrorCallback& on_error) const {
    std::vector<core::FileItem> results;
    std::set<std::string> visited_dirs;

    for (const auto& raw_path : request.input_paths) {
        if (stop_token.stop_requested()) {
            break;
        }

        const fs::path root(raw_path);
        std::error_code ec;
        const auto status = request.follow_symlinks == core::FollowSymlinksMode::On ? fs::status(root, ec) : fs::symlink_status(root, ec);
        if (ec) {
            on_error({root.string() + ": " + ec.message()});
            continue;
        }

        auto should_include_file = [&](const fs::path& file, std::uintmax_t size) {
            const auto normalized = normalize_path(file);
            if (request.ignore_hidden && is_hidden_path(file)) {
                return false;
            }
            if (!request.exclude_dirs.empty()) {
                const auto base = file.parent_path().filename().string();
                if (basename_match(base, request.exclude_dirs)) {
                    return false;
                }
            }
            if (!request.exclude_globs.empty() && match_any(request.exclude_globs, normalized)) {
                return false;
            }
            if (!request.extensions.empty()) {
                const auto ext = normalize_extension(file);
                if (request.extensions.find(ext) == request.extensions.end()) {
                    return false;
                }
            }
            if (!request.include_globs.empty() && !match_any(request.include_globs, normalized)) {
                return false;
            }
            if (request.max_bytes.has_value() && size > *request.max_bytes) {
                return false;
            }
            return true;
        };

        if (fs::is_regular_file(status)) {
            const auto normalized = normalize_path(root);
            if (!request.exclude_globs.empty() && match_any(request.exclude_globs, normalized)) {
                continue;
            }
            std::error_code sec;
            auto size = fs::file_size(root, sec);
            if (sec) {
                on_error({root.string() + ": " + sec.message()});
                continue;
            }
            if (should_include_file(root, size)) {
                results.push_back({root.string(), normalized, size});
            }
            continue;
        }

        if (!fs::is_directory(status)) {
            on_error({root.string() + ": unsupported path type"});
            continue;
        }

        std::unordered_map<std::string, std::vector<std::string>> ignore_cache;
        fs::directory_options options = fs::directory_options::skip_permission_denied;
        if (request.follow_symlinks == core::FollowSymlinksMode::On) {
            options |= fs::directory_options::follow_directory_symlink;
        }

        fs::recursive_directory_iterator it(root, options, ec);
        fs::recursive_directory_iterator end;
        if (ec) {
            on_error({root.string() + ": " + ec.message()});
            continue;
        }

        while (it != end) {
            if (stop_token.stop_requested()) {
                break;
            }

            const auto entry = *it;
            const auto current = entry.path();
            const auto current_norm = normalize_path(current);

            std::error_code link_ec;
            if (entry.is_directory(link_ec)) {
                if (link_ec) {
                    on_error({current.string() + ": " + link_ec.message()});
                    ++it;
                    continue;
                }

                if (request.ignore_hidden && is_hidden_path(current)) {
                    it.disable_recursion_pending();
                    ++it;
                    continue;
                }
                if (!request.exclude_dirs.empty() && basename_match(current.filename().string(), request.exclude_dirs)) {
                    it.disable_recursion_pending();
                    ++it;
                    continue;
                }
                if (!request.exclude_globs.empty() && match_any(request.exclude_globs, current_norm + "/x")) {
                    it.disable_recursion_pending();
                    ++it;
                    continue;
                }

                if (!request.no_ignore) {
                    auto key = normalize_path(current);
                    auto [found, inserted] = ignore_cache.try_emplace(key);
                    if (inserted) {
                        found->second = load_ignore_patterns(current);
                    }
                    const auto& patterns = found->second;
                    bool ignored_dir = false;
                    for (const auto& p : patterns) {
                        if (glob_match(p, current_norm) || glob_match(p + "/**", current_norm)) {
                            ignored_dir = true;
                            break;
                        }
                    }
                    if (ignored_dir) {
                        it.disable_recursion_pending();
                        ++it;
                        continue;
                    }
                }

                if (request.follow_symlinks == core::FollowSymlinksMode::On) {
                    std::error_code can_ec;
                    const auto canonical = fs::weakly_canonical(current, can_ec);
                    if (!can_ec) {
                        const auto canon_norm = normalize_path(canonical);
                        if (visited_dirs.contains(canon_norm)) {
                            it.disable_recursion_pending();
                            ++it;
                            continue;
                        }
                        visited_dirs.insert(canon_norm);
                    }
                }

                ++it;
                continue;
            }

            std::error_code reg_ec;
            if (!entry.is_regular_file(reg_ec)) {
                if (reg_ec) {
                    on_error({current.string() + ": " + reg_ec.message()});
                }
                ++it;
                continue;
            }

            std::error_code size_ec;
            const auto size = entry.file_size(size_ec);
            if (size_ec) {
                on_error({current.string() + ": " + size_ec.message()});
                ++it;
                continue;
            }

            bool ignored = false;
            if (!request.no_ignore) {
                fs::path walk = current.parent_path();
                while (!walk.empty()) {
                    const auto key = normalize_path(walk);
                    auto [found, inserted] = ignore_cache.try_emplace(key);
                    if (inserted) {
                        found->second = load_ignore_patterns(walk);
                    }
                    for (const auto& p : found->second) {
                        if (glob_match(p, current_norm)) {
                            ignored = true;
                            break;
                        }
                    }
                    if (ignored || walk == root || walk == walk.root_path()) {
                        break;
                    }
                    walk = walk.parent_path();
                }
            }

            if (!ignored && should_include_file(current, size)) {
                results.push_back({current.string(), current_norm, size});
            }
            ++it;
        }
    }

    return results;
}

} // namespace zenith::platform
