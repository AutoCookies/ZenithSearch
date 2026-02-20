#include "platform/StdFilesystemEnumerator.hpp"

#include "doctest.h"

#include <filesystem>
#include <fstream>
#include <stop_token>

TEST_CASE("Filesystem filtering pipeline works with excludes and ignore files") {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "zenith_fs_filter";
    fs::remove_all(root);
    fs::create_directories(root / "sub");
    fs::create_directories(root / "node_modules");
    std::ofstream(root / ".zenithignore") << "sub/ignored.txt\n";
    std::ofstream(root / "a.cpp") << "x";
    std::ofstream(root / "sub" / "ignored.txt") << "x";
    std::ofstream(root / "node_modules" / "b.cpp") << "x";

    zenith::core::SearchRequest req;
    req.input_paths = {root.string()};
    req.exclude_dirs = {"node_modules"};
    req.include_globs = {"**/*.cpp"};

    zenith::platform::StdFilesystemEnumerator en;
    std::vector<std::string> errors;
    const auto files = en.enumerate(req, std::stop_token{}, [&](const zenith::core::Error& e) { errors.push_back(e.message); });

    CHECK(errors.empty());
    REQUIRE(files.size() == 1);
    CHECK(files[0].normalized_path.find("a.cpp") != std::string::npos);

    fs::remove_all(root);
}
