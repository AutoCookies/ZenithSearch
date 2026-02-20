#include "core/NaiveSearchAlgorithm.hpp"
#include "core/SearchEngine.hpp"

#include "doctest.h"

#include <memory>
#include <stop_token>
#include <unordered_map>

namespace {
class FakeEnumerator final : public zenith::core::IFileEnumerator {
public:
    std::vector<zenith::core::FileItem> files;
    std::vector<zenith::core::FileItem> enumerate(const zenith::core::SearchRequest&, std::stop_token, const ErrorCallback&) const override {
        return files;
    }
};

class FakeReader final : public zenith::core::IFileReader {
public:
    std::unordered_map<std::string, std::string> contents;
    zenith::core::Expected<std::string, zenith::core::Error> read_prefix(const std::string& path, std::size_t max_bytes) const override {
        auto it = contents.find(path);
        if (it == contents.end()) return zenith::core::Error{"missing"};
        return it->second.substr(0, max_bytes);
    }
    zenith::core::Expected<void, zenith::core::Error> read_chunks(const std::string& path,
                                                                   std::size_t chunk_size,
                                                                   std::stop_token,
                                                                   const std::function<zenith::core::Expected<void, zenith::core::Error>(const std::string&)>& on_chunk) const override {
        auto it = contents.find(path);
        if (it == contents.end()) return zenith::core::Error{"missing"};
        for (std::size_t i = 0; i < it->second.size(); i += chunk_size) {
            auto r = on_chunk(it->second.substr(i, chunk_size));
            if (!r) return r.error();
        }
        return {};
    }
};

class FakeMappedFile final : public zenith::core::IMappedFile {
public:
    FakeMappedFile(std::string p, std::string d) : p_(std::move(p)), d_(std::move(d)) {}
    std::span<const std::byte> bytes() const override { return {reinterpret_cast<const std::byte*>(d_.data()), d_.size()}; }
    std::uint64_t size() const override { return d_.size(); }
    const std::string& path() const override { return p_; }

private:
    std::string p_;
    std::string d_;
};

class FakeMappedProvider final : public zenith::core::IMappedFileProvider {
public:
    std::unordered_map<std::string, std::string> contents;
    zenith::core::Expected<std::unique_ptr<zenith::core::IMappedFile>, zenith::core::Error> open(const std::string& path) const override {
        auto it = contents.find(path);
        if (it == contents.end()) return zenith::core::Error{"missing"};
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
    void write_error(const zenith::core::Error&) override {}
};
} // namespace

TEST_CASE("Search finds chunk boundary matches and max-matches cap") {
    FakeEnumerator en;
    en.files = {{"f", "f", 12}};
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
    req.max_matches_per_file = 1;

    auto stats = engine.run(req);
    CHECK(stats.any_match);
    CHECK(out.matches.size() == 1);
    CHECK(out.matches[0].offset == 2);
}

TEST_CASE("Count mode counts all even with max-matches") {
    FakeEnumerator en;
    en.files = {{"f", "f", 6}};
    FakeReader reader;
    reader.contents = {{"f", "aaaaaa"}};
    FakeMappedProvider mapped;
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;
    CaptureWriter out;
    CaptureError err;
    zenith::core::SearchEngine engine(en, reader, mapped, naive, bmh, bm, out, err);

    zenith::core::SearchRequest req;
    req.pattern = "aa";
    req.input_paths = {"f"};
    req.output_mode = zenith::core::OutputMode::Count;
    req.max_matches_per_file = 1;
    req.mmap_mode = zenith::core::MmapMode::Off;

    auto stats = engine.run(req);
    CHECK(stats.any_match);
    REQUIRE(out.summaries.size() == 1);
    CHECK(out.summaries[0].count == 5);
}

TEST_CASE("Algorithms overlap equivalence") {
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;
    auto n = naive.find_all("aaaaaa", "aaa");
    CHECK(n == bmh.find_all("aaaaaa", "aaa"));
    CHECK(n == bm.find_all("aaaaaa", "aaa"));
}
