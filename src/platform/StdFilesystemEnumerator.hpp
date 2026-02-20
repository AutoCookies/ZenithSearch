#pragma once

#include "core/Interfaces.hpp"

namespace zenith::platform {

class StdFilesystemEnumerator final : public core::IFileEnumerator {
public:
    std::vector<core::FileItem> enumerate(const core::SearchRequest& request,
                                          std::stop_token stop_token,
                                          const ErrorCallback& on_error) const override;
};

} // namespace zenith::platform
