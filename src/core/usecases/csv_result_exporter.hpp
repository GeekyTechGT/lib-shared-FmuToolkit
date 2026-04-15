#pragma once

#include "ports/i_result_exporter.hpp"
#include "core/domain/simulation_result.hpp"

#include <filesystem>

namespace libfmu_toolkit {

/// @brief Exports a SimulationResult to a comma-separated values (CSV) file.
///
/// Implements the IResultExporter port so callers can use it polymorphically.
/// The static exportToCsv() overload is provided for convenience when dynamic
/// dispatch is not needed.
class CsvResultExporter final : public IResultExporter {
public:
    // IResultExporter interface
    void exportResult(const SimulationResult& result,
                      const std::filesystem::path& outputPath) override;

    /// Convenience static method — equivalent to constructing an instance and
    /// calling exportResult().
    static void exportToCsv(const SimulationResult& result,
                             const std::filesystem::path& outputPath);
};

} // namespace libfmu_toolkit
