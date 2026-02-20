#pragma once

#include "Interfaces.hpp"

#include <stop_token>

namespace zenith::core {

class SearchEngine {
public:
    SearchEngine(const IFileEnumerator& enumerator,
                 const IFileReader& reader,
                 const IMappedFileProvider& mapped_provider,
                 const ISearchAlgorithm& naive_algorithm,
                 const ISearchAlgorithm& bmh_algorithm,
                 const ISearchAlgorithm& boyer_moore_algorithm,
                 IOutputWriter& output,
                 IErrorWriter& errors)
        : enumerator_(enumerator),
          reader_(reader),
          mapped_provider_(mapped_provider),
          naive_algorithm_(naive_algorithm),
          bmh_algorithm_(bmh_algorithm),
          boyer_moore_algorithm_(boyer_moore_algorithm),
          output_(output),
          errors_(errors) {}

    SearchStats run(const SearchRequest& request, std::stop_token stop_token = {}) const;

private:
    const ISearchAlgorithm& choose_algorithm(AlgorithmMode mode, std::size_t pattern_len, std::uintmax_t file_size) const;

    const IFileEnumerator& enumerator_;
    const IFileReader& reader_;
    const IMappedFileProvider& mapped_provider_;
    const ISearchAlgorithm& naive_algorithm_;
    const ISearchAlgorithm& bmh_algorithm_;
    const ISearchAlgorithm& boyer_moore_algorithm_;
    IOutputWriter& output_;
    IErrorWriter& errors_;
};

} // namespace zenith::core
