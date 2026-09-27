if(DEFINED GT_COVERAGE_INCLUDED)
    return()
endif()
set(GT_COVERAGE_INCLUDED TRUE)

# Code coverage instrumentation (gcov format), switched on by the
# *-coverage presets. scripts/prj_exec.py command 18 builds with that preset,
# runs the tests and turns the .gcda/.gcno files into a gcovr HTML report.
option(${PROJECT_IDENTIFIER_UPPER}_ENABLE_COVERAGE "Instrument targets for code coverage (GCC/Clang)" OFF)

# Called by gt_target_defaults() for every target, so production code and
# tests are instrumented consistently.
function(gt_enable_coverage target)
    if(NOT ${PROJECT_IDENTIFIER_UPPER}_ENABLE_COVERAGE)
        return()
    endif()
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(WARNING "Coverage requested but only GCC/Clang are supported - '${target}' not instrumented.")
        return()
    endif()
    target_compile_options(${target} PRIVATE --coverage -O0 -g)
    target_link_options(${target} PRIVATE --coverage)
endfunction()
