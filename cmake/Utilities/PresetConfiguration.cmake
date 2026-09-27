if(DEFINED GT_PRESET_CONFIGURATION_INCLUDED)
    return()
endif()
set(GT_PRESET_CONFIGURATION_INCLUDED TRUE)

function(gt_setup_preset_paths)
    # CMake's own Windows default (C:/Program Files (x86)/...) requires admin
    # rights. Only override when the user/preset did not set an explicit
    # CMAKE_INSTALL_PREFIX. (Install directories themselves come from
    # GNUInstallDirs, included by Install/InstallTargets.cmake.)
    if(WIN32 AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "C:/eng_apps/${PROJECT_NAME}" CACHE PATH "Install prefix" FORCE)
    endif()
endfunction()

gt_setup_preset_paths()
