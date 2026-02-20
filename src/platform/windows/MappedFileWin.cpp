#ifdef _WIN32

#include "platform/MappedFileProvider.hpp"

#define NOMINMAX
#include <windows.h>

namespace zenith::platform {
namespace {

std::wstring to_wide(const std::string& s) {
    const int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(size > 0 ? static_cast<std::size_t>(size - 1) : 0, L'\0');
    if (size > 1) {
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
    }
    return out;
}

class WinMappedFile final : public core::IMappedFile {
public:
    WinMappedFile(std::string p, HANDLE file, HANDLE mapping, std::byte* data, std::size_t size)
        : path_(std::move(p)), file_(file), mapping_(mapping), data_(data), size_(size) {}

    ~WinMappedFile() override {
        if (data_ != nullptr) {
            UnmapViewOfFile(data_);
        }
        if (mapping_ != nullptr) {
            CloseHandle(mapping_);
        }
        if (file_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_);
        }
    }

    std::span<const std::byte> bytes() const override { return {data_, size_}; }
    std::uint64_t size() const override { return size_; }
    const std::string& path() const override { return path_; }

private:
    std::string path_;
    HANDLE file_;
    HANDLE mapping_;
    std::byte* data_;
    std::size_t size_;
};

} // namespace

core::Expected<std::unique_ptr<core::IMappedFile>, core::Error> MappedFileProvider::open(const std::string& path) const {
    const auto wide = to_wide(path);
    HANDLE file = CreateFileW(wide.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return core::Error{"CreateFileW failed"};
    }

    LARGE_INTEGER size_li{};
    if (!GetFileSizeEx(file, &size_li)) {
        CloseHandle(file);
        return core::Error{"GetFileSizeEx failed"};
    }

    const auto size = static_cast<std::size_t>(size_li.QuadPart);
    if (size == 0) {
        return std::unique_ptr<core::IMappedFile>(new WinMappedFile(path, file, nullptr, nullptr, 0));
    }

    HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (mapping == nullptr) {
        CloseHandle(file);
        return core::Error{"CreateFileMappingW failed"};
    }

    void* view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (view == nullptr) {
        CloseHandle(mapping);
        CloseHandle(file);
        return core::Error{"MapViewOfFile failed"};
    }

    return std::unique_ptr<core::IMappedFile>(new WinMappedFile(path, file, mapping, static_cast<std::byte*>(view), size));
}

} // namespace zenith::platform

#endif
