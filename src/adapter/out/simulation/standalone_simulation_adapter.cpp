/// @file standalone_simulation_adapter.cpp
/// @brief Implementation of the in-process FMI co-simulation backend (FR-04, FR-05).
/// @internal

#include "standalone_simulation_adapter.hpp"
#include "domain/exception/fmu_exception.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#  include <windows.h>
#endif

namespace libfmu_toolkit {

namespace {

// Converts an extracted FMU path to a file:/// URI understood by FMI binaries.
std::string buildResourceUri(const std::filesystem::path& extractedPath) {
    std::string p = extractedPath.string();
    std::replace(p.begin(), p.end(), '\\', '/');
    return "file:///" + p;
}

// Builds a name → ModelVariable* map for O(1) lookups when applying parameters.
std::unordered_map<std::string, const ModelVariable*>
buildVarIndex(const std::vector<ModelVariable>& variables) {
    std::unordered_map<std::string, const ModelVariable*> idx;
    idx.reserve(variables.size());
    for (const auto& v : variables)
        idx[v.name] = &v;
    return idx;
}

} // anonymous namespace

// =============================================================================
// FMI 2.0 local type definitions
// =============================================================================

using fmi2Real           = double;
using fmi2Integer        = int;
using fmi2Boolean        = int;
using fmi2String         = const char*;
using fmi2Component      = void*;
using fmi2ValueReference = unsigned int;
using fmi2Status         = int;

constexpr fmi2Status fmi2OK      = 0;
constexpr fmi2Status fmi2Warning = 1;
constexpr fmi2Status fmi2Discard = 2;
constexpr fmi2Status fmi2Error   = 3;
constexpr fmi2Status fmi2Fatal   = 4;

constexpr int fmi2CoSimulation = 1;

using fmi2InstantiateFn      = fmi2Component(*)(fmi2String, int, fmi2String, fmi2String,
                                                 const void*, int, int);
using fmi2SetupExperimentFn  = fmi2Status(*)(fmi2Component, int, fmi2Real,
                                              fmi2Real, int, fmi2Real);
using fmi2EnterInitFn        = fmi2Status(*)(fmi2Component);
using fmi2ExitInitFn         = fmi2Status(*)(fmi2Component);
using fmi2DoStepFn           = fmi2Status(*)(fmi2Component, fmi2Real, fmi2Real, int);
using fmi2TerminateFn        = fmi2Status(*)(fmi2Component);
using fmi2FreeInstanceFn     = void(*)(fmi2Component);
using fmi2GetRealFn          = fmi2Status(*)(fmi2Component, const fmi2ValueReference*, size_t, fmi2Real*);
using fmi2SetRealFn          = fmi2Status(*)(fmi2Component, const fmi2ValueReference*, size_t, const fmi2Real*);
using fmi2GetIntegerFn       = fmi2Status(*)(fmi2Component, const fmi2ValueReference*, size_t, fmi2Integer*);
using fmi2SetIntegerFn       = fmi2Status(*)(fmi2Component, const fmi2ValueReference*, size_t, const fmi2Integer*);
using fmi2GetBooleanFn       = fmi2Status(*)(fmi2Component, const fmi2ValueReference*, size_t, fmi2Boolean*);
using fmi2SetBooleanFn       = fmi2Status(*)(fmi2Component, const fmi2ValueReference*, size_t, const fmi2Boolean*);
using fmi2GetStringFn        = fmi2Status(*)(fmi2Component, const fmi2ValueReference*, size_t, fmi2String*);
using fmi2SetStringFn        = fmi2Status(*)(fmi2Component, const fmi2ValueReference*, size_t, const fmi2String*);

using fmi2ComponentEnvironment = void*;
using fmi2LoggerFn       = void(*)(fmi2ComponentEnvironment, fmi2String, fmi2Status, fmi2String, fmi2String, ...);
using fmi2AllocMemFn     = void*(*)(size_t, size_t);
using fmi2FreeMemFn      = void(*)(void*);
using fmi2StepFinishedFn = void(*)(fmi2ComponentEnvironment, fmi2Status);

struct Fmi2Callbacks {
    fmi2LoggerFn             logger;
    fmi2AllocMemFn           allocateMemory;
    fmi2FreeMemFn            freeMemory;
    fmi2StepFinishedFn       stepFinished;
    fmi2ComponentEnvironment componentEnvironment;
};

static void fmi2_noop_logger(fmi2ComponentEnvironment, fmi2String, fmi2Status,
                              fmi2String, fmi2String, ...) noexcept {}

#ifdef _WIN32
static void* fmi2_heap_alloc(size_t count, size_t size) noexcept {
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, count * size);
}
static void fmi2_heap_free(void* ptr) noexcept {
    if (ptr) HeapFree(GetProcessHeap(), 0, ptr);
}
#else
static void* fmi2_heap_alloc(size_t count, size_t size) noexcept { return std::calloc(count, size); }
static void fmi2_heap_free(void* ptr) noexcept { std::free(ptr); }
#endif

