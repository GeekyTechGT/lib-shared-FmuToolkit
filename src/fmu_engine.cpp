// Public header MUST come first — FmuEngine.hpp declares class FmuEngine and
// class FmuSession (with their forward-declared struct Impl). FmuEngineImpl.hpp
// then provides the out-of-class definitions of those nested structs, which
// requires the enclosing classes to be known.
#include <libfmu_toolkit/fmu_engine.hpp>
#include <libfmu_toolkit/types.hpp>

#include "fmu_engine_impl.hpp"

#include <filesystem>
#include <functional>
#include <stdexcept>
#include <utility>

namespace libfmu_toolkit {

// ---------------------------------------------------------------------------
// FmuInfo convenience methods (implemented here so that Impl is visible)
// ---------------------------------------------------------------------------

std::vector<const VariableInfo*> FmuInfo::inputs() const {
    std::vector<const VariableInfo*> res;
    for (const auto& v : variables)
        if (v.causality == Causality::Input)
            res.push_back(&v);
    return res;
}

std::vector<const VariableInfo*> FmuInfo::outputs() const {
    std::vector<const VariableInfo*> res;
    for (const auto& v : variables)
        if (v.causality == Causality::Output)
            res.push_back(&v);
    return res;
}

std::vector<const VariableInfo*> FmuInfo::parameters() const {
    std::vector<const VariableInfo*> res;
    for (const auto& v : variables)
        if (v.causality == Causality::Parameter)
            res.push_back(&v);
    return res;
}

const VariableInfo* FmuInfo::findVariable(std::string_view name) const {
    for (const auto& v : variables)
        if (v.name == name)   // std::string == std::string_view, valid C++17+
            return &v;
    return nullptr;
}

// ---------------------------------------------------------------------------
// ValidationResult / SimResult — out-of-line helpers (ABI stability)
// ---------------------------------------------------------------------------

bool ValidationResult::hasErrors()   const noexcept { return !errors.empty(); }
bool ValidationResult::hasWarnings() const noexcept { return !warnings.empty(); }
size_t SimResult::stepCount()        const noexcept { return timePoints.size(); }
bool   SimResult::empty()            const noexcept { return timePoints.empty(); }

// ---------------------------------------------------------------------------
// FmuEngine — PIMPL wrapper methods
// ---------------------------------------------------------------------------

FmuEngine::FmuEngine(std::string_view xsdBasePath)
    : impl_(std::make_unique<Impl>(xsdBasePath))
{}

FmuEngine::~FmuEngine() = default;

FmuEngine::FmuEngine(FmuEngine&&) noexcept = default;
FmuEngine& FmuEngine::operator=(FmuEngine&&) noexcept = default;

// ---------------------------------------------------------------------------
// load()
// ---------------------------------------------------------------------------

uint64_t FmuEngine::load(std::string_view fmuPath)
{
    try {
        // Each load() creates a fresh ZipLoaderAdapter (heap-allocated so
        // Entry can store it via unique_ptr without requiring a movable adapter).
        // std::filesystem::path is constructible from std::string_view (C++20).
        auto loader = std::make_unique<ZipLoaderAdapter>();
        FmuImporter importer(*loader);
        FmuPackage pkg = importer.import(std::filesystem::path(fmuPath));

        uint64_t id = impl_->nextHandleId++;

        // Move BOTH loader (owns OS handles) AND pkg into Entry.
        // The loader must outlive the pkg because pkg.sharedLibHandle and
        // FMI function pointers point into the DLL that loader keeps open.
        impl_->entries.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(id),
            std::forward_as_tuple(std::move(loader), std::move(pkg)));

        return id;
    }
    catch (const FmuLoadException& e) {
        throw FmuLoadError(e.what());
    }
    catch (const FmuValidationException& e) {
        throw FmuValidationError(e.what());
    }
    catch (const std::invalid_argument& e) {
        throw FmuLoadError(e.what());
    }
}

