#include "zip_loader_adapter.hpp"
#include "adapters/xml/model_description_parser.hpp"
#include "core/exceptions/fmu_exception.hpp"

#include <zip.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace libfmu_toolkit {

namespace {

std::string randomHexSuffix() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    std::ostringstream oss;
    oss << std::hex << dist(gen);
    return oss.str();
}

void makeDirectoriesP(const std::filesystem::path& path) {
    std::filesystem::create_directories(path);
}

} // anonymous namespace

ZipLoaderAdapter::ZipLoaderAdapter(std::filesystem::path extractionRoot)
    : extractionRoot_(std::move(extractionRoot))
{
    if (extractionRoot_.empty()) {
        extractionRoot_ = std::filesystem::temp_directory_path()
                          / "libfmu_toolkit"
                          / randomHexSuffix();
    }
}

ZipLoaderAdapter::~ZipLoaderAdapter() {
    for (void* handle : loadedHandles_) {
        if (handle) {
#ifdef _WIN32
            FreeLibrary(static_cast<HMODULE>(handle));
#else
            dlclose(handle);
#endif
        }
    }
}

FmuPackage ZipLoaderAdapter::load(const std::filesystem::path& fmuPath) {
    std::filesystem::path extractedDir = extractFmu(fmuPath);

    std::filesystem::path xmlPath = extractedDir / "modelDescription.xml";
    if (!std::filesystem::exists(xmlPath)) {
        throw FmuLoadException(
            "ZipLoaderAdapter::load — modelDescription.xml not found in FMU: "
            + fmuPath.string());
    }

    std::ifstream xmlFile(xmlPath);
    if (!xmlFile.is_open()) {
        throw FmuLoadException(
            "ZipLoaderAdapter::load — cannot open modelDescription.xml: "
            + xmlPath.string());
    }
    std::string xmlContent(
        (std::istreambuf_iterator<char>(xmlFile)),
        std::istreambuf_iterator<char>());
    xmlFile.close();

    FmuPackage pkg;
    pkg.extractedPath = extractedDir;
    ModelDescriptionParser::parse(xmlContent, pkg);

    const std::string& libName = pkg.modelIdentifier.empty()
                               ? pkg.modelName
                               : pkg.modelIdentifier;
    std::filesystem::path libPath = findSharedLibrary(extractedDir, libName);
    pkg.sharedLibraryPath = libPath;

    void* handle = loadSharedLibrary(libPath);
    pkg.sharedLibHandle = handle;
    loadedHandles_.push_back(handle);

    if (pkg.isFmi2()) {
        loadFmi2FunctionPointers(pkg);
    } else {
        loadFmi3FunctionPointers(pkg);
    }

    return pkg;
}

std::filesystem::path ZipLoaderAdapter::extractFmu(const std::filesystem::path& fmuPath) {
    std::filesystem::path destDir = extractionRoot_
                                    / (fmuPath.stem().string() + "_" + randomHexSuffix());
    makeDirectoriesP(destDir);

    int zipErr = 0;
    zip_t* za = zip_open(fmuPath.string().c_str(), ZIP_RDONLY, &zipErr);
    if (!za) {
        zip_error_t ze;
        zip_error_init_with_code(&ze, zipErr);
        std::string msg = "ZipLoaderAdapter::extractFmu — cannot open ZIP: "
                          + fmuPath.string() + " (" + zip_error_strerror(&ze) + ")";
        zip_error_fini(&ze);
        throw FmuLoadException(msg);
    }

    struct ZipGuard {
        zip_t* z;
        explicit ZipGuard(zip_t* p) : z(p) {}
        ~ZipGuard() { if (z) zip_close(z); }
    } zipGuard(za);

    zip_int64_t numEntries = zip_get_num_entries(za, 0);
    if (numEntries < 0) {
        throw FmuLoadException(
            "ZipLoaderAdapter::extractFmu — zip_get_num_entries failed for: "
            + fmuPath.string());
    }

    for (zip_int64_t i = 0; i < numEntries; ++i) {
        zip_stat_t st;
        zip_stat_init(&st);
        if (zip_stat_index(za, static_cast<zip_uint64_t>(i), 0, &st) != 0) {
            throw FmuLoadException(
                "ZipLoaderAdapter::extractFmu — zip_stat_index failed for entry "
                + std::to_string(i) + " in: " + fmuPath.string());
        }

        std::string entryName(st.name);

        if (!entryName.empty() && entryName.back() == '/') {
            makeDirectoriesP(destDir / entryName);
            continue;
        }

        std::filesystem::path outPath = destDir / entryName;
        makeDirectoriesP(outPath.parent_path());

        zip_file_t* zf = zip_fopen_index(za, static_cast<zip_uint64_t>(i), 0);
        if (!zf) {
            throw FmuLoadException(
                "ZipLoaderAdapter::extractFmu — zip_fopen_index failed for entry: "
                + entryName);
        }

        {
            std::ofstream outFile(outPath, std::ios::binary);
            if (!outFile.is_open()) {
                zip_fclose(zf);
                throw FmuLoadException(
                    "ZipLoaderAdapter::extractFmu — cannot create output file: "
                    + outPath.string());
            }

            constexpr std::size_t bufSize = 65536;
            std::vector<char> buf(bufSize);
            zip_int64_t bytesRead = 0;
            while ((bytesRead = zip_fread(zf, buf.data(), bufSize)) > 0) {
                outFile.write(buf.data(), bytesRead);
            }

            if (bytesRead < 0) {
                zip_fclose(zf);
                throw FmuLoadException(
                    "ZipLoaderAdapter::extractFmu — zip_fread error for entry: "
                    + entryName);
            }
        }

        zip_fclose(zf);
    }

    return destDir;
}