static const Fmi2Callbacks kFmi2Callbacks {
    fmi2_noop_logger,
    fmi2_heap_alloc,
    fmi2_heap_free,
    nullptr,
    nullptr
};

// =============================================================================
// FMI 3.0 local type definitions
// =============================================================================

using fmi3Instance       = void*;
using fmi3Float64        = double;
using fmi3Float32        = float;
using fmi3Int8           = int8_t;
using fmi3Int16          = int16_t;
using fmi3Int32          = int32_t;
using fmi3Int64          = int64_t;
using fmi3UInt8          = uint8_t;
using fmi3UInt16         = uint16_t;
using fmi3UInt32         = uint32_t;
using fmi3UInt64         = uint64_t;
using fmi3Boolean        = bool;   // fmi3Boolean is C99 bool in fmi3PlatformTypes.h
using fmi3Clock          = bool;
using fmi3Binary         = const uint8_t*;
using fmi3IntervalQualifier = int;  // enum, int-sized
using fmi3String         = const char*;
using fmi3ValueReference = uint32_t;
using fmi3Status         = int;

constexpr fmi3Status fmi3OK      = 0;
constexpr fmi3Status fmi3Warning = 1;
constexpr fmi3Status fmi3Discard = 2;
constexpr fmi3Status fmi3Error   = 3;
constexpr fmi3Status fmi3Fatal   = 4;

using fmi3LogMessageCallbackFn = void(*)(void*, fmi3Status, fmi3String, fmi3String);

using fmi3IntermediateUpdateCallbackFn = void*;  // never used: the importer does not provide intermediate updates

// 12 parameters (fmi3FunctionTypes.h): name, token, resourcePath, visible, loggingOn, eventModeUsed, earlyReturnAllowed,
// requiredIntermediateVariables, nRequiredIntermediateVariables, instanceEnvironment, logMessage, intermediateUpdate.
using fmi3InstantiateCoSimulationFn = fmi3Instance(*)(
    fmi3String, fmi3String, fmi3String, fmi3Boolean, fmi3Boolean, fmi3Boolean, fmi3Boolean,
    const fmi3ValueReference*, size_t,
    void*, fmi3LogMessageCallbackFn, fmi3IntermediateUpdateCallbackFn
);
using fmi3EnterInitializationModeFn = fmi3Status(*)(fmi3Instance, fmi3Boolean, fmi3Float64,
                                                     fmi3Float64, fmi3Boolean, fmi3Float64);
using fmi3EnterModeFn               = fmi3Status(*)(fmi3Instance);
using fmi3UpdateDiscreteStatesFn    = fmi3Status(*)(fmi3Instance, fmi3Boolean*, fmi3Boolean*, fmi3Boolean*, fmi3Boolean*,
                                                     fmi3Boolean*, fmi3Float64*);
using fmi3GetBinaryFn               = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, size_t*, fmi3Binary*, size_t);
using fmi3SetBinaryFn               = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const size_t*, const fmi3Binary*, size_t);
using fmi3GetClockFn                = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, fmi3Clock*);
using fmi3SetClockFn                = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const fmi3Clock*);
using fmi3GetIntervalDecimalFn      = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, fmi3Float64*, fmi3IntervalQualifier*);
using fmi3ExitInitializationModeFn  = fmi3Status(*)(fmi3Instance);
using fmi3DoStepFn                  = fmi3Status(*)(fmi3Instance, fmi3Float64, fmi3Float64,
                                                     fmi3Boolean, fmi3Boolean*, fmi3Boolean*, fmi3Boolean*, fmi3Float64*);
using fmi3TerminateFn               = fmi3Status(*)(fmi3Instance);
using fmi3FreeInstanceFn            = void(*)(fmi3Instance);
using fmi3GetFloat64Fn = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, fmi3Float64*, size_t);
using fmi3SetFloat64Fn = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const fmi3Float64*, size_t);
using fmi3GetFloat32Fn = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, fmi3Float32*, size_t);
using fmi3SetFloat32Fn = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const fmi3Float32*, size_t);
using fmi3GetInt32Fn   = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, fmi3Int32*, size_t);
using fmi3SetInt32Fn   = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const fmi3Int32*, size_t);
using fmi3GetInt64Fn   = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, fmi3Int64*, size_t);
using fmi3SetInt64Fn   = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const fmi3Int64*, size_t);
using fmi3GetBooleanFn = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, fmi3Boolean*, size_t);
using fmi3SetBooleanFn = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const fmi3Boolean*, size_t);
using fmi3GetStringFn  = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, fmi3String*, size_t);
using fmi3SetStringFn  = fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const fmi3String*, size_t);

