#pragma once

/// @file i_result_exporter.hpp
/// @brief Port interface for exporting simulation results.
/// @internal

#include "core/domain/simulation_result.hpp"
#include <filesystem>

namespace libfmu_toolkit {

/// @brief Port interface for exporting simulation results (Open/Closed Principle).
///
/// Provides a stable interface for writing SimulationResult data to any file
/// format without modifying simulation use-case logic.
///
/// Built-in implementations:
///   - `CsvResultExporter` — RFC-4180 CSV, no external dependencies.
///   - `JsonResultExporter` — hand-written JSON, no external dependencies.
/// @req FR-06.1
class IResultExporter {
public:
    virtual ~IResultExporter() = default;

    /// @brief Exports a simulation result to a file.
    ///
    /// @param result      The simulation result to serialise.
    /// @param outputPath  Destination file path. Created or overwritten.
    ///
    /// @throws std::ios_base::failure  if the file cannot be opened or written.
    /// @req FR-06.1
    virtual void exportResult(const SimulationResult& result,
                              const std::filesystem::path& outputPath) = 0;
};

} // namespace libfmu_toolkit
