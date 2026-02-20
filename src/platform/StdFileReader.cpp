#include "StdFileReader.hpp"

#include <fstream>
#include <vector>

namespace zenith::platform {

core::Expected<std::string, core::Error> StdFileReader::read_prefix(const std::string& path, std::size_t max_bytes) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return core::Error{"unable to open file"};
    }
    std::string buffer(max_bytes, '\0');
    input.read(buffer.data(), static_cast<std::streamsize>(max_bytes));
    buffer.resize(static_cast<std::size_t>(input.gcount()));
    return buffer;
}

core::Expected<void, core::Error> StdFileReader::read_chunks(
    const std::string& path,
    std::size_t chunk_size,
    const std::function<core::Expected<void, core::Error>(const std::string&)>& on_chunk) const {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return core::Error{"unable to open file"};
    }

    std::vector<char> buf(chunk_size);
    while (input) {
        input.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        const auto read_count = input.gcount();
        if (read_count <= 0) {
            break;
        }
        auto result = on_chunk(std::string(buf.data(), static_cast<std::size_t>(read_count)));
        if (!result) {
            return result.error();
        }
    }

    if (input.bad()) {
        return core::Error{"i/o error while reading"};
    }

    return {};
}

} // namespace zenith::platform
