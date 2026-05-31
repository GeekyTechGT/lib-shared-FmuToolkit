#pragma once

/// @file json_result_exporter.hpp
/// @brief Hand-written JSON exporter implementing the IResultExporter port.
/// @internal

#include "ports/i_result_exporter.hpp"

namespace libfmu_toolkit {

/// @brief Exports a SimulationResult to a JSON file (no external dependencies).
///
/// Concrete IResultExporter that serialises results with a small hand-written
/// JSON writer. Output format:
/// @code
/// {
///   "timePoints": [0.0, 0.01, ...],
///   "series": {
///     "y": [1.0, 0.99, ...],
///     "x": [0.0, 0.001, ...]
///   }
/// }
/// @endcode
///
/// @par Requirements
/// Implements FR-06.2 and FR-06.3 (JSON export without an external JSON library,
/// I/O error reporting).
/// @req FR-06.2
class JsonResultExporter final : public IResultExporter {
public:
    /// @brief Writes @p result to @p outputPath in JSON format (IResultExporter port).
    /// @throws std::ios_base::failure if the file cannot be opened or written.
    /// @req FR-06.2
    void exportResult(const SimulationResult& result,
                      const std::filesystem::path& outputPath) override;

    /// @brief Convenience static export — mirrors CsvResultExporter::exportToCsv().
    /// @req FR-06.2
    static void exportToJson(const SimulationResult& result,
                             const std::filesystem::path& outputPath);
};

} // namespace libfmu_toolkit
