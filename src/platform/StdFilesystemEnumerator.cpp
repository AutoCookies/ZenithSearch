#include "StdFilesystemEnumerator.hpp"

#include <algorithm>
#include <filesystem>

namespace zenith::platform {
namespace fs = std::filesystem;

static bool is_hidden_path(const fs::path& path) {
    const auto filename = path.filename().string();
    return !filename.empty() && filename[0] == '.';
}

static std::string normalize_extension(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

std::vector<core::FileItem> StdFilesystemEnumerator::enumerate(const std::vector<std::string>& paths,
                                                                bool ignore_hidden,
                                                                const std::unordered_set<std::string>& extensions,
                                                                const std::optional<std::uintmax_t>& max_bytes,
                                                                const ErrorCallback& on_error) const {
    std::vector<core::FileItem> results;

    auto consider_file = [&](const fs::path& file) {
        if (ignore_hidden && is_hidden_path(file)) {
            return;
        }

        if (!extensions.empty()) {
            const auto ext = normalize_extension(file);
            if (extensions.find(ext) == extensions.end()) {
                return;
            }
        }

        std::error_code ec;
        const auto size = fs::file_size(file, ec);
        if (ec) {
            on_error({file.string() + ": " + ec.message()});
            return;
        }
        if (max_bytes.has_value() && size > *max_bytes) {
            return;
        }
        results.push_back({file.string(), size});
    };

    for (const auto& raw_path : paths) {
        fs::path path(raw_path);
        std::error_code ec;
        const auto status = fs::status(path, ec);
        if (ec) {
            on_error({path.string() + ": " + ec.message()});
            continue;
        }

        if (fs::is_regular_file(status)) {
            consider_file(path);
            continue;
        }

        if (fs::is_directory(status)) {
            fs::recursive_directory_iterator it(path, fs::directory_options::skip_permission_denied, ec);
            fs::recursive_directory_iterator end;
            if (ec) {
                on_error({path.string() + ": " + ec.message()});
                continue;
            }
            while (it != end) {
                const auto entry = *it;
                const auto current = entry.path();
                if (ignore_hidden && is_hidden_path(current)) {
                    if (entry.is_directory()) {
                        it.disable_recursion_pending();
                    }
                    ++it;
                    continue;
                }
                std::error_code entry_ec;
                if (entry.is_regular_file(entry_ec)) {
                    consider_file(current);
                } else if (entry_ec) {
                    on_error({current.string() + ": " + entry_ec.message()});
                }
                ++it;
            }
            continue;
        }

        on_error({path.string() + ": unsupported path type"});
    }

    return results;
}

} // namespace zenith::platform