std::filesystem::path ZipLoaderAdapter::findSharedLibrary(
    const std::filesystem::path& extractedDir,
    const std::string& modelName)
{
#ifdef _WIN32
#  if defined(_WIN64) || defined(__x86_64__)
    std::vector<std::filesystem::path> candidates = {
        extractedDir / "binaries" / "x86_64-windows" / (modelName + ".dll"),
        extractedDir / "binaries" / "win64"          / (modelName + ".dll"),
        extractedDir / "binaries" / "win32"          / (modelName + ".dll"),
    };
#  else
    std::vector<std::filesystem::path> candidates = {
        extractedDir / "binaries" / "x86-windows" / (modelName + ".dll"),
        extractedDir / "binaries" / "win32"        / (modelName + ".dll"),
        extractedDir / "binaries" / "win64"        / (modelName + ".dll"),
    };
#  endif
    const std::string libExt = ".dll";
#elif defined(__APPLE__)
    std::vector<std::filesystem::path> candidates = {
        extractedDir / "binaries" / "aarch64-darwin" / (modelName + ".dylib"),
        extractedDir / "binaries" / "x86_64-darwin"  / (modelName + ".dylib"),
        extractedDir / "binaries" / "darwin64"        / (modelName + ".dylib"),
    };
    const std::string libExt = ".dylib";
#else
    std::vector<std::filesystem::path> candidates = {
        extractedDir / "binaries" / "x86_64-linux" / (modelName + ".so"),
        extractedDir / "binaries" / "linux64"       / (modelName + ".so"),
        extractedDir / "binaries" / "linux32"       / (modelName + ".so"),
    };
    const std::string libExt = ".so";
#endif

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate))
            return candidate;
    }

    std::filesystem::path binariesDir = extractedDir / "binaries";
    if (std::filesystem::exists(binariesDir) && std::filesystem::is_directory(binariesDir)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(binariesDir)) {
            if (!entry.is_regular_file()) continue;
            const auto& p = entry.path();
            if (p.extension() == libExt && p.stem() == modelName)
                return p;
        }
    }

    throw FmuLoadException(
        "ZipLoaderAdapter::findSharedLibrary — shared library for model '"
        + modelName + "' not found in: " + (extractedDir / "binaries").string());
}

void* ZipLoaderAdapter::loadSharedLibrary(const std::filesystem::path& libPath) {
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(libPath.string().c_str());
    if (!handle) {
        DWORD err = GetLastError();
        throw FmuLoadException(
            "ZipLoaderAdapter::loadSharedLibrary — LoadLibraryA failed for: "
            + libPath.string() + " (error code: " + std::to_string(err) + ")");
    }
    return static_cast<void*>(handle);
#else
    void* handle = dlopen(libPath.string().c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        const char* err = dlerror();
        throw FmuLoadException(
            "ZipLoaderAdapter::loadSharedLibrary — dlopen failed for: "
            + libPath.string() + " (" + (err ? err : "unknown error") + ")");
    }
    return handle;
#endif
}

void* ZipLoaderAdapter::resolveSymbol(void* handle, const char* name) {
    if (!handle || !name) return nullptr;
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
}

void ZipLoaderAdapter::loadFmi2FunctionPointers(FmuPackage& pkg) {
    FmuPackage::Fmi2Functions fns;
    void* h = pkg.sharedLibHandle;

    fns.fmi2GetTypesPlatform        = resolveSymbol(h, "fmi2GetTypesPlatform");
    fns.fmi2GetVersion              = resolveSymbol(h, "fmi2GetVersion");
    fns.fmi2SetDebugLogging         = resolveSymbol(h, "fmi2SetDebugLogging");
    fns.fmi2Instantiate             = resolveSymbol(h, "fmi2Instantiate");
    fns.fmi2FreeInstance            = resolveSymbol(h, "fmi2FreeInstance");
    fns.fmi2SetupExperiment         = resolveSymbol(h, "fmi2SetupExperiment");
    fns.fmi2EnterInitializationMode = resolveSymbol(h, "fmi2EnterInitializationMode");
    fns.fmi2ExitInitializationMode  = resolveSymbol(h, "fmi2ExitInitializationMode");
    fns.fmi2Terminate               = resolveSymbol(h, "fmi2Terminate");
    fns.fmi2Reset                   = resolveSymbol(h, "fmi2Reset");
    fns.fmi2GetReal                 = resolveSymbol(h, "fmi2GetReal");
    fns.fmi2GetInteger              = resolveSymbol(h, "fmi2GetInteger");
    fns.fmi2GetBoolean              = resolveSymbol(h, "fmi2GetBoolean");
    fns.fmi2GetString               = resolveSymbol(h, "fmi2GetString");
    fns.fmi2SetReal                 = resolveSymbol(h, "fmi2SetReal");
    fns.fmi2SetInteger              = resolveSymbol(h, "fmi2SetInteger");
    fns.fmi2SetBoolean              = resolveSymbol(h, "fmi2SetBoolean");
    fns.fmi2SetString               = resolveSymbol(h, "fmi2SetString");
    fns.fmi2DoStep                  = resolveSymbol(h, "fmi2DoStep");

    if (!fns.isComplete()) {
        throw FmuLoadException(
            "ZipLoaderAdapter::loadFmi2FunctionPointers — one or more mandatory FMI 2.0 "
            "symbols could not be resolved from: " + pkg.sharedLibraryPath.string());
    }
    pkg.fmi2 = fns;
}

