macro(detectOS)
    
    IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
        message(STATUS "using Linux")
    ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
        message(STATUS "using Windows")
    ELSEIF (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        message(STATUS "using FreeBSD")
    ELSE ()
        MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
    ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    
endmacro(detectOS)


macro(detectCompiler)
    
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
        message(STATUS "using Clang")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
        message(STATUS "using GCC")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang++")
    # using Clang++
        message(STATUS "using Clang++")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
        message(STATUS "Intel C++")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
        message(STATUS "using Visual Studio C++")
    else()
        message(STATUS "using ${CMAKE_CXX_COMPILER_ID}")
    endif()
endmacro(detectCompiler)

