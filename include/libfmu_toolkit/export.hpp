/// @file export.hpp
/// @brief DLL visibility macros for the lib-fmu-toolkit public API.
///
/// Consumers do not need to include this header directly — it is pulled in
/// transitively by every other public header via `types.hpp` and `fmu_engine.hpp`.
///
/// When building the shared library (`LIB_FMU_TOOLKIT_BUILDING_SHARED` defined),
/// `LIB_FMU_TOOLKIT_API` expands to the appropriate dllexport / visibility attribute.
/// When consuming the shared library (`LIB_FMU_TOOLKIT_SHARED` defined without
/// `LIB_FMU_TOOLKIT_BUILDING_SHARED`), it expands to dllimport.
/// For static builds and GCC/Clang with hidden-by-default visibility, it expands
/// to `__attribute__((visibility("default")))`.
#pragma once

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(LIB_FMU_TOOLKIT_SHARED)
#    if defined(LIB_FMU_TOOLKIT_BUILDING_SHARED)
#      define LIB_FMU_TOOLKIT_API __declspec(dllexport)
#    else
#      define LIB_FMU_TOOLKIT_API __declspec(dllimport)
#    endif
#  else
#    define LIB_FMU_TOOLKIT_API
#  endif
#elif defined(__GNUC__) || defined(__clang__)
#  define LIB_FMU_TOOLKIT_API __attribute__((visibility("default")))
#else
#  define LIB_FMU_TOOLKIT_API
#endif
