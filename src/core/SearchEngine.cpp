#include "SearchEngine.hpp"

#include "TextUtils.hpp"

#include <algorithm>

namespace zenith::core {

bool SearchEngine::is_binary_file(const std::string& path) const {
    auto prefix = reader_.read_prefix(path, 4096);
    if (!prefix) {
        errors_.write_error({path + ": " + prefix.error().message});
        return true;
    }
    return std::find(prefix.value().begin(), prefix.value().end(), '\0') != prefix.value().end();
}

SearchStats SearchEngine::run(const SearchRequest& request) const {
    SearchStats stats{};
    const auto files = enumerator_.enumerate(request.input_paths,
                                             request.ignore_hidden,
                                             request.extensions,
                                             request.max_bytes,
                                             [this](const Error& err) { errors_.write_error(err); });

    for (const auto& file : files) {
        if (request.binary_mode == BinaryMode::Skip && is_binary_file(file.path)) {
            continue;
        }

        std::size_t match_count = 0;
        bool file_has_match = false;
        std::string carry;
        std::uintmax_t processed = 0;
        const auto read_result = reader_.read_chunks(
            file.path,
            request.chunk_size,
            [&](const std::string& chunk) -> Expected<void, Error> {
                std::string combined = carry + chunk;
                const std::size_t carry_size = carry.size();
                const auto positions = algorithm_.find_all(combined, request.pattern);

                for (std::size_t pos : positions) {
                    if (pos + request.pattern.size() <= carry_size) {
                        continue;
                    }
                    const auto global_offset = processed - carry_size + pos;
                    ++match_count;
                    file_has_match = true;
                    stats.any_match = true;

                    if (request.output_mode == OutputMode::Matches) {
                        const std::size_t snippet_start = (pos > 60U) ? pos - 60U : 0U;
                        const std::size_t snippet_end = std::min(combined.size(), pos + request.pattern.size() + 60U);
                        const auto snippet = sanitize_snippet(combined.substr(snippet_start, snippet_end - snippet_start));
                        output_.write_match({file.path, global_offset, snippet});
                    }
                }

                processed += chunk.size();
                if (request.pattern.size() > 1U) {
                    const std::size_t overlap = request.pattern.size() - 1U;
                    if (combined.size() > overlap) {
                        carry = combined.substr(combined.size() - overlap);
                    } else {
                        carry = combined;
                    }
                } else {
                    carry.clear();
                }

                return {};
            });

        if (!read_result) {
            errors_.write_error({file.path + ": " + read_result.error().message});
            continue;
        }

        if (file_has_match && request.output_mode != OutputMode::Matches) {
            output_.write_file_summary({file.path, match_count});
        }
    }

    return stats;
}

} // namespace zenith::core