// ---------------------------------------------------------------------------
// info()
// ---------------------------------------------------------------------------

FmuInfo FmuEngine::info(uint64_t handle) const
{
    const auto& entry = impl_->getEntry(handle);
    return detail::toPublicInfo(entry.pkg);
}

// ---------------------------------------------------------------------------
// validate()
// ---------------------------------------------------------------------------

ValidationResult FmuEngine::validate(uint64_t handle) const
{
    const auto& entry = impl_->getEntry(handle);
    FmuValidator validator(*impl_->xmlValidator,
                           impl_->fmi2SchemaDir(),
                           impl_->fmi3SchemaDir());
    ValidationReport report = validator.validate(entry.pkg);
    return detail::toPublicReport(report);
}

// ---------------------------------------------------------------------------
// simulate()
// ---------------------------------------------------------------------------

SimResult FmuEngine::simulate(
    uint64_t handle,
    const SimConfig& config,
    std::function<void(size_t, double, size_t)> progress)
{
    auto& entry = impl_->getEntry(handle);

    SimulationConfig internalCfg = detail::toInternalConfig(config);

    try {
        StandaloneSimulationAdapter backend;
        SimulationController controller(backend);
        SimulationResult result = controller.run(entry.pkg, internalCfg, std::move(progress));
        return detail::toPublicResult(result);
    }
    catch (const FmuSimulationException& e) {
        throw FmuSimulationError(e.what());
    }
    catch (const FmuLoadException& e) {
        throw FmuLoadError(e.what());
    }
}

// ---------------------------------------------------------------------------
// exportCsv()
// ---------------------------------------------------------------------------

void FmuEngine::exportCsv(const SimResult& result, std::string_view outputPath)
{
    // SimResult and SimulationResult have structurally identical member types
    // (both using OutputValue from <libfmu_toolkit/types.hpp>), so the field copy is direct.
    // std::filesystem::path is constructible from std::string_view (C++20).
    SimulationResult internal;
    internal.timePoints = result.timePoints;
    internal.series     = result.series;
    CsvResultExporter::exportToCsv(internal, std::filesystem::path(outputPath));
}

// ---------------------------------------------------------------------------
// exportJson()
// ---------------------------------------------------------------------------

void FmuEngine::exportJson(const SimResult& result, std::string_view outputPath)
{
    SimulationResult internal;
    internal.timePoints = result.timePoints;
    internal.series     = result.series;
    JsonResultExporter::exportToJson(internal, std::filesystem::path(outputPath));
}

// ---------------------------------------------------------------------------
// unload()
// ---------------------------------------------------------------------------

void FmuEngine::unload(uint64_t handle)
{
    auto it = impl_->entries.find(handle);
    if (it == impl_->entries.end())
        throw std::out_of_range("FmuEngine::unload — unknown handle " + std::to_string(handle));
    impl_->entries.erase(it);
}

// ---------------------------------------------------------------------------
// openSession()
// ---------------------------------------------------------------------------

FmuSession FmuEngine::openSession(uint64_t handle, const SimConfig& config)
{
    auto& entry = impl_->getEntry(handle);
    SimulationConfig internalCfg = detail::toInternalConfig(config);

    auto sessionImpl = std::make_unique<FmuSession::Impl>();
    sessionImpl->info = detail::toPublicInfo(entry.pkg);

    for (const auto& mv : entry.pkg.variables)
        sessionImpl->varIndex[mv.name] = {mv.valueReference, mv.dataType};

    try {
        sessionImpl->backend->initialize(entry.pkg, internalCfg);
        sessionImpl->active = true;
    }
    catch (const FmuSimulationException& e) {
        throw FmuSimulationError(e.what());
    }

    return FmuSession(std::move(sessionImpl));
}

// ---------------------------------------------------------------------------
// FmuSession — constructor, destructor, move
// ---------------------------------------------------------------------------

