#include "core/NaiveSearchAlgorithm.hpp"
#include "core/SearchEngine.hpp"

#include "doctest.h"

#include <unordered_map>

namespace {
class FakeEnumerator final : public zenith::core::IFileEnumerator {
public:
    std::vector<zenith::core::FileItem> files;
    std::vector<zenith::core::FileItem> enumerate(const std::vector<std::string>&,
                                                  bool,
                                                  const std::unordered_set<std::string>&,
                                                  const std::optional<std::uintmax_t>&,
                                                  const ErrorCallback&) const override {
        return files;
    }
};

class FakeReader final : public zenith::core::IFileReader {
public:
    std::unordered_map<std::string, std::string> contents;

    zenith::core::Expected<std::string, zenith::core::Error> read_prefix(const std::string& path, std::size_t max_bytes) const override {
        auto it = contents.find(path);
        if (it == contents.end()) {
            return zenith::core::Error{"missing"};
        }
        return it->second.substr(0, max_bytes);
    }

    zenith::core::Expected<void, zenith::core::Error> read_chunks(
        const std::string& path,
        std::size_t chunk_size,
        const std::function<zenith::core::Expected<void, zenith::core::Error>(const std::string&)>& on_chunk) const override {
        auto it = contents.find(path);
        if (it == contents.end()) {
            return zenith::core::Error{"missing"};
        }
        for (std::size_t i = 0; i < it->second.size(); i += chunk_size) {
            auto result = on_chunk(it->second.substr(i, chunk_size));
            if (!result) {
                return result.error();
            }
        }
        return {};
    }
};

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

TEST_CASE("Search finds matches across chunk boundaries") {
    FakeEnumerator en;
    en.files = {{"f", 12}};
    FakeReader reader;
    reader.contents = {{"f", "xxab" "cxxabc"}};
    zenith::core::NaiveSearchAlgorithm algo;
    CaptureWriter out;
    CaptureError err;
    zenith::core::SearchEngine engine(en, reader, algo, out, err);

    zenith::core::SearchRequest req;
    req.pattern = "abc";
    req.input_paths = {"f"};
    req.chunk_size = 4;

    auto stats = engine.run(req);
    CHECK(stats.any_match);
    CHECK(out.matches.size() == 2);
    CHECK(out.matches[0].offset == 2);
    CHECK(out.matches[1].offset == 7);
}

TEST_CASE("Search count mode reports file count") {
    FakeEnumerator en;
    en.files = {{"f", 5}};
    FakeReader reader;
    reader.contents = {{"f", "aaaaa"}};
    zenith::core::NaiveSearchAlgorithm algo;
    CaptureWriter out;
    CaptureError err;
    zenith::core::SearchEngine engine(en, reader, algo, out, err);

    zenith::core::SearchRequest req;
    req.pattern = "aa";
    req.output_mode = zenith::core::OutputMode::Count;
    req.input_paths = {"f"};
    auto stats = engine.run(req);

    CHECK(stats.any_match);
    REQUIRE(out.summaries.size() == 1);
    CHECK(out.summaries[0].count == 4);
}

TEST_CASE("Binary skip and scan behavior") {
    FakeEnumerator en;
    en.files = {{"b", 4}};
    FakeReader reader;
    reader.contents = {{"b", std::string("a\0bc", 4)}};
    zenith::core::NaiveSearchAlgorithm algo;

    CaptureWriter out1;
    CaptureError err1;
    zenith::core::SearchEngine e1(en, reader, algo, out1, err1);
    zenith::core::SearchRequest skip;
    skip.pattern = "bc";
    skip.input_paths = {"b"};
    skip.binary_mode = zenith::core::BinaryMode::Skip;
    CHECK_FALSE(e1.run(skip).any_match);

    CaptureWriter out2;
    CaptureError err2;
    zenith::core::SearchEngine e2(en, reader, algo, out2, err2);
    zenith::core::SearchRequest scan = skip;
    scan.binary_mode = zenith::core::BinaryMode::Scan;
    CHECK(e2.run(scan).any_match);
}

TEST_CASE("Pattern longer than file yields no match") {
    FakeEnumerator en;
    en.files = {{"f", 2}};
    FakeReader reader;
    reader.contents = {{"f", "ab"}};
    zenith::core::NaiveSearchAlgorithm algo;
    CaptureWriter out;
    CaptureError err;
    zenith::core::SearchEngine engine(en, reader, algo, out, err);

    zenith::core::SearchRequest req;
    req.pattern = "abcdef";
    req.input_paths = {"f"};
    CHECK_FALSE(engine.run(req).any_match);
}
