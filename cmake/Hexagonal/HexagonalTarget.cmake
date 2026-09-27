if(DEFINED GT_HEXAGONAL_TARGET_INCLUDED)
    return()
endif()
set(GT_HEXAGONAL_TARGET_INCLUDED TRUE)

# Collects sources of the 4 hexagonal layers (domain, application, adapter,
# config) plus any .cpp/.c file directly under CMAKE_CURRENT_SOURCE_DIR
# (main.cpp included). Also exports ${layer}_SOURCES per layer in the caller's
# scope.
function(gt_discover_hexagonal_sources sources_var)
    set(all_sources "")

    foreach(layer domain application adapter config)
        file(GLOB_RECURSE layer_sources CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${layer}/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/${layer}/*.c"
        )
        set(${layer}_SOURCES "${layer_sources}" PARENT_SCOPE)
        list(APPEND all_sources ${layer_sources})
    endforeach()

    file(GLOB root_sources CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    )
    list(APPEND all_sources ${root_sources})

    set(${sources_var} "${all_sources}" PARENT_SCOPE)
endfunction()

function(gt_add_hexagonal_target name)
    set(one_value_args CATEGORY ALIAS)
    set(multi_value_args SOURCES DOMAIN_SOURCES APPLICATION_SOURCES ADAPTER_SOURCES INCLUDE_DIRS)
    cmake_parse_arguments(ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(sources ${ARG_SOURCES})
    if(NOT sources)
        gt_discover_hexagonal_sources(sources)
    endif()
    list(APPEND sources ${ARG_DOMAIN_SOURCES} ${ARG_APPLICATION_SOURCES} ${ARG_ADAPTER_SOURCES})

    if(ARG_CATEGORY MATCHES "^(app|tool|catalog|container|svc-cpp|app-qml)$")
        add_executable(${name} ${sources})
    elseif(ARG_CATEGORY MATCHES "^lib-(shared|static)$")
        add_library(${name} ${PROJECT_LIBRARY_TYPE} ${sources})
    else()
        message(FATAL_ERROR "gt_add_hexagonal_target: unknown CATEGORY '${ARG_CATEGORY}'.")
    endif()

    gt_target_defaults(${name})
    if(COMMAND gt_enable_sanitizers)
        gt_enable_sanitizers(${name})
    endif()

    if(ARG_INCLUDE_DIRS)
        target_include_directories(${name} PRIVATE ${ARG_INCLUDE_DIRS})
    endif()

    if(ARG_CATEGORY MATCHES "^lib-(shared|static)$" AND ARG_ALIAS)
        gt_add_namespace_alias(${name} ${ARG_ALIAS})
    endif()

    set_target_properties(${name} PROPERTIES
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
    )
endfunction()
