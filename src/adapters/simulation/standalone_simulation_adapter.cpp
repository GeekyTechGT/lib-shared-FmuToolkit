#include "standalone_simulation_adapter.hpp"
#include "core/exceptions/fmu_exception.hpp"

#include <algorithm>
#include <cstdint>
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
using fmi3Boolean        = int;
using fmi3String         = const char*;
using fmi3ValueReference = uint32_t;
using fmi3Status         = int;

constexpr fmi3Status fmi3OK      = 0;
constexpr fmi3Status fmi3Warning = 1;
constexpr fmi3Status fmi3Discard = 2;
constexpr fmi3Status fmi3Error   = 3;
constexpr fmi3Status fmi3Fatal   = 4;

using fmi3LogMessageCallbackFn = void(*)(void*, fmi3Status, fmi3String, fmi3String);

using fmi3InstantiateCoSimulationFn = fmi3Instance(*)(
    fmi3String, fmi3String, fmi3String, int, int, int, int,
    const fmi3ValueReference*, size_t,
    void*, fmi3LogMessageCallbackFn
);
using fmi3EnterInitializationModeFn = fmi3Status(*)(fmi3Instance, int, fmi3Float64,
                                                     fmi3Float64, int, fmi3Float64);
using fmi3ExitInitializationModeFn  = fmi3Status(*)(fmi3Instance);
using fmi3DoStepFn                  = fmi3Status(*)(fmi3Instance, fmi3Float64, fmi3Float64,
                                                     int, int*, int*, int*, fmi3Float64*);
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

    fmiComponent_ = instantiate(
        pkg.modelName.c_str(), pkg.guid.c_str(), resourceUri3.c_str(),
        0, 0, 0, 0, nullptr, 0, nullptr, nullptr);

    if (!fmiComponent_)
        throw FmuSimulationException("fmi3InstantiateCoSimulation returned null");

    auto enterInit = reinterpret_cast<fmi3EnterInitializationModeFn>(fns.fmi3EnterInitializationMode);
    if (!enterInit)
        throw FmuSimulationException("fmi3EnterInitializationMode function pointer is null");
    fmi3Status status = enterInit(static_cast<fmi3Instance>(fmiComponent_),
                                   0, config.tolerance, config.startTime, 1, config.stopTime);
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
    auto& fns = *pkg_->fmi3;
    auto fn = reinterpret_cast<fmi3DoStepFn>(fns.fmi3DoStep);
    if (!fn)
        throw FmuSimulationException("fmi3DoStep function pointer is null");

    int eventHandlingNeeded = 0;
    int terminateSimulation = 0;
    int earlyReturn = 0;
    fmi3Float64 lastSuccessfulTime = 0.0;

    fmi3Status status = fn(
        static_cast<fmi3Instance>(fmiComponent_),
        static_cast<fmi3Float64>(t), static_cast<fmi3Float64>(dt), 1,
        &eventHandlingNeeded, &terminateSimulation, &earlyReturn, &lastSuccessfulTime);

    if (terminateSimulation || status == fmi3Discard) return false;
    if (status >= fmi3Error)
        throw FmuSimulationException("fmi3DoStep failed with status " + std::to_string(status));
    return true;
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