// The sized integer families (Int8..UInt64) share one shape; the value travels as int64_t like every integer of this API.
template <typename T>
/// @brief Reads one variable of a sized integer family (Int8 to UInt64) and widens it to int64_t.
/// @tparam T The FMI 3.0 C type of the family. @param fn The resolved fmi3Get<T> function (may be null: the value stays 0).
FmiValue getIntegral(void* fn, void* component, fmi3ValueReference vr) {
    T value{};
    const auto get = reinterpret_cast<fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, T*, size_t)>(fn);
    if (get) get(static_cast<fmi3Instance>(component), &vr, 1, &value, 1);
    return FmiValue{static_cast<int64_t>(value)};
}

template <typename T>
/// @brief Writes one variable of a sized integer family, narrowing the int64_t to its C type.
/// @tparam T The FMI 3.0 C type of the family. @param fn The resolved fmi3Set<T> function (may be null: nothing is written).
void setIntegral(void* fn, void* component, fmi3ValueReference vr, const FmiValue& v) {
    const T value = static_cast<T>(std::get<int64_t>(v));
    const auto set = reinterpret_cast<fmi3Status(*)(fmi3Instance, const fmi3ValueReference*, size_t, const T*, size_t)>(fn);
    if (set) set(static_cast<fmi3Instance>(component), &vr, 1, &value, 1);
}

// Receives the FMU's own messages: errors and warnings go to stderr, the rest is dropped.
static void fmi3_logger(void*, fmi3Status status, fmi3String category, fmi3String message) noexcept {
    if (status >= fmi3Warning && message)
        std::fprintf(stderr, "[fmu %s] %s\n", category ? category : "", message);
}

// =============================================================================
// Destructor
// =============================================================================

StandaloneSimulationAdapter::~StandaloneSimulationAdapter() {
    if (initialized_) {
        try { terminate(); } catch (...) {}
    }
}

// =============================================================================
// Public interface
// =============================================================================

void StandaloneSimulationAdapter::initialize(const FmuPackage& pkg,
                                              const SimulationConfig& config) {
    if (initialized_) terminate();
    pkg_ = &pkg;
    if (pkg.isFmi2()) {
        if (!pkg.fmi2.has_value())
            throw FmuSimulationException("FmuPackage reports FMI2 but fmi2 table is absent");
        initFmi2(pkg, config);
    } else if (pkg.isFmi3()) {
        if (!pkg.fmi3.has_value())
            throw FmuSimulationException("FmuPackage reports FMI3 but fmi3 table is absent");
        initFmi3(pkg, config);
    } else {
        throw FmuSimulationException("Unsupported FMI version");
    }
}

void StandaloneSimulationAdapter::setInput(uint32_t valueRef, DataType dtype,
                                            const FmiValue& value) {
    if (!initialized_)
        throw FmuSimulationException("setInput called before initialize");
    if (pkg_->isFmi2())
        setInputFmi2(valueRef, dtype, value);
    else
        setInputFmi3(valueRef, dtype, value);
}

FmiValue StandaloneSimulationAdapter::getOutput(uint32_t valueRef, DataType dtype) {
    if (!initialized_)
        throw FmuSimulationException("getOutput called before initialize");
    if (pkg_->isFmi2())
        return getOutputFmi2(valueRef, dtype);
    else
        return getOutputFmi3(valueRef, dtype);
}

bool StandaloneSimulationAdapter::doStep(double currentTime, double stepSize) {
    if (!initialized_)
        throw FmuSimulationException("doStep called before initialize");
    if (pkg_->isFmi2())
        return doStepFmi2(currentTime, stepSize);
    else
        return doStepFmi3(currentTime, stepSize);
}

void StandaloneSimulationAdapter::terminate() {
    if (!initialized_) return;
    if (pkg_ && pkg_->isFmi2())
        terminateFmi2();
    else if (pkg_)
        terminateFmi3();
    initialized_  = false;
    fmiComponent_ = nullptr;
    pkg_          = nullptr;
}

// =============================================================================
// FMI 2.0 implementation
// =============================================================================

