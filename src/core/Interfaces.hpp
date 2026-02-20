#pragma once

#include "Expected.hpp"
#include "Types.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace zenith::core {

class ISearchAlgorithm {
public:
    virtual ~ISearchAlgorithm() = default;
    virtual std::vector<std::size_t> find_all(std::string_view buffer, std::string_view pattern) const = 0;
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

class IMappedFile {
public:
    virtual ~IMappedFile() = default;
    virtual std::span<const std::byte> bytes() const = 0;
    virtual std::uint64_t size() const = 0;
    virtual const std::string& path() const = 0;
};

class IMappedFileProvider {
public:
    virtual ~IMappedFileProvider() = default;
    virtual Expected<std::unique_ptr<IMappedFile>, Error> open(const std::string& path) const = 0;
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
