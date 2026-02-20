#include "OutputWriters.hpp"

#include <ostream>

namespace zenith::platform {

void StreamErrorWriter::write_error(const core::Error& error) { out_ << error.message << '\n'; }

void HumanOutputWriter::write_match(const core::MatchRecord& record) {
    out_ << record.path << ':' << record.offset << ':' << record.snippet << '\n';
}

void HumanOutputWriter::write_file_summary(const core::FileMatchSummary& summary) {
    if (mode_ == core::OutputMode::Count) {
        out_ << summary.path << ':' << summary.count << '\n';
    } else {
        out_ << summary.path << '\n';
    }
}

void JsonlOutputWriter::write_match(const core::MatchRecord& record) {
    out_ << "{\"type\":\"match\",\"path\":\"" << core::json_escape(record.path) << "\",\"offset\":" << record.offset
         << ",\"snippet\":\"" << core::json_escape(record.snippet) << "\"}" << '\n';
}

void JsonlOutputWriter::write_file_summary(const core::FileMatchSummary& summary) {
    if (mode_ == core::OutputMode::Count) {
        out_ << "{\"type\":\"count\",\"path\":\"" << core::json_escape(summary.path) << "\",\"count\":" << summary.count << "}" << '\n';
    } else {
        out_ << "{\"type\":\"file\",\"path\":\"" << core::json_escape(summary.path) << "\"}" << '\n';
    }
}

std::unique_ptr<core::IOutputWriter> make_output_writer(bool json, core::OutputMode mode, std::ostream& out) {
    if (json) {
        return std::make_unique<JsonlOutputWriter>(out, mode);
    }
    return std::make_unique<HumanOutputWriter>(out, mode);
}

} // namespace zenith::platform
