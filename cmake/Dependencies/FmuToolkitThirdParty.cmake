if(DEFINED FMUTOOLKIT_THIRD_PARTY_INCLUDED)
    return()
endif()
set(FMUTOOLKIT_THIRD_PARTY_INCLUDED TRUE)

# Project-specific third-party resolution: ported from the pre-scaffold
# lib-fmu-toolkit's own root CMakeLists.txt. Not shared with any other
# category (cmake/Dependencies/ThirdParty.cmake only knows nlohmann_json/
# httplib), kept here rather than forced into that generic file - libxml2/
# libzip static linking is unique to this library.
#
# On MinGW, libxml2 and libzip are linked STATICALLY into fmutoolkit.dll (via
# pkg-config's --static output) so consumers need nothing beyond the DLL
# itself and libwinpthread-1.dll - no libxml2.dll/libzip.dll to distribute.
# Elsewhere, they are linked as normal shared dependencies.

option(FMUTOOLKIT_MINGW_STATIC_RUNTIME
    "On MinGW, statically link the C++ runtime and libxml2/libzip into fmutoolkit.dll" ON)

include(FetchContent)
include(FindPkgConfig)

# -static-libgcc/-static-libstdc++ so libstdc++-6.dll/libgcc_s_seh-1.dll are
# not required at the consumer's end; winpthread is linked statically where
# possible (a transitive libwinpthread-1.dll reference can still surface if
# libxml2.a/libzip.a pull in further pthread symbols after -Bdynamic -
# distribute it alongside the DLL in that case, see fmutoolkit_copy_winpthread).
function(fmutoolkit_apply_mingw_static_runtime target_name)
    if(MINGW AND FMUTOOLKIT_MINGW_STATIC_RUNTIME)
        target_link_options(${target_name} PRIVATE
            -static-libgcc
            -static-libstdc++
            -Wl,-Bstatic
            -lwinpthread
            -Wl,-Bdynamic
        )
    endif()
endfunction()

# Copies libwinpthread-1.dll next to an EXECUTABLE target at build time (must
# only be called for executables, not libraries - nested cmd.exe /C quoting
# breaks otherwise on Windows).
function(fmutoolkit_copy_winpthread target_name)
    if(MINGW AND FMUTOOLKIT_MINGW_STATIC_RUNTIME AND WIN32)
        get_filename_component(_mingw_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(_winpthread_src "${_mingw_bin}/libwinpthread-1.dll")
        if(EXISTS "${_winpthread_src}")
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_winpthread_src}"
                    "$<TARGET_FILE_DIR:${target_name}>"
                COMMENT "Copying libwinpthread-1.dll next to ${target_name}"
            )
        endif()
    endif()
endfunction()

# Resolves nlohmann_json, LibXml2 and libzip, then links them (statically on
# MinGW, see above) into `target_name` and applies the MinGW static runtime.
function(fmutoolkit_configure_third_party target_name)
    gt_require_nlohmann_json()
    target_link_libraries(${target_name} PRIVATE nlohmann_json::nlohmann_json)

    find_package(LibXml2 REQUIRED)
    find_package(PkgConfig REQUIRED)

    set(_xml_link_libs LibXml2::LibXml2)
    set(_libzip_target "")
    set(_extra_include_dirs "")
    set(_extra_compile_definitions "")
    set(_extra_link_libs "")
    set(_extra_system_link_libs "")

    if(MINGW AND FMUTOOLKIT_MINGW_STATIC_RUNTIME)
        pkg_check_modules(FMUTOOLKIT_LIBXML2_PKG REQUIRED IMPORTED_TARGET libxml-2.0)
        pkg_check_modules(FMUTOOLKIT_LIBZIP_PKG REQUIRED IMPORTED_TARGET libzip)

        set(_extra_include_dirs
            ${FMUTOOLKIT_LIBXML2_PKG_INCLUDE_DIRS}
            ${FMUTOOLKIT_LIBZIP_PKG_INCLUDE_DIRS}
        )
        set(_extra_compile_definitions LIBXML_STATIC ZIP_STATIC)
        set(_xml_link_libs "")

        set(_static_lib_names
            ${FMUTOOLKIT_LIBXML2_PKG_STATIC_LIBRARIES}
            ${FMUTOOLKIT_LIBZIP_PKG_STATIC_LIBRARIES}
        )
        list(REMOVE_DUPLICATES _static_lib_names)

        # Windows system libraries pkg-config lists alongside libxml2/libzip's
        # own static archives - linked by name (-lxxx), never resolved to a
        # static-library file.
        set(_windows_system_libs
            advapi32 bcrypt crypt32 gdi32 iphlpapi kernel32 ncrypt netapi32
            ole32 oleaut32 rpcrt4 secur32 shell32 user32 userenv uuid version
            winhttp wininet winmm ws2_32
        )

        foreach(_lib_name IN LISTS _static_lib_names)
            if(_lib_name IN_LIST _windows_system_libs)
                list(APPEND _extra_system_link_libs ${_lib_name})
                continue()
            endif()
            set(_static_archive "")
            foreach(_lib_dir IN LISTS FMUTOOLKIT_LIBXML2_PKG_STATIC_LIBRARY_DIRS FMUTOOLKIT_LIBZIP_PKG_STATIC_LIBRARY_DIRS)
                if(EXISTS "${_lib_dir}/lib${_lib_name}.a")
                    set(_static_archive "${_lib_dir}/lib${_lib_name}.a")
                    break()
                endif()
                if(EXISTS "${_lib_dir}/${_lib_name}.a")
                    set(_static_archive "${_lib_dir}/${_lib_name}.a")
                    break()
                endif()
            endforeach()
            if(_static_archive)
                list(APPEND _extra_link_libs ${_static_archive})
            else()
                list(APPEND _extra_link_libs ${_lib_name})
            endif()
        endforeach()
        list(REMOVE_DUPLICATES _extra_system_link_libs)
    else()
        find_package(unofficial-libzip CONFIG QUIET)
        if(unofficial-libzip_FOUND)
            set(_libzip_target unofficial::libzip::libzip)
        else()
            pkg_check_modules(FMUTOOLKIT_LIBZIP REQUIRED IMPORTED_TARGET libzip)
            set(_libzip_target PkgConfig::FMUTOOLKIT_LIBZIP)
        endif()
    endif()

    target_include_directories(${target_name} PRIVATE ${_extra_include_dirs})
    target_compile_definitions(${target_name} PRIVATE
        ${_extra_compile_definitions}
        # Resolved by src/fmu_generation.cpp and fmu_engine_impl.hpp (XSD
        # schema lookup) - the fmi-standard-2.0/3.0 git submodules land here.
        LIB_FMU_TOOLKIT_SOURCE_EXTERNAL_DIR="${CMAKE_SOURCE_DIR}/external"
    )
    target_link_libraries(${target_name} PRIVATE
        ${_xml_link_libs}
        ${_libzip_target}
        ${_extra_link_libs}
        ${_extra_system_link_libs}
        ${CMAKE_DL_LIBS}
    )

    # Pin the libstdc++ C++11 ABI (non-COW string/list, GCC >= 5) so code
    # built against an older ABI cannot silently corrupt the heap across the
    # library boundary. PUBLIC: propagates to consumers so they stay
    # ABI-consistent. Unknown to MSVC, harmless there.
    if(NOT MSVC)
        target_compile_definitions(${target_name} PUBLIC _GLIBCXX_USE_CXX11_ABI=1)
    endif()

    fmutoolkit_apply_mingw_static_runtime(${target_name})
endfunction()
