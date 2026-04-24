/// @file lib_fmu_toolkit.hpp
/// @brief Umbrella header — includes the complete lib-fmu-toolkit public API.
///
/// Including this single header is sufficient for most consumers. It pulls in:
/// - `export.hpp`   — DLL visibility macros
/// - `types.hpp`    — all public data types (FmuInfo, SimConfig, SimResult, …)
/// - `exceptions.hpp` — the public exception hierarchy
/// - `fmu_engine.hpp` — FmuEngine and FmuSession
/// - `version.hpp`  — compile-time version constants
///
/// @code
/// #include <libfmu_toolkit/lib_fmu_toolkit.hpp>
///
/// libfmu_toolkit::FmuEngine engine;
/// auto handle = engine.load("model.fmu");
/// @endcode
#pragma once

#include "export.hpp"
#include "exceptions.hpp"
#include "fmu_generation.hpp"
#include "fmu_engine.hpp"
#include "types.hpp"
#include <libfmu_toolkit/version.hpp>
