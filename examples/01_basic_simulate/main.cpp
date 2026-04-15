// =============================================================================
// Example 01 — Basic Simulate
//
// Demonstrates the minimal workflow for loading and simulating an FMU:
//   1. Construct FmuEngine
//   2. Load an FMU file
//   3. Inspect metadata (FmuInfo)
//   4. Run a co-simulation with default config
//   5. Print output variable time series
//
// Usage:
//   example_01_basic_simulate path/to/model.fmu
// =============================================================================

#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path-to-fmu>\n";
        return EXIT_FAILURE;
    }

    const std::string fmuPath = argv[1];

    try {
        // 1. Create the engine (auto-discovers XSD schemas)
        libfmu_toolkit::FmuEngine engine;

        // 2. Load the FMU: extracts the archive, parses modelDescription.xml,
        //    loads the shared library, and resolves all FMI function symbols.
        const uint64_t handle = engine.load(fmuPath);

        // 3. Inspect metadata
        const libfmu_toolkit::FmuInfo info = engine.info(handle);

        std::cout << "Model   : " << info.modelName << "\n";
        std::cout << "FMI     : " << (info.fmiVersion == libfmu_toolkit::FmiVersion::FMI2
                                      ? "2.0" : "3.0") << "\n";
        std::cout << "GUID    : " << info.guid << "\n";
        if (!info.description.empty())
            std::cout << "Desc    : " << info.description << "\n";

        if (info.defaultExperiment.hasStopTime)
            std::cout << "DefaultStopTime : " << info.defaultExperiment.stopTime << "\n";

        std::cout << "\nVariables (" << info.variables.size() << "):\n";
        for (const auto& v : info.variables) {
            std::cout << "  [" << v.name << "]"
                      << "  causality=" << static_cast<int>(v.causality)
                      << "  type=" << static_cast<int>(v.dataType);
            if (!v.unit.empty()) std::cout << "  unit=" << v.unit;
            std::cout << "\n";
        }

        // 4. Configure and run co-simulation
        libfmu_toolkit::SimConfig cfg;
        cfg.startTime = 0.0;
        cfg.stopTime  = info.defaultExperiment.hasStopTime
                        ? info.defaultExperiment.stopTime : 1.0;
        cfg.stepSize  = info.defaultExperiment.hasStepSize
                        ? info.defaultExperiment.stepSize : 0.01;

        std::cout << "\nSimulating [0, " << cfg.stopTime
                  << "] with stepSize=" << cfg.stepSize << " ...\n";

        const libfmu_toolkit::SimResult result = engine.simulate(handle, cfg,
            [](size_t step, double t, size_t total) {
                if (step % 100 == 0)
                    std::cout << "  t=" << std::setprecision(4) << t
                              << " (" << step << "/" << total << ")\r" << std::flush;
            });

        std::cout << "\n\nDone. Recorded " << result.stepCount() << " time points.\n";

        // 5. Print last value of each output variable
        std::cout << "\nFinal output values:\n";
        for (const auto& [name, series] : result.series) {
            if (series.empty()) continue;
            std::cout << "  " << name << " = ";
            std::visit([](auto&& v) { std::cout << v; }, series.back());
            std::cout << "\n";
        }

        engine.unload(handle);
        return EXIT_SUCCESS;
    }
    catch (const libfmu_toolkit::FmuLoadError& e) {
        std::cerr << "[Load Error] " << e.what() << "\n";
    }
    catch (const libfmu_toolkit::FmuSimulationError& e) {
        std::cerr << "[Simulation Error] " << e.what() << "\n";
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << "\n";
    }
    return EXIT_FAILURE;
}
