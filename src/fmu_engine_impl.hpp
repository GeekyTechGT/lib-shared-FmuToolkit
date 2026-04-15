#pragma once

#include "core/domain/fmu_package.hpp"
#include "core/domain/simulation_config.hpp"
#include "core/domain/simulation_result.hpp"
#include "core/domain/validation_report.hpp"
#include "core/usecases/fmu_importer.hpp"
#include "core/usecases/fmu_validator.hpp"
#include "core/usecases/simulation_controller.hpp"
#include "core/usecases/csv_result_exporter.hpp"
#include "core/exceptions/fmu_exception.hpp"
#include "adapters/xml/libxml2_validator_adapter.hpp"
#include "adapters/zip/zip_loader_adapter.hpp"
#include "adapters/simulation/standalone_simulation_adapter.hpp"
#include "adapters/json/json_result_exporter.hpp"

#include <libfmu_toolkit/exceptions.hpp>

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace libfmu_toolkit {

namespace detail {

inline std::string startValueToStr(const StartValue& sv) {
    return std::visit([](auto&& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, double>)      return std::to_string(arg);
        if constexpr (std::is_same_v<T, int64_t>)     return std::to_string(arg);
        if constexpr (std::is_same_v<T, bool>)        return arg ? "true" : "false";
        if constexpr (std::is_same_v<T, std::string>) return arg;
    }, sv);
}

inline VariableInfo toPublicVar(const ModelVariable& mv)
{
    VariableInfo vi;
    vi.name           = mv.name;
    vi.description    = mv.description;
    vi.valueReference = mv.valueReference;
    vi.causality      = mv.causality;
    vi.variability    = mv.variability;
    vi.dataType       = mv.dataType;
    vi.unit           = mv.unit;
    vi.typeName       = mv.typeName;
    vi.hasStartValue  = mv.startValue.has_value();
    if (mv.startValue.has_value()) {
        vi.startValueStr = startValueToStr(*mv.startValue);
    }
    return vi;
}

inline FmuInfo toPublicInfo(const FmuPackage& pkg)
{
    FmuInfo info;
    info.modelName               = pkg.modelName;
    info.modelIdentifier         = pkg.modelIdentifier;
    info.guid                    = pkg.guid;
    info.description             = pkg.description;
    info.generationTool          = pkg.generationTool;
    info.generationDateAndTime   = pkg.generationDateAndTime;
    info.fmiVersion              = pkg.fmiVersion;
    info.numberOfEventIndicators = pkg.numberOfEventIndicators;

    info.variables.reserve(pkg.variables.size());
    for (const auto& mv : pkg.variables)
        info.variables.push_back(toPublicVar(mv));

    // UnitDefinitions
    info.unitDefinitions.reserve(pkg.unitDefinitions.size());
    for (const auto& ud : pkg.unitDefinitions) {
        UnitInfo ui;
        ui.name   = ud.name;
        ui.factor = ud.factor;
        ui.offset = ud.offset;
        info.unitDefinitions.push_back(std::move(ui));
    }

    // TypeDefinitions
    info.typeDefinitions.reserve(pkg.typeDefinitions.size());
    for (const auto& td : pkg.typeDefinitions) {
        TypeDefinitionInfo ti;
        ti.name        = td.name;
        ti.description = td.description;
        ti.dataType    = td.dataType;
        ti.unit        = td.unit;
        ti.quantity    = td.quantity;
        ti.displayUnit = td.displayUnit;
        ti.minStr      = td.minStr;
        ti.maxStr      = td.maxStr;
        info.typeDefinitions.push_back(std::move(ti));
    }

    // DefaultExperiment
    const auto& de = pkg.defaultExperiment;
    info.defaultExperiment.startTime    = de.startTime;
    info.defaultExperiment.stopTime     = de.stopTime;
    info.defaultExperiment.stepSize     = de.stepSize;
    info.defaultExperiment.tolerance    = de.tolerance;
    info.defaultExperiment.hasStartTime = de.hasStartTime;
    info.defaultExperiment.hasStopTime  = de.hasStopTime;
    info.defaultExperiment.hasStepSize  = de.hasStepSize;
    info.defaultExperiment.hasTolerance = de.hasTolerance;

    return info;
}

inline ValidationResult toPublicReport(const ValidationReport& vr)
{
    ValidationResult result;
    result.passed = vr.passed;
    for (const auto& e : vr.errors) {
        // Copy category, message, line, and column from the internal error entry.
        result.errors.push_back(ValidationIssue{e.category, e.message, e.line, e.column});
    }
    for (const auto& w : vr.warnings) {
        // Warnings have no line/column; default to -1.
        result.warnings.push_back(ValidationIssue{w.category, w.message});
    }
    return result;
}

inline SimulationConfig toInternalConfig(const SimConfig& pub)
{
    SimulationConfig cfg;
    cfg.startTime         = pub.startTime;
    cfg.stopTime          = pub.stopTime;
    cfg.stepSize          = pub.stepSize;
    cfg.tolerance         = pub.tolerance;
    cfg.recordingInterval = pub.recordingInterval;
    cfg.outputVariables   = pub.outputVariables;
    cfg.initialParameters = pub.initialParameters;
    return cfg;
}

inline SimResult toPublicResult(const SimulationResult& sr)
{
    SimResult result;
    result.timePoints = sr.timePoints;
    result.series     = sr.series;
    return result;
}

inline bool schemaTreeExists(const std::filesystem::path& basePath)
{
    return std::filesystem::exists(basePath / "fmi-standard-2.0" / "schema" / "fmi2ModelDescription.xsd")
        && std::filesystem::exists(basePath / "fmi-standard-3.0" / "schema" / "fmi3ModelDescription.xsd");
}

inline std::optional<std::filesystem::path> pathFromEnvironment(const char* name)
{
    const char* value = std::getenv(name);
    if (!value || !*value) {
        return std::nullopt;
    }
    return std::filesystem::path(value);
}

inline std::optional<std::filesystem::path> moduleDirectory()
{
#if defined(_WIN32)
    HMODULE module = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&moduleDirectory),
            &module)) {
        return std::nullopt;
    }

    std::string buffer(MAX_PATH, static_cast<char>(0));
    DWORD length = 0;
    for (;;) {
        length = GetModuleFileNameA(module, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return std::nullopt;
        }
        if (length < buffer.size()) {
            break;
        }
        buffer.resize(buffer.size() * 2U);
    }
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
#else
    Dl_info info{};
    if (dladdr(reinterpret_cast<void*>(&moduleDirectory), &info) == 0 || info.dli_fname == nullptr) {
        return std::nullopt;
    }
    return std::filesystem::path(info.dli_fname).parent_path();