void StandaloneSimulationAdapter::initFmi2(const FmuPackage& pkg,
                                            const SimulationConfig& config) {
    auto& fns = *pkg.fmi2;
    auto instantiate = reinterpret_cast<fmi2InstantiateFn>(fns.fmi2Instantiate);
    if (!instantiate)
        throw FmuSimulationException("fmi2Instantiate function pointer is null");

    const std::string resourceUri = buildResourceUri(pkg.extractedPath);

    fmiComponent_ = instantiate(
        pkg.modelName.c_str(), fmi2CoSimulation, pkg.guid.c_str(),
        resourceUri.c_str(), &kFmi2Callbacks, 0, 0);

    if (!fmiComponent_)
        throw FmuSimulationException("fmi2Instantiate returned null");

    auto setupExp = reinterpret_cast<fmi2SetupExperimentFn>(fns.fmi2SetupExperiment);
    if (!setupExp)
        throw FmuSimulationException("fmi2SetupExperiment function pointer is null");
    fmi2Status status = setupExp(static_cast<fmi2Component>(fmiComponent_),
                                  0, config.tolerance, config.startTime, 1, config.stopTime);
    if (status >= fmi2Error)
        throw FmuSimulationException("fmi2SetupExperiment failed with status " + std::to_string(status));

    auto enterInit = reinterpret_cast<fmi2EnterInitFn>(fns.fmi2EnterInitializationMode);
    if (!enterInit)
        throw FmuSimulationException("fmi2EnterInitializationMode function pointer is null");
    status = enterInit(static_cast<fmi2Component>(fmiComponent_));
    if (status >= fmi2Error)
        throw FmuSimulationException("fmi2EnterInitializationMode failed with status " + std::to_string(status));

    const auto varIndex = buildVarIndex(pkg.variables);
    for (const auto& [name, paramVal] : config.initialParameters) {
        auto it = varIndex.find(name);
        if (it != varIndex.end() &&
            (it->second->causality == Causality::Parameter ||
             it->second->causality == Causality::CalculatedParameter)) {
            setInputFmi2(it->second->valueReference, it->second->dataType,
                std::visit([](auto&& v) -> FmiValue { return FmiValue{v}; }, paramVal));
        }
    }

    auto exitInit = reinterpret_cast<fmi2ExitInitFn>(fns.fmi2ExitInitializationMode);
    if (!exitInit)
        throw FmuSimulationException("fmi2ExitInitializationMode function pointer is null");
    status = exitInit(static_cast<fmi2Component>(fmiComponent_));
    if (status >= fmi2Error)
        throw FmuSimulationException("fmi2ExitInitializationMode failed with status " + std::to_string(status));

    initialized_ = true;
}

void StandaloneSimulationAdapter::setInputFmi2(uint32_t vr, DataType dtype, const FmiValue& value) {
    auto& fns = *pkg_->fmi2;
    fmi2ValueReference vrRef = static_cast<fmi2ValueReference>(vr);

    switch (dtype) {
        case DataType::Real: case DataType::Float64: case DataType::Float32: {
            fmi2Real val = std::get<double>(value);
            auto fn = reinterpret_cast<fmi2SetRealFn>(fns.fmi2SetReal);
            if (fn) fn(static_cast<fmi2Component>(fmiComponent_), &vrRef, 1, &val);
            break;
        }
        case DataType::Integer: case DataType::Int8: case DataType::Int16:
        case DataType::Int32:   case DataType::Int64: {
            fmi2Integer val = static_cast<fmi2Integer>(std::get<int64_t>(value));
            auto fn = reinterpret_cast<fmi2SetIntegerFn>(fns.fmi2SetInteger);
            if (fn) fn(static_cast<fmi2Component>(fmiComponent_), &vrRef, 1, &val);
            break;
        }
        case DataType::Boolean: {
            fmi2Boolean val = std::get<bool>(value) ? 1 : 0;
            auto fn = reinterpret_cast<fmi2SetBooleanFn>(fns.fmi2SetBoolean);
            if (fn) fn(static_cast<fmi2Component>(fmiComponent_), &vrRef, 1, &val);
            break;
        }
        case DataType::String: {
            fmi2String val = std::get<std::string>(value).c_str();
            auto fn = reinterpret_cast<fmi2SetStringFn>(fns.fmi2SetString);
            if (fn) fn(static_cast<fmi2Component>(fmiComponent_), &vrRef, 1, &val);
            break;
        }
        default: break;
    }
}

