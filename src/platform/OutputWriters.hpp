#pragma once

#include "core/Interfaces.hpp"
#include "core/TextUtils.hpp"
#include "core/Types.hpp"

#include <iosfwd>
#include <memory>

namespace zenith::platform {

class StreamErrorWriter final : public core::IErrorWriter {
public:
    explicit StreamErrorWriter(std::ostream& out) : out_(out) {}
    void write_error(const core::Error& error) override;

private:
    std::ostream& out_;
};

class HumanOutputWriter final : public core::IOutputWriter {
public:
    explicit HumanOutputWriter(std::ostream& out, core::OutputMode mode) : out_(out), mode_(mode) {}
    void write_match(const core::MatchRecord& record) override;
    void write_file_summary(const core::FileMatchSummary& summary) override;

private:
    std::ostream& out_;
    core::OutputMode mode_;
};

class JsonlOutputWriter final : public core::IOutputWriter {
public:
    explicit JsonlOutputWriter(std::ostream& out, core::OutputMode mode) : out_(out), mode_(mode) {}
    void write_match(const core::MatchRecord& record) override;
    void write_file_summary(const core::FileMatchSummary& summary) override;

private:
    std::ostream& out_;
    core::OutputMode mode_;
};

std::unique_ptr<core::IOutputWriter> make_output_writer(bool json, core::OutputMode mode, std::ostream& out);

} // namespace zenith::platform
