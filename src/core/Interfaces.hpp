#pragma once

#include "Expected.hpp"
#include "Types.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace zenith::core {

class ISearchAlgorithm {
public:
    virtual ~ISearchAlgorithm() = default;
    virtual std::vector<std::size_t> find_all(const std::string& buffer, const std::string& pattern) const = 0;
};

class IFileEnumerator {
public:
    using ErrorCallback = std::function<void(const Error&)>;
    virtual ~IFileEnumerator() = default;
    virtual std::vector<FileItem> enumerate(const std::vector<std::string>& paths,
                                            bool ignore_hidden,
                                            const std::unordered_set<std::string>& extensions,
                                            const std::optional<std::uintmax_t>& max_bytes,
                                            const ErrorCallback& on_error) const = 0;
};

class IFileReader {
public:
    virtual ~IFileReader() = default;
    virtual Expected<std::string, Error> read_prefix(const std::string& path, std::size_t max_bytes) const = 0;
    virtual Expected<void, Error> read_chunks(const std::string& path,
                                              std::size_t chunk_size,
                                              const std::function<Expected<void, Error>(const std::string&)>& on_chunk) const = 0;
};

class IOutputWriter {
public:
    virtual ~IOutputWriter() = default;
    virtual void write_match(const MatchRecord& record) = 0;
    virtual void write_file_summary(const FileMatchSummary& summary) = 0;
};

class IErrorWriter {
public:
    virtual ~IErrorWriter() = default;
    virtual void write_error(const Error& error) = 0;
};

} // namespace zenith::core
