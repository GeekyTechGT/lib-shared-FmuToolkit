#pragma once

/// @file csv_result_exporter.hpp
/// @brief Use case / adapter: export a SimulationResult to a CSV file.
/// @internal

#include "domain/port/out/i_result_exporter.hpp"
#include "domain/model/simulation_result.hpp"

#include <filesystem>

namespace libfmu_toolkit {

/// @brief Exports a SimulationResult to a comma-separated values (CSV) file.
///
/// Implements the IResultExporter port so callers can use it polymorphically.
/// The static exportToCsv() overload is provided for convenience when dynamic
/// dispatch is not needed.
///
/// @par Requirements
/// Implements FR-06.1 and FR-06.3 (CSV export, I/O error reporting).
/// @req FR-06.1
class CsvResultExporter final : public IResultExporter {
public:
    /// @brief Writes @p result to @p outputPath in CSV format (IResultExporter port).
    /// @throws std::ios_base::failure if the file cannot be opened or written.
    /// @req FR-06.1
    void exportResult(const SimulationResult& result,
                      const std::filesystem::path& outputPath) override;

    /// @brief Convenience static export — equivalent to a default-constructed
    ///        instance calling exportResult().
    /// @req FR-06.1
    static void exportToCsv(const SimulationResult& result,
                             const std::filesystem::path& outputPath);
};

} // namespace libfmu_toolkit
