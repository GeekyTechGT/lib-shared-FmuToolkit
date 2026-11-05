// =============================================================================
// Example 04 — Export Simulation Results (CSV + JSON)
//
// Demonstrates both built-in result exporters:
//   - FmuEngine::exportCsv()  — CSV with header row
//   - FmuEngine::exportJson() — JSON with timePoints and series arrays
//
// Usage:
//   example_04_export_results path/to/model.fmu [stopTime] [stepSize]
//
// Outputs:
//   results.csv
//   results.json
// =============================================================================

#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <path-to-fmu> [stopTime=1.0] [stepSize=0.01]\n";
        return EXIT_FAILURE;
    }

    const std::string fmuPath  = argv[1];
    const double      stopTime = (argc >= 3) ? std::stod(argv[2]) : 1.0;
    const double      stepSize = (argc >= 4) ? std::stod(argv[3]) : 0.01;

    try {
        libfmu_toolkit::FmuEngine engine;
        const uint64_t handle = engine.load(fmuPath);

        const libfmu_toolkit::FmuInfo info = engine.info(handle);
        std::cout << "Loaded : " << info.modelName << "\n";
        std::cout << "Outputs: ";
        for (const auto* v : info.outputs()) std::cout << v->name << " ";
        std::cout << "\n";

        libfmu_toolkit::SimConfig cfg;
        cfg.startTime = 0.0;
        cfg.stopTime  = stopTime;
        cfg.stepSize  = stepSize;

        std::cout << "Simulating [0, " << stopTime << "] ...\n";
        const libfmu_toolkit::SimResult result = engine.simulate(handle, cfg);
        std::cout << "Done. " << result.stepCount() << " time points recorded.\n\n";

        // Export to CSV
        const std::string csvPath = "results.csv";
        libfmu_toolkit::FmuEngine::exportCsv(result, csvPath);
        std::cout << "CSV  written: " << std::filesystem::absolute(csvPath).string() << "\n";

        // Export to JSON
        const std::string jsonPath = "results.json";
        libfmu_toolkit::FmuEngine::exportJson(result, jsonPath);
        std::cout << "JSON written: " << std::filesystem::absolute(jsonPath).string() << "\n";

        engine.unload(handle);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << "\n";
    }
    return EXIT_FAILURE;
}
