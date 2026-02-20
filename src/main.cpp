#include "cli/ArgParser.hpp"
#include "core/NaiveSearchAlgorithm.hpp"
#include "core/SearchEngine.hpp"
#include "platform/MappedFileProvider.hpp"
#include "platform/OutputWriters.hpp"
#include "platform/StdFileReader.hpp"
#include "platform/StdFilesystemEnumerator.hpp"

#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    zenith::cli::ArgParser parser;
    auto parsed = parser.parse(args);
    if (!parsed) {
        std::cerr << "error: " << parsed.error().message << '\n';
        std::cerr << zenith::cli::ArgParser::help_text();
        return 2;
    }

    if (parsed.value().show_help) {
        std::cout << zenith::cli::ArgParser::help_text();
        return 0;
    }
    if (parsed.value().show_version) {
        std::cout << "zenithsearch v0.2.0\n";
        return 0;
    }

    zenith::platform::StdFilesystemEnumerator enumerator;
    zenith::platform::StdFileReader reader;
    zenith::platform::MappedFileProvider mapped_provider;
    zenith::core::NaiveSearchAlgorithm naive_algorithm;
    zenith::core::BmhSearchAlgorithm bmh_algorithm;
    zenith::core::BoyerMooreSearchAlgorithm bm_algorithm;
    auto output = zenith::platform::make_output_writer(parsed.value().request.json_output,
                                                        parsed.value().request.output_mode,
                                                        std::cout);
    zenith::platform::StreamErrorWriter err(std::cerr);

    zenith::core::SearchEngine engine(enumerator,
                                      reader,
                                      mapped_provider,
                                      naive_algorithm,
                                      bmh_algorithm,
                                      bm_algorithm,
                                      *output,
                                      err);
    const auto stats = engine.run(parsed.value().request);
    return stats.any_match ? 0 : 1;
}