FmiValue StandaloneSimulationAdapter::getOutputFmi2(uint32_t vr, DataType dtype) {
    auto& fns = *pkg_->fmi2;
    fmi2ValueReference vrRef = static_cast<fmi2ValueReference>(vr);

    switch (dtype) {
        case DataType::Real: case DataType::Float64: case DataType::Float32: {
            fmi2Real val = 0.0;
            auto fn = reinterpret_cast<fmi2GetRealFn>(fns.fmi2GetReal);
            if (fn) fn(static_cast<fmi2Component>(fmiComponent_), &vrRef, 1, &val);
            return FmiValue{static_cast<double>(val)};
        }
        case DataType::Integer: case DataType::Int8: case DataType::Int16:
        case DataType::Int32:   case DataType::Int64: {
            fmi2Integer val = 0;
            auto fn = reinterpret_cast<fmi2GetIntegerFn>(fns.fmi2GetInteger);
            if (fn) fn(static_cast<fmi2Component>(fmiComponent_), &vrRef, 1, &val);
            return FmiValue{static_cast<int64_t>(val)};
        }
        case DataType::Boolean: {
            fmi2Boolean val = 0;
            auto fn = reinterpret_cast<fmi2GetBooleanFn>(fns.fmi2GetBoolean);
            if (fn) fn(static_cast<fmi2Component>(fmiComponent_), &vrRef, 1, &val);
            return FmiValue{val != 0};
        }
        case DataType::String: {
            fmi2String val = nullptr;
            auto fn = reinterpret_cast<fmi2GetStringFn>(fns.fmi2GetString);
            if (fn) fn(static_cast<fmi2Component>(fmiComponent_), &vrRef, 1, &val);
            return FmiValue{val ? std::string(val) : std::string{}};
        }
        default: return FmiValue{0.0};
    }
}

bool StandaloneSimulationAdapter::doStepFmi2(double t, double dt) {
    auto& fns = *pkg_->fmi2;
    auto fn = reinterpret_cast<fmi2DoStepFn>(fns.fmi2DoStep);
    if (!fn)
        throw FmuSimulationException("fmi2DoStep function pointer is null");

    fmi2Status status   = fmi2Fatal;
    bool       seh_hit  = false;

#if defined(_WIN32) && defined(_MSC_VER)
    __try {
        status = fn(static_cast<fmi2Component>(fmiComponent_),
                    static_cast<fmi2Real>(t), static_cast<fmi2Real>(dt), 1);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        seh_hit = true;
    }
#else
    status = fn(static_cast<fmi2Component>(fmiComponent_),
                static_cast<fmi2Real>(t), static_cast<fmi2Real>(dt), 1);
#endif

    if (seh_hit)
        throw FmuSimulationException("fmi2DoStep raised an SEH exception inside the FMU DLL");
    if (status == fmi2Discard) return false;
    if (status >= fmi2Error)
        throw FmuSimulationException("fmi2DoStep failed with status " + std::to_string(status));
    return true;
}

void StandaloneSimulationAdapter::terminateFmi2() {
    auto& fns = *pkg_->fmi2;
    auto terminateFn = reinterpret_cast<fmi2TerminateFn>(fns.fmi2Terminate);
    if (terminateFn) terminateFn(static_cast<fmi2Component>(fmiComponent_));
    auto freeInst = reinterpret_cast<fmi2FreeInstanceFn>(fns.fmi2FreeInstance);
    if (freeInst)   freeInst(static_cast<fmi2Component>(fmiComponent_));
    fmiComponent_ = nullptr;
}

// =============================================================================
// FMI 3.0 implementation
// =============================================================================

void StandaloneSimulationAdapter::initFmi3(const FmuPackage& pkg,
                                            const SimulationConfig& config) {
    auto& fns = *pkg.fmi3;
    auto instantiate = reinterpret_cast<fmi3InstantiateCoSimulationFn>(fns.fmi3InstantiateCoSimulation);
    if (!instantiate)
        throw FmuSimulationException("fmi3InstantiateCoSimulation function pointer is null");

    const std::string resourceUri3 = buildResourceUri(pkg.extractedPath);

    eventModeUsed_ = config.eventModeUsed;
    fmiComponent_ = instantiate(
        pkg.modelName.c_str(), pkg.guid.c_str(), resourceUri3.c_str(),
        false, false, config.eventModeUsed, config.earlyReturnAllowed, nullptr, 0, nullptr, fmi3_logger, nullptr);

    if (!fmiComponent_)
        throw FmuSimulationException("fmi3InstantiateCoSimulation returned null");

    auto enterInit = reinterpret_cast<fmi3EnterInitializationModeFn>(fns.fmi3EnterInitializationMode);
    if (!enterInit)
        throw FmuSimulationException("fmi3EnterInitializationMode function pointer is null");
    fmi3Status status = enterInit(static_cast<fmi3Instance>(fmiComponent_),
                                   false, config.tolerance, config.startTime, true, config.stopTime);
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3EnterInitializationMode failed with status " + std::to_string(status));

    const auto varIndex3 = buildVarIndex(pkg.variables);
    for (const auto& [name, paramVal] : config.initialParameters) {
        auto it = varIndex3.find(name);
        if (it != varIndex3.end() &&
            (it->second->causality == Causality::Parameter ||
             it->second->causality == Causality::CalculatedParameter)) {
            setInputFmi3(it->second->valueReference, it->second->dataType,
                std::visit([](auto&& v) -> FmiValue { return FmiValue{v}; }, paramVal));
        }
    }

    auto exitInit = reinterpret_cast<fmi3ExitInitializationModeFn>(fns.fmi3ExitInitializationMode);
    if (!exitInit)
        throw FmuSimulationException("fmi3ExitInitializationMode function pointer is null");
    status = exitInit(static_cast<fmi3Instance>(fmiComponent_));
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3ExitInitializationMode failed with status " + std::to_string(status));

    initialized_ = true;
    if (config.eventModeUsed) {
        // After Initialization Mode the instance is in Event Mode: settle the discrete states, then step.
        constexpr int kMaxUpdates = 1000;
        for (int i = 0;; ++i) {
            if (i == kMaxUpdates)
                throw FmuSimulationException("fmi3UpdateDiscreteStates still asks for an update after " + std::to_string(kMaxUpdates) + " calls");
            if (!updateDiscreteStates().discreteStatesNeedUpdate) break;
        }
        enterStepMode();
    }
}

