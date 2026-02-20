#pragma once

#include "core/Interfaces.hpp"

namespace zenith::platform {

class MappedFileProvider final : public core::IMappedFileProvider {
public:
    core::Expected<std::unique_ptr<core::IMappedFile>, core::Error> open(const std::string& path) const override;
};

} // namespace zenith::platform
