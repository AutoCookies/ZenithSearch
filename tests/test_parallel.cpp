#include "core/NaiveSearchAlgorithm.hpp"
#include "core/SearchEngine.hpp"
#include "platform/MappedFileProvider.hpp"
#include "platform/StdFileReader.hpp"
#include "platform/StdFilesystemEnumerator.hpp"

#include "doctest.h"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>

namespace {
class CaptureWriter final : public zenith::core::IOutputWriter {
public:
    std::vector<std::string> lines;
    std::mutex m;
    void write_match(const zenith::core::MatchRecord& record) override {
        std::scoped_lock lock(m);
        lines.push_back(record.path + ":" + std::to_string(record.offset));
    }
    void write_file_summary(const zenith::core::FileMatchSummary& summary) override {
        std::scoped_lock lock(m);
        lines.push_back(summary.path + ":" + std::to_string(summary.count));
    }
};

class CaptureError final : public zenith::core::IErrorWriter {
public:
    void write_error(const zenith::core::Error&) override {}
};
} // namespace

TEST_CASE("stable output deterministic between thread counts") {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "zenith_parallel_stable";
    fs::remove_all(root);
    fs::create_directories(root);
    for (int i = 0; i < 40; ++i) {
        std::ofstream(root / ("f" + std::to_string(i) + ".txt")) << "a pattern b pattern";
    }

    zenith::platform::StdFilesystemEnumerator en;
    zenith::platform::StdFileReader reader;
    zenith::platform::MappedFileProvider mapped;
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;
    CaptureError err;

    zenith::core::SearchRequest req;
    req.pattern = "pattern";
    req.input_paths = {root.string()};
    req.output_mode = zenith::core::OutputMode::Count;
    req.stable_output = zenith::core::StableOutputMode::On;

    CaptureWriter out1;
    zenith::core::SearchEngine e1(en, reader, mapped, naive, bmh, bm, out1, err);
    req.threads = 1;
    e1.run(req);

    CaptureWriter out8;
    zenith::core::SearchEngine e8(en, reader, mapped, naive, bmh, bm, out8, err);
    req.threads = 8;
    e8.run(req);

    CHECK(out1.lines == out8.lines);
    fs::remove_all(root);
}

TEST_CASE("parallel many files no deadlock and correct count") {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "zenith_parallel_many";
    fs::remove_all(root);
    fs::create_directories(root);
    for (int i = 0; i < 600; ++i) {
        std::ofstream(root / ("f" + std::to_string(i) + ".txt")) << "token";
    }

    zenith::platform::StdFilesystemEnumerator en;
    zenith::platform::StdFileReader reader;
    zenith::platform::MappedFileProvider mapped;
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;
    CaptureWriter out;
    CaptureError err;
    zenith::core::SearchEngine engine(en, reader, mapped, naive, bmh, bm, out, err);

    zenith::core::SearchRequest req;
    req.pattern = "token";
    req.input_paths = {root.string()};
    req.output_mode = zenith::core::OutputMode::FilesWithMatches;
    req.stable_output = zenith::core::StableOutputMode::Off;
    req.threads = 16;

    CHECK(engine.run(req).any_match);
    CHECK(out.lines.size() == 600);

    fs::remove_all(root);
}
