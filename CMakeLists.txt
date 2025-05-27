cmake_minimum_required(VERSION 3.13)
project(vk_exp)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")

# Append cmake/ to the module search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Output directories for binaries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find SDL2 package
find_package(SDL2 REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find OpenGL
cmake_policy(SET CMP0072 NEW)
find_package(OpenGL REQUIRED)

# Add Glad
add_subdirectory(${CMAKE_SOURCE_DIR}/ext/glad)

# Add VkBootstrap
# add_subdirectory(${CMAKE_SOURCE_DIR}/ext/vk-bootstrap)

# Add VkMA
# add_subdirectory(${CMAKE_SOURCE_DIR}/ext/vma)

include(FetchContent)

# Add Zlib
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "Disable shared" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "Enable static" FORCE)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Disable building zlib examples" FORCE)

FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG        v1.3.1
)

FetchContent_MakeAvailable(zlib)
add_library(ZLIB::ZLIB ALIAS zlib)

# Add cpptrace with debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CPPTRACE_GET_SYMBOLS_WITH_LIBDWARF OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_GET_SYMBOLS_WITH_ADDR2LINE OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_GET_SYMBOLS_WITH_LIBDL ON CACHE BOOL "" FORCE)
    set(CPPTRACE_DEMANGLE_WITH_CXXABI ON CACHE BOOL "" FORCE)

    set(CPPTRACE_USE_EXTERNAL_LIBDWARF OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_INSTALL OFF CACHE BOOL "" FORCE)

    set(CPPTRACE_GET_SYMBOLS_WITH_LIBDL OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_GET_SYMBOLS_WITH_ADDR2LINE ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        cpptrace
        GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
        GIT_TAG        v0.8.3
    )

    FetchContent_MakeAvailable(cpptrace)
endif()

# Set platform-specific macros for C++ code
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DPLATFORM_MAC)
endif()

# Add DEBUG macro for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    message(STATUS "Building in Debug mode, defining DEBUG macro")
endif()

# common
add_subdirectory(${CMAKE_SOURCE_DIR}/src/common)

# renderers
add_subdirectory(${CMAKE_SOURCE_DIR}/src/gl)
# add_subdirectory(${CMAKE_SOURCE_DIR}/src/vk)

# examples
add_subdirectory(${CMAKE_SOURCE_DIR}/exp)