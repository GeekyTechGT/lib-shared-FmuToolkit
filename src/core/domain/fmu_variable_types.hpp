#pragma once

/// @file fmu_variable_types.hpp
/// @brief Internal re-export of the public FMI enums plus stringification helpers.
/// @internal
///
/// The canonical enum definitions (FmiVersion, Causality, Variability, Initial,
/// DataType) live in the public API header `<libfmu_toolkit/types.hpp>`. This
/// internal header re-exports them and adds `constexpr` helpers (`to_string`,
/// `isFmi3Only`) that are **not** part of the public API. Including this file
/// from internal code provides both the enums and the helpers in one place.

#include <libfmu_toolkit/types.hpp>   // FmiVersion, Causality, Variability, Initial, DataType

#include <string_view>

namespace libfmu_toolkit {

// ---------------------------------------------------------------------------
// constexpr helpers — human-readable names (internal use only)
// ---------------------------------------------------------------------------

/// @brief Returns the canonical FMI string name of an FMI version (`"FMI2"`/`"FMI3"`).
/// @req FR-01.4
[[nodiscard]] constexpr std::string_view to_string(FmiVersion v) noexcept {
    switch (v) {
        case FmiVersion::FMI2: return "FMI2";
        case FmiVersion::FMI3: return "FMI3";
    }
    return "Unknown";
}

/// @brief Returns the FMI XML spelling of a causality (e.g. `"input"`, `"output"`).
/// @req FR-02.2
[[nodiscard]] constexpr std::string_view to_string(Causality c) noexcept {
    switch (c) {
        case Causality::Parameter:           return "parameter";
        case Causality::CalculatedParameter: return "calculatedParameter";
        case Causality::Input:               return "input";
        case Causality::Output:              return "output";
        case Causality::Local:               return "local";
        case Causality::Independent:         return "independent";
    }
    return "unknown";
}

/// @brief Returns the FMI XML spelling of a variability (e.g. `"continuous"`, `"tunable"`).
/// @req FR-02.2
[[nodiscard]] constexpr std::string_view to_string(Variability v) noexcept {
    switch (v) {
        case Variability::Constant:   return "constant";
        case Variability::Fixed:      return "fixed";
        case Variability::Tunable:    return "tunable";
        case Variability::Discrete:   return "discrete";
        case Variability::Continuous: return "continuous";
    }
    return "unknown";
}

/// @brief Returns the FMI XML spelling of an initial kind (e.g. `"exact"`, `"calculated"`).
/// @req FR-02.2
[[nodiscard]] constexpr std::string_view to_string(Initial i) noexcept {
    switch (i) {
        case Initial::Exact:      return "exact";
        case Initial::Approx:     return "approx";
        case Initial::Calculated: return "calculated";
        case Initial::None:       return "none";
    }
    return "unknown";
}

/// @brief Returns the FMI type name of a data type (e.g. `"Real"`, `"Float64"`, `"UInt32"`).
/// @req FR-02.2
[[nodiscard]] constexpr std::string_view to_string(DataType dt) noexcept {
    switch (dt) {
        case DataType::Real:    return "Real";
        case DataType::Integer: return "Integer";
        case DataType::Boolean: return "Boolean";
        case DataType::String:  return "String";
        case DataType::Float64: return "Float64";
        case DataType::Float32: return "Float32";
        case DataType::Int8:    return "Int8";
        case DataType::Int16:   return "Int16";
        case DataType::Int32:   return "Int32";
        case DataType::Int64:   return "Int64";
        case DataType::UInt8:   return "UInt8";
        case DataType::UInt16:  return "UInt16";
        case DataType::UInt32:  return "UInt32";
        case DataType::UInt64:  return "UInt64";
        case DataType::Binary:  return "Binary";
        case DataType::Clock:   return "Clock";
    }
    return "unknown";
}

/// @brief Reports whether a data type exists only in FMI 3.0 (not representable in FMI 2.0).
/// @param dt Data type to classify.
/// @returns `true` for FMI 3.0-only types (Float32/64, sized ints, Binary, Clock).
/// @req FR-02.2
[[nodiscard]] constexpr bool isFmi3Only(DataType dt) noexcept {
    switch (dt) {
        case DataType::Float64: case DataType::Float32:
        case DataType::Int8:    case DataType::Int16:
        case DataType::Int32:   case DataType::Int64:
        case DataType::UInt8:   case DataType::UInt16:
        case DataType::UInt32:  case DataType::UInt64:
        case DataType::Binary:  case DataType::Clock:
            return true;
        default:
            return false;
    }
}

} // namespace libfmu_toolkit
