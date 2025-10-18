# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# project name and languages
project(TextFiler CXX)

# enable CTest
enable_testing()

# set output directory (build/)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

if(WIN32)
    # we don't want runtime DLLs
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # using Clang
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # using GCC
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
    elseif (MSVC)
        # replace "/MD" with "/MT" (building without runtime DLLs)
        set(CompilerFlags
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        foreach(CompilerFlags ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlags} "${${CompilerFlags}}")
        endforeach()
    endif()
endif()

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DNO_OLDNAMES)
endif()

##############################################################################

# libTextFiler.a
if(WIN32)
    add_library(libTextFiler STATIC TextFiler.cpp detail/TextFiler_impl.cpp detail/win_impl.cpp)
    target_compile_definitions(libTextFiler PRIVATE UNICODE _UNICODE)
else()
    add_library(libTextFiler STATIC TextFiler.cpp detail/TextFiler_impl.cpp detail/posix_impl.cpp)
    target_compile_definitions(libTextFiler PRIVATE)
endif()
set_target_properties(libTextFiler PROPERTIES PREFIX "")

# add tests
add_subdirectory(tests)