void StandaloneSimulationAdapter::setInputFmi3(uint32_t vr, DataType dtype, const FmiValue& value) {
    auto& fns = *pkg_->fmi3;
    fmi3ValueReference vrRef = static_cast<fmi3ValueReference>(vr);

    switch (dtype) {
        case DataType::Real: case DataType::Float64: {
            fmi3Float64 val = std::get<double>(value);
            auto fn = reinterpret_cast<fmi3SetFloat64Fn>(fns.fmi3SetFloat64);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            break;
        }
        case DataType::Float32: {
            fmi3Float32 val = static_cast<fmi3Float32>(std::get<double>(value));
            auto fn = reinterpret_cast<fmi3SetFloat32Fn>(fns.fmi3SetFloat32);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            break;
        }
        case DataType::Integer: case DataType::Int32: {
            fmi3Int32 val = static_cast<fmi3Int32>(std::get<int64_t>(value));
            auto fn = reinterpret_cast<fmi3SetInt32Fn>(fns.fmi3SetInt32);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            break;
        }
        case DataType::Int64: {
            fmi3Int64 val = std::get<int64_t>(value);
            auto fn = reinterpret_cast<fmi3SetInt64Fn>(fns.fmi3SetInt64);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            break;
        }
        case DataType::Int8:   setIntegral<fmi3Int8>(fns.fmi3SetInt8, fmiComponent_, vrRef, value); break;
        case DataType::Int16:  setIntegral<fmi3Int16>(fns.fmi3SetInt16, fmiComponent_, vrRef, value); break;
        case DataType::UInt8:  setIntegral<fmi3UInt8>(fns.fmi3SetUInt8, fmiComponent_, vrRef, value); break;
        case DataType::UInt16: setIntegral<fmi3UInt16>(fns.fmi3SetUInt16, fmiComponent_, vrRef, value); break;
        case DataType::UInt32: setIntegral<fmi3UInt32>(fns.fmi3SetUInt32, fmiComponent_, vrRef, value); break;
        case DataType::UInt64: setIntegral<fmi3UInt64>(fns.fmi3SetUInt64, fmiComponent_, vrRef, value); break;
        case DataType::Boolean: {
            fmi3Boolean val = std::get<bool>(value) ? 1 : 0;
            auto fn = reinterpret_cast<fmi3SetBooleanFn>(fns.fmi3SetBoolean);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            break;
        }
        case DataType::String: {
            fmi3String val = std::get<std::string>(value).c_str();
            auto fn = reinterpret_cast<fmi3SetStringFn>(fns.fmi3SetString);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            break;
        }
        default: break;
    }
}

