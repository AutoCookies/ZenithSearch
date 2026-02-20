#include "core/NaiveSearchAlgorithm.hpp"
#include "core/SearchEngine.hpp"
#include "platform/MappedFileProvider.hpp"
#include "platform/StdFileReader.hpp"
#include "platform/StdFilesystemEnumerator.hpp"

#include "doctest.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {
class CaptureWriter final : public zenith::core::IOutputWriter {
public:
    std::vector<zenith::core::MatchRecord> matches;
    std::vector<zenith::core::FileMatchSummary> summaries;
    void write_match(const zenith::core::MatchRecord& record) override { matches.push_back(record); }
    void write_file_summary(const zenith::core::FileMatchSummary& summary) override { summaries.push_back(summary); }
};

class CaptureError final : public zenith::core::IErrorWriter {
public:
    std::vector<std::string> errors;
    void write_error(const zenith::core::Error& error) override { errors.push_back(error.message); }
};
} // namespace

TEST_CASE("Mapped file reads bytes and handles empty file") {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "zenith_mmap_test";
    fs::remove_all(root);
    fs::create_directories(root);
    const auto file = root / "a.txt";
    const auto empty = root / "e.txt";
    std::ofstream(file) << "hello mmap";
    std::ofstream{empty};

    zenith::platform::MappedFileProvider provider;
    auto mapped = provider.open(file.string());
    REQUIRE(mapped.has_value());
    const auto bytes = mapped.value()->bytes();
    std::string str(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    CHECK(str == "hello mmap");

    auto mapped_empty = provider.open(empty.string());
    REQUIRE(mapped_empty.has_value());
    CHECK(mapped_empty.value()->bytes().size() == 0);

    fs::remove_all(root);
}

TEST_CASE("mmap and streaming count outputs are equivalent") {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "zenith_mmap_equiv";
    fs::remove_all(root);
    fs::create_directories(root);
    const auto file = root / "sample.txt";
    std::ofstream(file) << "abc xx abc yy abc";

    zenith::platform::StdFilesystemEnumerator en;
    zenith::platform::StdFileReader reader;
    zenith::platform::MappedFileProvider mapped;
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;

    CaptureWriter out_stream;
    CaptureError err_stream;
    zenith::core::SearchEngine eng_stream(en, reader, mapped, naive, bmh, bm, out_stream, err_stream);
    zenith::core::SearchRequest req_stream;
    req_stream.pattern = "abc";
    req_stream.input_paths = {root.string()};
    req_stream.output_mode = zenith::core::OutputMode::Count;
    req_stream.mmap_mode = zenith::core::MmapMode::Off;

    CaptureWriter out_mmap;
    CaptureError err_mmap;
    zenith::core::SearchEngine eng_mmap(en, reader, mapped, naive, bmh, bm, out_mmap, err_mmap);
    auto req_mmap = req_stream;
    req_mmap.mmap_mode = zenith::core::MmapMode::On;

    CHECK(eng_stream.run(req_stream).any_match);
    CHECK(eng_mmap.run(req_mmap).any_match);
    REQUIRE(out_stream.summaries.size() == 1);
    REQUIRE(out_mmap.summaries.size() == 1);
    CHECK(out_stream.summaries[0].count == out_mmap.summaries[0].count);

    fs::remove_all(root);
}
