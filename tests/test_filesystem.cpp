#include "platform/StdFilesystemEnumerator.hpp"

#include "doctest.h"

#include <filesystem>
#include <fstream>

TEST_CASE("Filesystem enumerates recursively with filters") {
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / "zenith_fs_test";
    fs::remove_all(root);
    fs::create_directories(root / "sub");
    fs::create_directories(root / ".hidden");

    std::ofstream(root / "a.cpp") << "x";
    std::ofstream(root / "sub" / "b.h") << "x";
    std::ofstream(root / ".hidden" / "c.cpp") << "x";

    zenith::platform::StdFilesystemEnumerator en;
    std::vector<std::string> errors;
    const auto files = en.enumerate({root.string()}, true, {".cpp", ".h"}, std::nullopt,
                                    [&](const zenith::core::Error& e) { errors.push_back(e.message); });

    CHECK(errors.empty());
    CHECK(files.size() == 2);

    fs::remove_all(root);
}