FmiValue StandaloneSimulationAdapter::getOutputFmi3(uint32_t vr, DataType dtype) {
    auto& fns = *pkg_->fmi3;
    fmi3ValueReference vrRef = static_cast<fmi3ValueReference>(vr);

    switch (dtype) {
        case DataType::Real: case DataType::Float64: {
            fmi3Float64 val = 0.0;
            auto fn = reinterpret_cast<fmi3GetFloat64Fn>(fns.fmi3GetFloat64);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            return FmiValue{static_cast<double>(val)};
        }
        case DataType::Float32: {
            fmi3Float32 val = 0.0f;
            auto fn = reinterpret_cast<fmi3GetFloat32Fn>(fns.fmi3GetFloat32);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            return FmiValue{static_cast<double>(val)};
        }
        case DataType::Integer: case DataType::Int32: {
            fmi3Int32 val = 0;
            auto fn = reinterpret_cast<fmi3GetInt32Fn>(fns.fmi3GetInt32);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            return FmiValue{static_cast<int64_t>(val)};
        }
        case DataType::Int64: {
            fmi3Int64 val = 0;
            auto fn = reinterpret_cast<fmi3GetInt64Fn>(fns.fmi3GetInt64);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            return FmiValue{val};
        }
        case DataType::Int8:   return getIntegral<fmi3Int8>(fns.fmi3GetInt8, fmiComponent_, vrRef);
        case DataType::Int16:  return getIntegral<fmi3Int16>(fns.fmi3GetInt16, fmiComponent_, vrRef);
        case DataType::UInt8:  return getIntegral<fmi3UInt8>(fns.fmi3GetUInt8, fmiComponent_, vrRef);
        case DataType::UInt16: return getIntegral<fmi3UInt16>(fns.fmi3GetUInt16, fmiComponent_, vrRef);
        case DataType::UInt32: return getIntegral<fmi3UInt32>(fns.fmi3GetUInt32, fmiComponent_, vrRef);
        case DataType::UInt64: return getIntegral<fmi3UInt64>(fns.fmi3GetUInt64, fmiComponent_, vrRef);
        case DataType::Boolean: {
            fmi3Boolean val = 0;
            auto fn = reinterpret_cast<fmi3GetBooleanFn>(fns.fmi3GetBoolean);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            return FmiValue{val != 0};
        }
        case DataType::String: {
            fmi3String val = nullptr;
            auto fn = reinterpret_cast<fmi3GetStringFn>(fns.fmi3GetString);
            if (fn) fn(static_cast<fmi3Instance>(fmiComponent_), &vrRef, 1, &val, 1);
            return FmiValue{val ? std::string(val) : std::string{}};
        }
        default: return FmiValue{0.0};
    }
}

bool StandaloneSimulationAdapter::doStepFmi3(double t, double dt) {
    const StepOutcome outcome = doStepEx(t, dt);
    return outcome.completed && !outcome.terminateSimulation;
}

StepOutcome StandaloneSimulationAdapter::doStepEx(double t, double dt) {
    if (!initialized_)
        throw FmuSimulationException("doStepEx called before initialize");
    if (!pkg_->isFmi3()) {
        StepOutcome out;
        out.completed = doStepFmi2(t, dt);
        out.lastSuccessfulTime = t + dt;
        return out;
    }
    auto& fns = *pkg_->fmi3;
    auto fn = reinterpret_cast<fmi3DoStepFn>(fns.fmi3DoStep);
    if (!fn)
        throw FmuSimulationException("fmi3DoStep function pointer is null");

    fmi3Boolean eventHandlingNeeded = false;
    fmi3Boolean terminateSimulation = false;
    fmi3Boolean earlyReturn = false;
    fmi3Float64 lastSuccessfulTime = t + dt;

    const fmi3Status status = fn(
        static_cast<fmi3Instance>(fmiComponent_),
        static_cast<fmi3Float64>(t), static_cast<fmi3Float64>(dt), true,
        &eventHandlingNeeded, &terminateSimulation, &earlyReturn, &lastSuccessfulTime);

    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3DoStep failed with status " + std::to_string(status));
    StepOutcome out;
    out.completed = status != fmi3Discard;
    out.eventHandlingNeeded = eventHandlingNeeded;
    out.terminateSimulation = terminateSimulation;
    out.earlyReturn = earlyReturn;
    out.lastSuccessfulTime = lastSuccessfulTime;
    return out;
}

const FmuPackage::Fmi3Functions& StandaloneSimulationAdapter::requireEventApi(const char* operation) const {
    if (!initialized_)
        throw FmuSimulationException(std::string(operation) + " called before initialize");
    if (!pkg_->isFmi3() || !pkg_->fmi3.has_value())
        throw FmuSimulationException(std::string(operation) + " needs an FMI 3.0 FMU");
    if (!pkg_->fmi3->isEventModeComplete())
        throw FmuSimulationException(std::string(operation) + ": the FMU does not export the Event Mode / Binary / Clock functions");
    return *pkg_->fmi3;
}

void StandaloneSimulationAdapter::enterEventMode() {
    const auto& fns = requireEventApi("enterEventMode");
    if (!eventModeUsed_)
        throw FmuSimulationException("enterEventMode: the session was opened without eventModeUsed");
    const auto status = reinterpret_cast<fmi3EnterModeFn>(fns.fmi3EnterEventMode)(static_cast<fmi3Instance>(fmiComponent_));
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3EnterEventMode failed with status " + std::to_string(status));
}

void StandaloneSimulationAdapter::enterStepMode() {
    const auto& fns = requireEventApi("enterStepMode");
    const auto status = reinterpret_cast<fmi3EnterModeFn>(fns.fmi3EnterStepMode)(static_cast<fmi3Instance>(fmiComponent_));
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3EnterStepMode failed with status " + std::to_string(status));
}

