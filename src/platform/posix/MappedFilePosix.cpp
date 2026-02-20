#ifndef _WIN32

#include "platform/MappedFileProvider.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace zenith::platform {
namespace {

class PosixMappedFile final : public core::IMappedFile {
public:
    PosixMappedFile(std::string p, int fd, std::byte* data, std::size_t size)
        : path_(std::move(p)), fd_(fd), data_(data), size_(size) {}

    ~PosixMappedFile() override {
        if (data_ != nullptr && size_ > 0) {
            munmap(data_, size_);
        }
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    std::span<const std::byte> bytes() const override { return {data_, size_}; }
    std::uint64_t size() const override { return size_; }
    const std::string& path() const override { return path_; }

private:
    std::string path_;
    int fd_;
    std::byte* data_;
    std::size_t size_;
};

} // namespace

core::Expected<std::unique_ptr<core::IMappedFile>, core::Error> MappedFileProvider::open(const std::string& path) const {
    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return core::Error{"open failed"};
    }

    struct stat st {};
    if (fstat(fd, &st) != 0) {
        close(fd);
        return core::Error{"fstat failed"};
    }

    const auto size = static_cast<std::size_t>(st.st_size);
    if (size == 0) {
        return std::unique_ptr<core::IMappedFile>(new PosixMappedFile(path, fd, nullptr, 0));
    }

    void* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return core::Error{"mmap failed"};
    }

    return std::unique_ptr<core::IMappedFile>(new PosixMappedFile(path, fd, static_cast<std::byte*>(mapped), size));
}

} // namespace zenith::platform

#endif
