#pragma once

#include "core/Interfaces.hpp"

namespace zenith::platform {

class StdFileReader final : public core::IFileReader {
public:
    core::Expected<std::string, core::Error> read_prefix(const std::string& path, std::size_t max_bytes) const override;
    core::Expected<void, core::Error> read_chunks(const std::string& path,
                                                  std::size_t chunk_size,
                                                  std::stop_token stop_token,
                                                  const std::function<core::Expected<void, core::Error>(const std::string&)>& on_chunk) const override;
};

} // namespace zenith::platform
