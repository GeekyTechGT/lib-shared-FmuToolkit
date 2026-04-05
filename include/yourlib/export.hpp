#pragma once

#if defined(_WIN32) && defined(YOUR_LIBRARY_SHARED)
#    if defined(YOUR_LIBRARY_BUILDING_SHARED)
#        define YOUR_LIBRARY_API __declspec(dllexport)
#    else
#        define YOUR_LIBRARY_API __declspec(dllimport)
#    endif
#else
#    define YOUR_LIBRARY_API
#endif
