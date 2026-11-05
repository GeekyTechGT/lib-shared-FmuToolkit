// =============================================================================
// Example 02 — Step-by-Step Simulation (FmuSession)
//
// Demonstrates fine-grained control over the co-simulation loop using
// FmuSession. This pattern is required when an external time-synchronisation
// coordinator (e.g. SIL-Kit, AUTOSAR) drives simulation time.
//
// Workflow:
//   1. Load the FMU and open a session (engine initialises the FMU)
//   2. Drive the loop manually: set inputs → doStep → read outputs
//   3. Terminate the session explicitly
//
// Usage:
//   example_02_step_by_step path/to/model.fmu [stopTime] [stepSize]
// =============================================================================

#include <libfmu_toolkit/lib_fmu_toolkit.hpp>

#include <cstdlib>
#include <iomanip>
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
        std::cout << "Loaded: " << info.modelName << "  (FMI "
                  << (info.fmiVersion == libfmu_toolkit::FmiVersion::FMI2 ? "2" : "3")
                  << ")\n";

        // List output variables
        auto outputs = info.outputs();
        std::cout << "Output variables: ";
        for (const auto* v : outputs) std::cout << v->name << " ";
        std::cout << "\n\n";

        // Open session — FMU is instantiated and initialised immediately
        libfmu_toolkit::SimConfig cfg;
        cfg.startTime = 0.0;
        cfg.stopTime  = stopTime;
        cfg.stepSize  = stepSize;

        libfmu_toolkit::FmuSession session = engine.openSession(handle, cfg);

        double t = 0.0;
        const int totalSteps = static_cast<int>((stopTime - cfg.startTime) / stepSize);

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "time";
        for (const auto* v : outputs) std::cout << "\t" << v->name;
        std::cout << "\n";

        for (int step = 0; step <= totalSteps && session.isActive(); ++step) {
            // Read outputs at current time
            std::cout << t;
            for (const auto* v : outputs) {
                auto val = session.getValue(v->name);
                std::cout << "\t";
                std::visit([](auto&& x) { std::cout << x; }, val);
            }
            std::cout << "\n";

            // Advance by one step
            if (step < totalSteps) {
                if (!session.doStep(t, stepSize))
                    break;
                t += stepSize;
            }
        }

        session.terminate();
        engine.unload(handle);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << "\n";
    }
    return EXIT_FAILURE;
}
