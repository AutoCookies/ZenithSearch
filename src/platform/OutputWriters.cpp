#include "OutputWriters.hpp"

#include <ostream>

namespace zenith::platform {

void StreamErrorWriter::write_error(const core::Error& error) { out_ << error.message << '\n'; }

void HumanOutputWriter::write_match(const core::MatchRecord& record) {
    out_ << record.path << ':' << record.offset;
    if (!no_snippet_) {
        out_ << ':' << record.snippet;
    }
    out_ << '\n';
}

void HumanOutputWriter::write_file_summary(const core::FileMatchSummary& summary) {
    if (mode_ == core::OutputMode::Count) {
        out_ << summary.path << ':' << summary.count << '\n';
    } else {
        out_ << summary.path << '\n';
    }
}

void JsonlOutputWriter::write_match(const core::MatchRecord& record) {
    out_ << "{\"path\":\"" << core::json_escape(record.path) << "\",\"mode\":\"match\",\"pattern\":\""
         << core::json_escape(pattern_) << "\",\"offset\":" << record.offset << ",\"binary\":" << (record.binary ? "true" : "false");
    if (!no_snippet_) {
        out_ << ",\"snippet\":\"" << core::json_escape(record.snippet) << "\"";
    }
    out_ << "}" << '\n';
}

void JsonlOutputWriter::write_file_summary(const core::FileMatchSummary& summary) {
    const char* mode_name = mode_ == core::OutputMode::Count ? "count" : "files_with_matches";
    out_ << "{\"path\":\"" << core::json_escape(summary.path) << "\",\"mode\":\"" << mode_name << "\",\"pattern\":\""
         << core::json_escape(pattern_) << "\",\"binary\":" << (summary.binary ? "true" : "false");
    if (mode_ == core::OutputMode::Count) {
        out_ << ",\"count\":" << summary.count;
    }
    out_ << "}" << '\n';
}

std::unique_ptr<core::IOutputWriter> make_output_writer(const core::SearchRequest& request, std::ostream& out) {
    if (request.json_output) {
        return std::make_unique<JsonlOutputWriter>(out, request.output_mode, request.pattern, request.no_snippet);
    }
    return std::make_unique<HumanOutputWriter>(out, request.output_mode, request.no_snippet);
}

} // namespace zenith::platform