void ZipLoaderAdapter::loadFmi3FunctionPointers(FmuPackage& pkg) {
    FmuPackage::Fmi3Functions fns;
    void* h = pkg.sharedLibHandle;

    fns.fmi3GetVersion                = resolveSymbol(h, "fmi3GetVersion");
    fns.fmi3SetDebugLogging           = resolveSymbol(h, "fmi3SetDebugLogging");
    fns.fmi3InstantiateCoSimulation   = resolveSymbol(h, "fmi3InstantiateCoSimulation");
    fns.fmi3FreeInstance              = resolveSymbol(h, "fmi3FreeInstance");
    fns.fmi3EnterInitializationMode   = resolveSymbol(h, "fmi3EnterInitializationMode");
    fns.fmi3ExitInitializationMode    = resolveSymbol(h, "fmi3ExitInitializationMode");
    fns.fmi3Terminate                 = resolveSymbol(h, "fmi3Terminate");
    fns.fmi3Reset                     = resolveSymbol(h, "fmi3Reset");
    fns.fmi3GetFloat64                = resolveSymbol(h, "fmi3GetFloat64");
    fns.fmi3GetFloat32                = resolveSymbol(h, "fmi3GetFloat32");
    fns.fmi3GetInt8                   = resolveSymbol(h, "fmi3GetInt8");
    fns.fmi3GetInt16                  = resolveSymbol(h, "fmi3GetInt16");
    fns.fmi3GetInt32                  = resolveSymbol(h, "fmi3GetInt32");
    fns.fmi3GetInt64                  = resolveSymbol(h, "fmi3GetInt64");
    fns.fmi3GetUInt8                  = resolveSymbol(h, "fmi3GetUInt8");
    fns.fmi3GetUInt16                 = resolveSymbol(h, "fmi3GetUInt16");
    fns.fmi3GetUInt32                 = resolveSymbol(h, "fmi3GetUInt32");
    fns.fmi3GetUInt64                 = resolveSymbol(h, "fmi3GetUInt64");
    fns.fmi3GetBoolean                = resolveSymbol(h, "fmi3GetBoolean");
    fns.fmi3GetString                 = resolveSymbol(h, "fmi3GetString");
    fns.fmi3GetBinary                 = resolveSymbol(h, "fmi3GetBinary");
    fns.fmi3GetClock                  = resolveSymbol(h, "fmi3GetClock");
    fns.fmi3SetFloat64                = resolveSymbol(h, "fmi3SetFloat64");
    fns.fmi3SetFloat32                = resolveSymbol(h, "fmi3SetFloat32");
    fns.fmi3SetInt8                   = resolveSymbol(h, "fmi3SetInt8");
    fns.fmi3SetInt16                  = resolveSymbol(h, "fmi3SetInt16");
    fns.fmi3SetInt32                  = resolveSymbol(h, "fmi3SetInt32");
    fns.fmi3SetInt64                  = resolveSymbol(h, "fmi3SetInt64");
    fns.fmi3SetUInt8                  = resolveSymbol(h, "fmi3SetUInt8");
    fns.fmi3SetUInt16                 = resolveSymbol(h, "fmi3SetUInt16");
    fns.fmi3SetUInt32                 = resolveSymbol(h, "fmi3SetUInt32");
    fns.fmi3SetUInt64                 = resolveSymbol(h, "fmi3SetUInt64");
    fns.fmi3SetBoolean                = resolveSymbol(h, "fmi3SetBoolean");
    fns.fmi3SetString                 = resolveSymbol(h, "fmi3SetString");
    fns.fmi3SetBinary                 = resolveSymbol(h, "fmi3SetBinary");
    fns.fmi3SetClock                  = resolveSymbol(h, "fmi3SetClock");
    fns.fmi3DoStep                    = resolveSymbol(h, "fmi3DoStep");

    if (!fns.isComplete()) {
        throw FmuLoadException(
            "ZipLoaderAdapter::loadFmi3FunctionPointers — one or more mandatory FMI 3.0 "
            "symbols could not be resolved from: " + pkg.sharedLibraryPath.string());
    }
    pkg.fmi3 = fns;
}

} // namespace libfmu_toolkit