DiscreteStatesOutcome StandaloneSimulationAdapter::updateDiscreteStates() {
    const auto& fns = requireEventApi("updateDiscreteStates");
    fmi3Boolean needUpdate = false, terminate = false, nominals = false, values = false, nextDefined = false;
    fmi3Float64 nextTime = 0.0;
    const auto status = reinterpret_cast<fmi3UpdateDiscreteStatesFn>(fns.fmi3UpdateDiscreteStates)(
        static_cast<fmi3Instance>(fmiComponent_), &needUpdate, &terminate, &nominals, &values, &nextDefined, &nextTime);
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3UpdateDiscreteStates failed with status " + std::to_string(status));
    DiscreteStatesOutcome out;
    out.discreteStatesNeedUpdate = needUpdate;
    out.terminateSimulation = terminate;
    out.nextEventTimeDefined = nextDefined;
    out.nextEventTime = nextTime;
    return out;
}

std::vector<uint8_t> StandaloneSimulationAdapter::getBinary(uint32_t vr) {
    const auto& fns = requireEventApi("getBinary");
    const fmi3ValueReference ref = vr;
    size_t size = 0;
    fmi3Binary data = nullptr;
    const auto status = reinterpret_cast<fmi3GetBinaryFn>(fns.fmi3GetBinary)(
        static_cast<fmi3Instance>(fmiComponent_), &ref, 1, &size, &data, 1);
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3GetBinary failed with status " + std::to_string(status));
    if (size == 0) return {};
    if (data == nullptr || size > (64u << 20))
        throw FmuSimulationException("fmi3GetBinary returned an invalid buffer (size " + std::to_string(size) + ")");
    return std::vector<uint8_t>(data, data + size);   // copied now: the FMU may reuse its buffer
}

void StandaloneSimulationAdapter::setBinary(uint32_t vr, std::span<const uint8_t> bytes) {
    const auto& fns = requireEventApi("setBinary");
    const fmi3ValueReference ref = vr;
    const size_t size = bytes.size();
    const fmi3Binary data = bytes.data();
    const auto status = reinterpret_cast<fmi3SetBinaryFn>(fns.fmi3SetBinary)(
        static_cast<fmi3Instance>(fmiComponent_), &ref, 1, &size, &data, 1);
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3SetBinary failed with status " + std::to_string(status));
}

bool StandaloneSimulationAdapter::getClock(uint32_t vr) {
    const auto& fns = requireEventApi("getClock");
    const fmi3ValueReference ref = vr;
    fmi3Clock value = false;
    const auto status = reinterpret_cast<fmi3GetClockFn>(fns.fmi3GetClock)(static_cast<fmi3Instance>(fmiComponent_), &ref, 1, &value);
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3GetClock failed with status " + std::to_string(status));
    return value;
}

void StandaloneSimulationAdapter::setClock(uint32_t vr, bool active) {
    const auto& fns = requireEventApi("setClock");
    const fmi3ValueReference ref = vr;
    const fmi3Clock value = active;
    const auto status = reinterpret_cast<fmi3SetClockFn>(fns.fmi3SetClock)(static_cast<fmi3Instance>(fmiComponent_), &ref, 1, &value);
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3SetClock failed with status " + std::to_string(status));
}

ClockInterval StandaloneSimulationAdapter::getClockInterval(uint32_t vr) {
    const auto& fns = requireEventApi("getClockInterval");
    const fmi3ValueReference ref = vr;
    fmi3Float64 interval = 0.0;
    fmi3IntervalQualifier qualifier = 0;
    const auto status = reinterpret_cast<fmi3GetIntervalDecimalFn>(fns.fmi3GetIntervalDecimal)(
        static_cast<fmi3Instance>(fmiComponent_), &ref, 1, &interval, &qualifier);
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3GetIntervalDecimal failed with status " + std::to_string(status));
    ClockInterval out;
    out.interval = interval;
    out.qualifier = qualifier == 1 ? IntervalQualifier::Unchanged : qualifier == 2 ? IntervalQualifier::Changed : IntervalQualifier::NotYetKnown;
    return out;
}

void StandaloneSimulationAdapter::terminateFmi3() {
    auto& fns = *pkg_->fmi3;
    auto terminateFn = reinterpret_cast<fmi3TerminateFn>(fns.fmi3Terminate);
    if (terminateFn) terminateFn(static_cast<fmi3Instance>(fmiComponent_));
    auto freeInst = reinterpret_cast<fmi3FreeInstanceFn>(fns.fmi3FreeInstance);
    if (freeInst)   freeInst(static_cast<fmi3Instance>(fmiComponent_));
    fmiComponent_ = nullptr;
}

} // namespace libfmu_toolkit
