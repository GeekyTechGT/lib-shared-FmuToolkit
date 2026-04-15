#pragma once

// ---------------------------------------------------------------------------
// FmuVariableTypes.hpp — internal header
//
// The canonical enum definitions (FmiVersion, Causality, Variability,
// Initial, DataType) live in the public API header <libfmu_toolkit/types.hpp>.
// This header re-exports them plus adds internal helpers (to_string, isFmi3Only)
// that are NOT part of the public API.
//
// Including this file from internal code gives both the enums and the helpers.
// ---------------------------------------------------------------------------

#include <libfmu_toolkit/types.hpp>   // FmiVersion, Causality, Variability, Initial, DataType

#include <string_view>

namespace libfmu_toolkit {

// ---------------------------------------------------------------------------
// constexpr helpers — human-readable names (internal use only)
// ---------------------------------------------------------------------------

[[nodiscard]] constexpr std::string_view to_string(FmiVersion v) noexcept {
    switch (v) {
        case FmiVersion::FMI2: return "FMI2";
        case FmiVersion::FMI3: return "FMI3";
    }
    return "Unknown";
}

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

[[nodiscard]] constexpr std::string_view to_string(Initial i) noexcept {
    switch (i) {
        case Initial::Exact:      return "exact";
        case Initial::Approx:     return "approx";
        case Initial::Calculated: return "calculated";
        case Initial::None:       return "none";
    }
    return "unknown";
}

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
