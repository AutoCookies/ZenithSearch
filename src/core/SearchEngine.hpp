#pragma once

#include "Interfaces.hpp"

namespace zenith::core {

class SearchEngine {
public:
    SearchEngine(const IFileEnumerator& enumerator,
                 const IFileReader& reader,
                 const ISearchAlgorithm& algorithm,
                 IOutputWriter& output,
                 IErrorWriter& errors)
        : enumerator_(enumerator), reader_(reader), algorithm_(algorithm), output_(output), errors_(errors) {}

    SearchStats run(const SearchRequest& request) const;

private:
    [[nodiscard]] bool is_binary_file(const std::string& path) const;

    const IFileEnumerator& enumerator_;
    const IFileReader& reader_;
    const ISearchAlgorithm& algorithm_;
    IOutputWriter& output_;
    IErrorWriter& errors_;
};

} // namespace zenith::core
