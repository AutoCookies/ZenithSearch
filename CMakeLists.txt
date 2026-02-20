cmake_minimum_required(VERSION 3.20)
project(ZenithSearch VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_library(zenithsearch_core
  src/core/NaiveSearchAlgorithm.cpp
  src/core/SearchEngine.cpp
  src/cli/ArgParser.cpp
  src/platform/StdFilesystemEnumerator.cpp
  src/platform/StdFileReader.cpp
  src/platform/OutputWriters.cpp
)
target_include_directories(zenithsearch_core PUBLIC src)

add_executable(zenithsearch src/main.cpp)
target_link_libraries(zenithsearch PRIVATE zenithsearch_core)

include(CTest)
if(BUILD_TESTING)
  add_executable(zenithsearch_tests
    tests/test_main.cpp
    tests/test_arg_parser.cpp
    tests/test_search.cpp
    tests/test_filesystem.cpp
    tests/test_output.cpp
    tests/test_exit_codes.cpp
  )
  target_link_libraries(zenithsearch_tests PRIVATE zenithsearch_core)
  target_include_directories(zenithsearch_tests PRIVATE src tests)
  add_test(NAME zenithsearch_tests COMMAND zenithsearch_tests)
endif()
