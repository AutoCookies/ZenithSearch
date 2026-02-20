#pragma once

#include "core/Interfaces.hpp"

namespace zenith::platform {

class StdFilesystemEnumerator final : public core::IFileEnumerator {
public:
    std::vector<core::FileItem> enumerate(const std::vector<std::string>& paths,
                                          bool ignore_hidden,
                                          const std::unordered_set<std::string>& extensions,
                                          const std::optional<std::uintmax_t>& max_bytes,
                                          const ErrorCallback& on_error) const override;
};

} // namespace zenith::platform