FmuSession::FmuSession(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{}

FmuSession::~FmuSession()
{
    if (impl_ && impl_->active) {
        try { impl_->backend->terminate(); } catch (...) {}
        impl_->active = false;
    }
}

FmuSession::FmuSession(FmuSession&&) noexcept = default;
FmuSession& FmuSession::operator=(FmuSession&&) noexcept = default;

// ---------------------------------------------------------------------------
// FmuSession — queries
// ---------------------------------------------------------------------------

const FmuInfo& FmuSession::info() const noexcept
{
    return impl_->info;
}

bool FmuSession::isActive() const noexcept
{
    return impl_ && impl_->active;
}

// ---------------------------------------------------------------------------
// FmuSession — setValue / getValue
// ---------------------------------------------------------------------------

void FmuSession::setValue(std::string_view varName, double value)
{
    // varIndex uses std::less<> (transparent comparator), so find() accepts
    // std::string_view directly without constructing a temporary std::string.
    auto it = impl_->varIndex.find(varName);
    if (it == impl_->varIndex.end())
        throw std::out_of_range("FmuSession::setValue — unknown variable: " + std::string(varName));
    try {
        impl_->backend->setInput(it->second.first, it->second.second, FmiValue{value});
    } catch (const FmuSimulationException& e) { throw FmuSimulationError(e.what()); }
}

void FmuSession::setValue(std::string_view varName, int64_t value)
{
    auto it = impl_->varIndex.find(varName);
    if (it == impl_->varIndex.end())
        throw std::out_of_range("FmuSession::setValue — unknown variable: " + std::string(varName));
    try {
        impl_->backend->setInput(it->second.first, it->second.second, FmiValue{value});
    } catch (const FmuSimulationException& e) { throw FmuSimulationError(e.what()); }
}

void FmuSession::setValue(std::string_view varName, bool value)
{
    auto it = impl_->varIndex.find(varName);
    if (it == impl_->varIndex.end())
        throw std::out_of_range("FmuSession::setValue — unknown variable: " + std::string(varName));
    try {
        impl_->backend->setInput(it->second.first, it->second.second, FmiValue{value});
    } catch (const FmuSimulationException& e) { throw FmuSimulationError(e.what()); }
}

void FmuSession::setValue(std::string_view varName, std::string_view value)
{
    auto it = impl_->varIndex.find(varName);
    if (it == impl_->varIndex.end())
        throw std::out_of_range("FmuSession::setValue — unknown variable: " + std::string(varName));
    try {
        // FMI backends require an owned std::string; construct it here, inside
        // the library, so the allocation always uses the library's allocator.
        impl_->backend->setInput(it->second.first, it->second.second,
                                 FmiValue{std::string(value)});
    } catch (const FmuSimulationException& e) { throw FmuSimulationError(e.what()); }
}

OutputValue FmuSession::getValue(std::string_view varName) const
{
    auto it = impl_->varIndex.find(varName);
    if (it == impl_->varIndex.end())
        throw std::out_of_range("FmuSession::getValue — unknown variable: " + std::string(varName));
    try {
        FmiValue v = impl_->backend->getOutput(it->second.first, it->second.second);
        // FmiValue and OutputValue are both std::variant<double,int64_t,bool,std::string>
        return std::visit([](auto&& x) -> OutputValue { return x; }, v);
    } catch (const FmuSimulationException& e) { throw FmuSimulationError(e.what()); }
}

// ---------------------------------------------------------------------------
// FmuSession — doStep / terminate
// ---------------------------------------------------------------------------

bool FmuSession::doStep(double currentTime, double stepSize)
{
    try {
        return impl_->backend->doStep(currentTime, stepSize);
    } catch (const FmuSimulationException& e) {
        throw FmuSimulationError(e.what());
    }
}

void FmuSession::terminate()
{
    if (!impl_ || !impl_->active) return;
    try {
        impl_->backend->terminate();
    } catch (const FmuSimulationException& e) {
        impl_->active = false;
        throw FmuSimulationError(e.what());
    }
    impl_->active = false;
}

} // namespace libfmu_toolkit
