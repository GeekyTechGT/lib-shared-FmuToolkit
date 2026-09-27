if(DEFINED GT_PROJECT_CONFIGURATION_INCLUDED)
    return()
endif()
set(GT_PROJECT_CONFIGURATION_INCLUDED TRUE)

if(NOT DEFINED PROJECT_IDENTIFIER)
    message(FATAL_ERROR "ProjectConfiguration.cmake requires PROJECT_IDENTIFIER to be set before inclusion.")
endif()

if(NOT DEFINED PROJECT_CATEGORY)
    message(FATAL_ERROR "ProjectConfiguration.cmake requires PROJECT_CATEGORY to be set before inclusion.")
endif()

# "container" is the legacy alias for "catalog", kept for compatibility with
# project.json files generated before the category rename.
if(NOT PROJECT_CATEGORY MATCHES "^(app|tool|lib-shared|lib-static|catalog|container|svc-cpp|app-qml)$")
    message(FATAL_ERROR "Unknown PROJECT_CATEGORY '${PROJECT_CATEGORY}'.")
endif()

string(TOUPPER "${PROJECT_IDENTIFIER}" PROJECT_IDENTIFIER_UPPER)

if(PROJECT_CATEGORY STREQUAL "lib-shared")
    set(PROJECT_LIBRARY_TYPE "SHARED")
elseif(PROJECT_CATEGORY STREQUAL "lib-static")
    set(PROJECT_LIBRARY_TYPE "STATIC")
else()
    set(PROJECT_LIBRARY_TYPE "")
endif()
