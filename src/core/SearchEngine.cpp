#include "SearchEngine.hpp"

#include "TextUtils.hpp"

#include <algorithm>
#include <atomic>
#include <deque>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace zenith::core {
namespace {

bool is_binary_prefix(std::string_view prefix) { return std::find(prefix.begin(), prefix.end(), '\0') != prefix.end(); }

bool is_binary_prefix(std::span<const std::byte> prefix) {
    for (auto b : prefix) {
        if (b == std::byte{0}) return true;
    }
    return false;
}

std::size_t effective_threads(std::size_t configured) {
    if (configured != 0) return std::clamp<std::size_t>(configured, 1, 32);
    const auto hc = std::thread::hardware_concurrency();
    const std::size_t base = hc == 0 ? 4 : static_cast<std::size_t>(hc);
    return std::clamp<std::size_t>(base, 1, 32);
}

std::string make_snippet(std::string_view all, std::size_t pos, std::size_t pat_len, std::size_t snippet_cap) {
    const std::size_t half = snippet_cap / 2;
    const std::size_t start = (pos > half) ? pos - half : 0U;
    const std::size_t end = std::min(all.size(), pos + pat_len + half);
    return sanitize_snippet(std::string(all.substr(start, end - start)));
}

} // namespace

const ISearchAlgorithm& SearchEngine::choose_algorithm(AlgorithmMode mode, std::size_t pattern_len, std::uintmax_t file_size) const {
    if (mode == AlgorithmMode::Naive) return naive_algorithm_;
    if (mode == AlgorithmMode::Bmh) return bmh_algorithm_;
    if (mode == AlgorithmMode::BoyerMoore) return boyer_moore_algorithm_;

    if (pattern_len < 4) return naive_algorithm_;
    if (pattern_len >= 8) return boyer_moore_algorithm_;
    if (file_size >= 64U * 1024U) return bmh_algorithm_;
    return naive_algorithm_;
}

