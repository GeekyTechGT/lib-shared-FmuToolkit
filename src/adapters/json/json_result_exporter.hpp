#pragma once

#include "ports/i_result_exporter.hpp"

namespace libfmu_toolkit {

// ---------------------------------------------------------------------------
// JsonResultExporter — exports SimulationResult to a JSON file
//
// Output format (no external dependencies — hand-written JSON):
// {
//   "timePoints": [0.0, 0.01, ...],
//   "series": {
//     "y": [1.0, 0.99, ...],
//     "x": [0.0, 0.001, ...]
//   }
// }
// ---------------------------------------------------------------------------

class JsonResultExporter final : public IResultExporter {
public:
    void exportResult(const SimulationResult& result,
                      const std::filesystem::path& outputPath) override;

    // Convenience static method (mirrors CsvResultExporter::exportToCsv)
    static void exportToJson(const SimulationResult& result,
                             const std::filesystem::path& outputPath);
};

} // namespace libfmu_toolkit
