#include "SearchEngine.hpp"

#include "TextUtils.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <thread>

namespace zenith::core {
namespace {

bool is_binary_prefix(std::string_view prefix) {
    return std::find(prefix.begin(), prefix.end(), '\0') != prefix.end();
}

bool is_binary_prefix(std::span<const std::byte> prefix) {
    for (auto b : prefix) {
        if (b == std::byte{0}) {
            return true;
        }
    }
    return false;
}

std::size_t effective_threads(std::size_t configured) {
    if (configured != 0) {
        return std::clamp<std::size_t>(configured, 1, 32);
    }
    const auto hc = std::thread::hardware_concurrency();
    const std::size_t base = hc == 0 ? 4 : static_cast<std::size_t>(hc);
    return std::clamp<std::size_t>(base, 1, 32);
}

std::string make_snippet(std::string_view all, std::size_t pos, std::size_t pat_len) {
    const std::size_t start = (pos > 60U) ? pos - 60U : 0U;
    const std::size_t end = std::min(all.size(), pos + pat_len + 60U);
    return sanitize_snippet(std::string(all.substr(start, end - start)));
}

} // namespace

const ISearchAlgorithm& SearchEngine::choose_algorithm(AlgorithmMode mode, std::size_t pattern_len, std::uintmax_t file_size) const {
    if (mode == AlgorithmMode::Naive) {
        return naive_algorithm_;
    }
    if (mode == AlgorithmMode::Bmh) {
        return bmh_algorithm_;
    }
    if (mode == AlgorithmMode::BoyerMoore) {
        return boyer_moore_algorithm_;
    }

    if (pattern_len < 4) {
        return naive_algorithm_;
    }
    if (pattern_len >= 8) {
        return boyer_moore_algorithm_;
    }
    if (file_size >= 64U * 1024U) {
        return bmh_algorithm_;
    }
    return naive_algorithm_;
}

SearchStats SearchEngine::run(const SearchRequest& request) const {
    SearchStats stats{};
    auto files = enumerator_.enumerate(request.input_paths,
                                       request.ignore_hidden,
                                       request.extensions,
                                       request.max_bytes,
                                       [this](const Error& err) { errors_.write_error(err); });

    std::sort(files.begin(), files.end(), [](const FileItem& a, const FileItem& b) { return a.path < b.path; });

    std::vector<FileResult> results(files.size());
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::deque<std::size_t> jobs;
    for (std::size_t i = 0; i < files.size(); ++i) {
        jobs.push_back(i);
    }

    std::mutex emit_mutex;
    std::atomic<bool> any_match{false};

    auto scan_file = [&](const FileItem& file) -> FileResult {
        FileResult file_result;
        file_result.path = file.path;
        const auto& algorithm = choose_algorithm(request.algorithm_mode, request.pattern.size(), file.size);

        const bool use_mmap = (request.mmap_mode == MmapMode::On) ||
                              (request.mmap_mode == MmapMode::Auto && file.size >= request.mmap_threshold_bytes);

        auto emit_match = [&](std::uintmax_t offset, std::string snippet) {
            file_result.any_match = true;
            ++file_result.count;
            if (request.output_mode == OutputMode::Matches) {
                file_result.matches.push_back({file.path, offset, std::move(snippet)});
            }
        };

        if (use_mmap) {
            auto mapped = mapped_provider_.open(file.path);
            if (mapped) {
                auto bytes = mapped.value()->bytes();
                if (request.binary_mode == BinaryMode::Skip) {
                    const auto probe = bytes.subspan(0, std::min<std::size_t>(bytes.size(), 4096));
                    if (is_binary_prefix(probe)) {
                        return file_result;
                    }
                }
                std::string_view haystack(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                const auto positions = algorithm.find_all(haystack, request.pattern);
                for (const auto pos : positions) {
                    emit_match(pos, make_snippet(haystack, pos, request.pattern.size()));
                }
                return file_result;
            }

            if (request.mmap_mode == MmapMode::On) {
                errors_.write_error({file.path + ": mmap failed, falling back to streaming: " + mapped.error().message});
            }
        }

        if (request.binary_mode == BinaryMode::Skip) {
            auto prefix = reader_.read_prefix(file.path, 4096);
            if (!prefix) {
                errors_.write_error({file.path + ": " + prefix.error().message});
                return file_result;
            }
            if (is_binary_prefix(prefix.value())) {
                return file_result;
            }
        }

        std::string carry;
        std::uintmax_t processed = 0;
        const auto read_result = reader_.read_chunks(
            file.path,
            request.chunk_size,
            [&](const std::string& chunk) -> Expected<void, Error> {
                const std::string combined = carry + chunk;
                const std::size_t carry_size = carry.size();
                const auto positions = algorithm.find_all(combined, request.pattern);
                for (const auto pos : positions) {
                    if (pos + request.pattern.size() <= carry_size) {
                        continue;
                    }
                    const auto global_offset = processed - carry_size + pos;
                    emit_match(global_offset, make_snippet(combined, pos, request.pattern.size()));
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

        if (!read_result) {
            errors_.write_error({file.path + ": " + read_result.error().message});
        }
        return file_result;
    };

    auto emit_file_result = [&](const FileResult& fr) {
        if (!fr.any_match) {
            return;
        }
        if (request.output_mode == OutputMode::Matches) {
            for (const auto& m : fr.matches) {
                output_.write_match(m);
            }
        } else {
            output_.write_file_summary({fr.path, fr.count});
        }
    };

    const auto worker_count = std::min<std::size_t>(effective_threads(request.threads), files.size() == 0 ? 1 : files.size());
    std::vector<std::jthread> workers;
    workers.reserve(worker_count);

    for (std::size_t w = 0; w < worker_count; ++w) {
        workers.emplace_back([&](std::stop_token) {
            while (true) {
                std::size_t job = 0;
                {
                    std::unique_lock lock(queue_mutex);
                    if (jobs.empty()) {
                        return;
                    }
                    job = jobs.front();
                    jobs.pop_front();
                }
                auto result = scan_file(files[job]);
                if (result.any_match) {
                    any_match = true;
                    std::sort(result.matches.begin(), result.matches.end(), [](const MatchRecord& a, const MatchRecord& b) {
                        return a.offset < b.offset;
                    });
                }

                if (request.stable_output == StableOutputMode::On) {
                    results[job] = std::move(result);
                } else {
                    std::scoped_lock lock(emit_mutex);
                    emit_file_result(result);
                }
            }
        });
    }

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (request.stable_output == StableOutputMode::On) {
        for (const auto& fr : results) {
            emit_file_result(fr);
        }
    }

    stats.any_match = any_match.load();
    return stats;
}

} // namespace zenith::core