SearchStats SearchEngine::run(const SearchRequest& request, std::stop_token stop_token) const {
    SearchStats stats{};
    auto files = enumerator_.enumerate(request, stop_token, [this](const Error& err) { errors_.write_error(err); });
    std::sort(files.begin(), files.end(), [](const FileItem& a, const FileItem& b) { return a.normalized_path < b.normalized_path; });

    std::vector<FileResult> results(files.size());
    std::mutex queue_mutex;
    std::deque<std::size_t> jobs;
    for (std::size_t i = 0; i < files.size(); ++i) jobs.push_back(i);

    std::mutex emit_mutex;
    std::atomic<bool> any_match{false};
    std::atomic<bool> cancelled{false};
#ifdef ZENITHSEARCH_ENABLE_TEST_HOOKS
    std::atomic<std::size_t> completed_files{0};
    std::atomic<bool> injected_cancel{false};
    std::size_t cancel_after_files = 0;
    if (const char* env = std::getenv("ZENITHSEARCH_TEST_CANCEL_AFTER_FILES"); env != nullptr) {
        cancel_after_files = static_cast<std::size_t>(std::strtoull(env, nullptr, 10));
    }
#endif

    auto scan_file = [&](const FileItem& file) -> FileResult {
        FileResult fr;
        fr.path = file.path;
        if (stop_token.stop_requested()) {
            fr.completed = false;
            return fr;
        }

        const auto& algorithm = choose_algorithm(request.algorithm_mode, request.pattern.size(), file.size);
        const bool use_mmap = request.mmap_mode == MmapMode::On ||
                              (request.mmap_mode == MmapMode::Auto && file.size >= request.mmap_threshold_bytes);

        auto add_match = [&](std::uintmax_t offset, std::string snippet) {
            fr.any_match = true;
            ++fr.count;
            if (request.output_mode == OutputMode::Count) return;
            if (request.max_matches_per_file.has_value() && fr.matches.size() >= *request.max_matches_per_file) return;
            fr.matches.push_back({file.path, offset, std::move(snippet), fr.binary});
        };

        if (use_mmap) {
            auto mapped = mapped_provider_.open(file.path);
            if (mapped) {
                auto bytes = mapped.value()->bytes();
                fr.binary = is_binary_prefix(bytes.subspan(0, std::min<std::size_t>(bytes.size(), 4096)));
                if (fr.binary && request.binary_mode == BinaryMode::Skip) return fr;
                std::string_view hay(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                auto pos = algorithm.find_all(hay, request.pattern);
                for (auto p : pos) {
                    if (stop_token.stop_requested()) {
                        fr.completed = false;
                        return fr;
                    }
                    add_match(p, request.no_snippet ? std::string{} : make_snippet(hay, p, request.pattern.size(), request.max_snippet_bytes));
                }
                return fr;
            }
            if (request.mmap_mode == MmapMode::On) {
                errors_.write_error({file.path + ": mmap failed, fallback to stream: " + mapped.error().message});
            }
        }

        if (request.binary_mode == BinaryMode::Skip) {
            auto prefix = reader_.read_prefix(file.path, 4096);
            if (!prefix) {
                errors_.write_error({file.path + ": " + prefix.error().message});
                return fr;
            }
            fr.binary = is_binary_prefix(prefix.value());
            if (fr.binary) return fr;
        }

        std::string carry;
        std::uintmax_t processed = 0;
        auto rr = reader_.read_chunks(file.path, request.chunk_size, stop_token, [&](const std::string& chunk) -> Expected<void, Error> {
            if (stop_token.stop_requested()) {
                fr.completed = false;
                return {};
            }
            std::string combined = carry + chunk;
            const std::size_t carry_size = carry.size();
            auto positions = algorithm.find_all(combined, request.pattern);
            for (auto pos : positions) {
                if (pos + request.pattern.size() <= carry_size) continue;
                const auto global_offset = processed - carry_size + pos;
                add_match(global_offset,
                          request.no_snippet ? std::string{} : make_snippet(combined, pos, request.pattern.size(), request.max_snippet_bytes));
            }
            processed += chunk.size();
            if (request.pattern.size() > 1U) {
                const std::size_t overlap = request.pattern.size() - 1U;
                carry = combined.size() > overlap ? combined.substr(combined.size() - overlap) : combined;
            } else {
                carry.clear();
            }
            return {};
        });
        if (!rr) {
            errors_.write_error({file.path + ": " + rr.error().message});
        }
        return fr;
    };

    auto emit = [&](const FileResult& fr) {
        if (!fr.any_match) return;
        if (request.output_mode == OutputMode::Matches) {
            for (const auto& m : fr.matches) output_.write_match(m);
        } else if (request.output_mode == OutputMode::Count) {
            output_.write_file_summary({fr.path, fr.count, fr.binary});
        } else {
            output_.write_file_summary({fr.path, 1, fr.binary});
        }
    };

    const auto workers_n = std::min<std::size_t>(effective_threads(request.threads), files.empty() ? 1 : files.size());
    std::vector<std::jthread> workers;
    workers.reserve(workers_n);

    for (std::size_t w = 0; w < workers_n; ++w) {
        workers.emplace_back([&](std::stop_token) {
            while (!stop_token.stop_requested()
#ifdef ZENITHSEARCH_ENABLE_TEST_HOOKS
                   && !injected_cancel.load()
#endif
            ) {
                std::size_t job = 0;
                {
                    std::scoped_lock lock(queue_mutex);
                    if (jobs.empty()) return;
                    job = jobs.front();
                    jobs.pop_front();
                }

                auto fr = scan_file(files[job]);
                if (fr.any_match) {
                    any_match = true;
                    std::sort(fr.matches.begin(), fr.matches.end(), [](const MatchRecord& a, const MatchRecord& b) { return a.offset < b.offset; });
                }
                if (!fr.completed) cancelled = true;
#ifdef ZENITHSEARCH_ENABLE_TEST_HOOKS
                const auto done = ++completed_files;
                if (cancel_after_files > 0 && done >= cancel_after_files) {
                    injected_cancel = true;
                    cancelled = true;
                }
#endif

                if (request.stable_output == StableOutputMode::On) {
                    results[job] = std::move(fr);
                } else {
                    std::scoped_lock lock(emit_mutex);
                    emit(fr);
                }
            }
        });
    }

    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }

    if (request.stable_output == StableOutputMode::On) {
        for (const auto& fr : results) {
            if (cancelled && !fr.completed) continue;
            emit(fr);
        }
    }

    stats.any_match = any_match.load();
    stats.cancelled = cancelled.load() || stop_token.stop_requested()
#ifdef ZENITHSEARCH_ENABLE_TEST_HOOKS
                      || injected_cancel.load()
#endif
    ;
    return stats;
}

} // namespace zenith::core
