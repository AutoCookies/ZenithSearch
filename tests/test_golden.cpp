#include "core/NaiveSearchAlgorithm.hpp"
#include "core/SearchEngine.hpp"
#include "platform/MappedFileProvider.hpp"
#include "platform/OutputWriters.hpp"
#include "platform/StdFileReader.hpp"
#include "platform/StdFilesystemEnumerator.hpp"

#include "doctest.h"

#include <filesystem>
#include <fstream>
#include <sstream>

class CaptureErr final : public zenith::core::IErrorWriter {
public:
    void write_error(const zenith::core::Error&) override {}
};

TEST_CASE("golden stable output snapshot") {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "zenith_golden";
    fs::remove_all(root);
    fs::create_directories(root / "d");
    std::ofstream(root / "a.txt") << "hello pat world pat";
    std::ofstream(root / "d" / "b.txt") << "pat";

    zenith::core::SearchRequest req;
    req.pattern = "pat";
    req.input_paths = {root.string()};
    req.stable_output = zenith::core::StableOutputMode::On;
    req.mmap_mode = zenith::core::MmapMode::Off;

    std::ostringstream os;
    auto writer = zenith::platform::make_output_writer(req, os);
    CaptureErr err;
    zenith::platform::StdFilesystemEnumerator en;
    zenith::platform::StdFileReader reader;
    zenith::platform::MappedFileProvider mapped;
    zenith::core::NaiveSearchAlgorithm naive;
    zenith::core::BmhSearchAlgorithm bmh;
    zenith::core::BoyerMooreSearchAlgorithm bm;
    zenith::core::SearchEngine engine(en, reader, mapped, naive, bmh, bm, *writer, err);
    CHECK(engine.run(req).any_match);

    const auto out = os.str();
    CHECK(out.find("a.txt") != std::string::npos);
    CHECK(out.find("b.txt") != std::string::npos);

    fs::remove_all(root);
}
