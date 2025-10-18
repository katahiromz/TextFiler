# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# project name and languages
project(TextFiler CXX)

# enable CTest
enable_testing()

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

##############################################################################

# libTextFiler.a
if(WIN32)
    add_library(libTextFiler STATIC TextFiler.cpp TextFiler_impl.cpp win_impl.cpp)
    target_compile_definitions(libTextFiler PRIVATE UNICODE _UNICODE NO_OLDNAMES)
else()
    add_library(libTextFiler STATIC TextFiler.cpp TextFiler_impl.cpp posix_impl.cpp)
    target_compile_definitions(libTextFiler PRIVATE)
endif()

##############################################################################

# add tests
add_subdirectory(tests)
