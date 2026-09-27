if(DEFINED GT_THIRD_PARTY_INCLUDED)
    return()
endif()
set(GT_THIRD_PARTY_INCLUDED TRUE)

# Pinned third-party libraries shared by several categories. Each function
# prefers an installed CMake package (offline builds) and otherwise downloads
# the pinned release once per build tree (FetchContent).
include(FetchContent)

# nlohmann/json (header-only): JSON for the MCP adapter (app, tool) and the
# HTTP adapter (svc-cpp). Target: nlohmann_json::nlohmann_json.
function(gt_require_nlohmann_json)
    if(TARGET nlohmann_json::nlohmann_json)
        return()
    endif()
    FetchContent_Declare(nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS NAMES nlohmann_json
    )
    FetchContent_MakeAvailable(nlohmann_json)
endfunction()

# cpp-httplib (header-only): HTTP server/client for svc-cpp.
# Target: httplib::httplib.
function(gt_require_httplib)
    if(TARGET httplib::httplib)
        return()
    endif()
    set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
    # The non-blocking getaddrinfo path needs GetAddrInfoExCancel, which the
    # MinGW-w64 headers do not declare; the blocking path is portable.
    if(MINGW)
        set(HTTPLIB_USE_NON_BLOCKING_GETADDRINFO OFF CACHE BOOL "" FORCE)
    endif()
    FetchContent_Declare(httplib
        URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.58.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        FIND_PACKAGE_ARGS NAMES httplib
    )
    FetchContent_MakeAvailable(httplib)
endfunction()
