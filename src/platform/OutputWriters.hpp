#pragma once

#include "core/Interfaces.hpp"
#include "core/TextUtils.hpp"
#include "core/Types.hpp"

#include <iosfwd>
#include <memory>
#include <string>

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
    HumanOutputWriter(std::ostream& out, core::OutputMode mode, bool no_snippet)
        : out_(out), mode_(mode), no_snippet_(no_snippet) {}
    void write_match(const core::MatchRecord& record) override;
    void write_file_summary(const core::FileMatchSummary& summary) override;

private:
    std::ostream& out_;
    core::OutputMode mode_;
    bool no_snippet_;
};

class JsonlOutputWriter final : public core::IOutputWriter {
public:
    JsonlOutputWriter(std::ostream& out, core::OutputMode mode, const std::string& pattern, bool no_snippet)
        : out_(out), mode_(mode), pattern_(pattern), no_snippet_(no_snippet) {}
    void write_match(const core::MatchRecord& record) override;
    void write_file_summary(const core::FileMatchSummary& summary) override;

private:
    std::ostream& out_;
    core::OutputMode mode_;
    std::string pattern_;
    bool no_snippet_;
};

std::unique_ptr<core::IOutputWriter> make_output_writer(const core::SearchRequest& request, std::ostream& out);

} // namespace zenith::platform
