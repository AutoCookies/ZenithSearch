#include "core/NaiveSearchAlgorithm.hpp"
#include "core/SearchEngine.hpp"

#include "doctest.h"

#include <memory>
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

class FakeMappedFile final : public zenith::core::IMappedFile {
public:
    explicit FakeMappedFile(std::string p, std::string d) : path_(std::move(p)), data_(std::move(d)) {}
    std::span<const std::byte> bytes() const override {
        return {reinterpret_cast<const std::byte*>(data_.data()), data_.size()};
    }
    std::uint64_t size() const override { return data_.size(); }
    const std::string& path() const override { return path_; }

private:
    std::string path_;
    std::string data_;
};

class FakeMappedProvider final : public zenith::core::IMappedFileProvider {
public:
    std::unordered_map<std::string, std::string> contents;
    bool fail{false};
    zenith::core::Expected<std::unique_ptr<zenith::core::IMappedFile>, zenith::core::Error> open(const std::string& path) const override {
        if (fail) {
            return zenith::core::Error{"forced fail"};
        }
        auto it = contents.find(path);
        if (it == contents.end()) {
            return zenith::core::Error{"missing"};
        }
        return std::unique_ptr<zenith::core::IMappedFile>(new FakeMappedFile(path, it->second));
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
    FakeMappedProvider mapped;
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;
    CaptureWriter out;
    CaptureError err;
    zenith::core::SearchEngine engine(en, reader, mapped, naive, bmh, bm, out, err);

    zenith::core::SearchRequest req;
    req.pattern = "abc";
    req.input_paths = {"f"};
    req.chunk_size = 4;
    req.mmap_mode = zenith::core::MmapMode::Off;

    auto stats = engine.run(req);
    CHECK(stats.any_match);
    CHECK(out.matches.size() == 2);
    CHECK(out.matches[0].offset == 2);
    CHECK(out.matches[1].offset == 7);
}

TEST_CASE("Binary skip and scan behavior") {
    FakeEnumerator en;
    en.files = {{"b", 4}};
    FakeReader reader;
    reader.contents = {{"b", std::string("a\0bc", 4)}};
    FakeMappedProvider mapped;
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;

    CaptureWriter out1;
    CaptureError err1;
    zenith::core::SearchEngine e1(en, reader, mapped, naive, bmh, bm, out1, err1);
    zenith::core::SearchRequest skip;
    skip.pattern = "bc";
    skip.input_paths = {"b"};
    skip.binary_mode = zenith::core::BinaryMode::Skip;
    skip.mmap_mode = zenith::core::MmapMode::Off;
    CHECK_FALSE(e1.run(skip).any_match);

    CaptureWriter out2;
    CaptureError err2;
    zenith::core::SearchEngine e2(en, reader, mapped, naive, bmh, bm, out2, err2);
    zenith::core::SearchRequest scan = skip;
    scan.binary_mode = zenith::core::BinaryMode::Scan;
    CHECK(e2.run(scan).any_match);
}

TEST_CASE("Algorithms return overlapping matches equivalently") {
    const std::string text = "aaaaaa";
    const std::string pat = "aaa";
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;
    const auto n = naive.find_all(text, pat);
    const auto h = bmh.find_all(text, pat);
    const auto b = bm.find_all(text, pat);
    CHECK(n.size() == 4);
    CHECK(n == h);
    CHECK(n == b);
}
