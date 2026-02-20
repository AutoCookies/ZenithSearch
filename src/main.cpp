#include "cli/ArgParser.hpp"
#include "core/NaiveSearchAlgorithm.hpp"
#include "core/SearchEngine.hpp"
#include "platform/MappedFileProvider.hpp"
#include "platform/OutputWriters.hpp"
#include "platform/StdFileReader.hpp"
#include "platform/StdFilesystemEnumerator.hpp"

#include <atomic>
#include <csignal>
#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace {
std::atomic<bool>* g_cancelled = nullptr;

void signal_handler(int) {
    if (g_cancelled != nullptr) {
        g_cancelled->store(true);
    }
}

#ifdef _WIN32
BOOL WINAPI ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
        if (g_cancelled != nullptr) {
            g_cancelled->store(true);
        }
        return TRUE;
    }
    return FALSE;
}
#endif
} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);

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
        std::cout << "zenithsearch v1.0.0\n";
        return 0;
    }

    std::atomic<bool> cancelled{false};
    g_cancelled = &cancelled;
    std::signal(SIGINT, signal_handler);
#ifndef _WIN32
    std::signal(SIGTERM, signal_handler);
#else
    SetConsoleCtrlHandler(ctrl_handler, TRUE);
#endif

    zenith::platform::StdFilesystemEnumerator enumerator;
    zenith::platform::StdFileReader reader;
    zenith::platform::MappedFileProvider mapped_provider;
    zenith::core::NaiveSearchAlgorithm naive_algorithm;
    zenith::core::BmhSearchAlgorithm bmh_algorithm;
    zenith::core::BoyerMooreSearchAlgorithm bm_algorithm;
    auto output = zenith::platform::make_output_writer(parsed.value().request, std::cout);
    zenith::platform::StreamErrorWriter err(std::cerr);

    zenith::core::SearchEngine engine(enumerator, reader, mapped_provider, naive_algorithm, bmh_algorithm, bm_algorithm, *output, err);

    std::stop_source stop_source;
    std::jthread cancel_monitor([&](std::stop_token st) {
        while (!st.stop_requested()) {
            if (cancelled.load()) {
                stop_source.request_stop();
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    const auto stats = engine.run(parsed.value().request, stop_source.get_token());
    cancel_monitor.request_stop();
    if (cancel_monitor.joinable()) cancel_monitor.join();

    if (stats.cancelled || cancelled.load()) return 130;
    return stats.any_match ? 0 : 1;
}