#endif
}

inline std::filesystem::path resolveDefaultSchemaBasePath()
{
    std::vector<std::filesystem::path> candidates;

    if (auto envPath = pathFromEnvironment("LIB_FMU_TOOLKIT_XSD_BASE")) {
        candidates.push_back(*envPath);
    }

#ifdef LIB_FMU_TOOLKIT_SOURCE_EXTERNAL_DIR
    candidates.emplace_back(LIB_FMU_TOOLKIT_SOURCE_EXTERNAL_DIR);
#endif

    if (auto moduleDir = moduleDirectory()) {
        candidates.push_back(*moduleDir / ".." / "share" / "lib-fmu-toolkit" / "schemas");
        candidates.push_back(*moduleDir / "schemas");
    }

    candidates.push_back(std::filesystem::current_path() / "external");

    for (const auto& candidate : candidates) {
        if (schemaTreeExists(candidate)) {
            return std::filesystem::weakly_canonical(candidate);
        }
    }

    return std::filesystem::current_path() / "external";
}

} // namespace detail

// ---------------------------------------------------------------------------
// FmuSession::Impl
//
// Uses ISimulationBackend (port abstraction) instead of a concrete adapter so
// that an alternative backend can be injected in tests or custom integrations.
// The default constructor wires the production StandaloneSimulationAdapter.
// ---------------------------------------------------------------------------
struct FmuSession::Impl {
    std::unique_ptr<ISimulationBackend> backend;
    FmuInfo                             info;
    // std::less<> (transparent comparator) allows map::find() to accept
    // std::string_view directly without constructing a std::string key.
    std::map<std::string, std::pair<uint32_t, DataType>, std::less<>> varIndex;
    bool active{false};

    // Default — production wiring
    Impl() : backend(std::make_unique<StandaloneSimulationAdapter>()) {}

    // Injection constructor — used in tests / custom integrations
    explicit Impl(std::unique_ptr<ISimulationBackend> b)
        : backend(std::move(b)) {}

    Impl(const Impl&)            = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&)                 = delete;
    Impl& operator=(Impl&&)      = delete;
};

// ---------------------------------------------------------------------------
// FmuEngine::Impl
//
// Uses IXmlValidator (port abstraction) so the validator can be replaced
// without recompiling clients.  The default wires Libxml2ValidatorAdapter.
// ZipLoaderAdapter is created per-load so each FMU gets its own extraction
// directory and OS library handle lifetime.
// ---------------------------------------------------------------------------
struct FmuEngine::Impl {
    std::filesystem::path         xsdBasePath;
    std::unique_ptr<IXmlValidator> xmlValidator;
    uint64_t                       nextHandleId{1};

    struct Entry {
        std::unique_ptr<IFmuLoader> loader;
        FmuPackage pkg;

        Entry() = default;
        Entry(std::unique_ptr<IFmuLoader> l, FmuPackage&& p)
            : loader(std::move(l)), pkg(std::move(p)) {}

        Entry(Entry&&)            = default;
        Entry& operator=(Entry&&) = default;
    };

    std::map<uint64_t, Entry> entries;

    // Default constructor — production wiring.
    // std::string_view avoids a copy when called with a string literal or
    // std::string_view; the path is immediately converted to filesystem::path.
    explicit Impl(std::string_view xsdBase)
        : xsdBasePath(xsdBase.empty()
                      ? detail::resolveDefaultSchemaBasePath()
                      : std::filesystem::path(xsdBase))
        , xmlValidator(std::make_unique<Libxml2ValidatorAdapter>())
    {}

    // Injection constructor — used in tests / custom integrations.
    Impl(std::string_view xsdBase, std::unique_ptr<IXmlValidator> validator)
        : xsdBasePath(xsdBase.empty()
                      ? detail::resolveDefaultSchemaBasePath()
                      : std::filesystem::path(xsdBase))
        , xmlValidator(std::move(validator))
    {}

    [[nodiscard]] std::filesystem::path fmi2SchemaDir() const {
        return xsdBasePath / "fmi-standard-2.0" / "schema";
    }

    [[nodiscard]] std::filesystem::path fmi3SchemaDir() const {
        return xsdBasePath / "fmi-standard-3.0" / "schema";
    }

    Entry& getEntry(uint64_t handle) {
        auto it = entries.find(handle);
        if (it == entries.end())
            throw std::out_of_range("FmuEngine: unknown handle " + std::to_string(handle));
        return it->second;
    }

    const Entry& getEntry(uint64_t handle) const {
        auto it = entries.find(handle);
        if (it == entries.end())
            throw std::out_of_range("FmuEngine: unknown handle " + std::to_string(handle));
        return it->second;
    }
};

} // namespace libfmu_toolkit
