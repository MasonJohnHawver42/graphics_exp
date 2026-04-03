cmake_minimum_required(VERSION 3.16)
project(gfx_exp)

# --- C++ standard ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- All targets static -------------------------------------------------------
set(BUILD_SHARED_LIBS OFF)

# --- Module path --------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/gfx/cmake")

# --- Output layout inside BUILD_CM (set by -B build/BUILD_CM) ----------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Install prefix: build/INSTALL/ ------------------------------------------
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/build/INSTALL"
    CACHE PATH "Install prefix" FORCE)

# --- Platform macros ----------------------------------------------------------
if(WIN32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
elseif(APPLE)
    add_definitions(-DPLATFORM_MAC)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    message(STATUS "Debug build -- DEBUG macro defined")
endif()

# --- System packages ----------------------------------------------------------
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Python3 3.9 COMPONENTS Interpreter REQUIRED)

# --- Protobuf (static, built into the dev container image) -------------------
# protobuf 3.21.12 is compiled from source in ctx/Containerfile.dev and
# installed to /usr/local. This provides both protoc (used by generate_proto.sh)
# and libprotobuf.a (linked statically here so binaries run on the host
# without needing any container libprotobuf.so).
set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf REQUIRED)

# --- zlib (submodule at gfx/ext/zlib) — static only --------------------------
# Exports: zlibstatic target and ZLIB::ZLIBSTATIC alias
set(ZLIB_BUILD_SHARED   OFF CACHE BOOL "Disable zlib shared"   FORCE)
set(ZLIB_BUILD_STATIC   ON  CACHE BOOL "Enable zlib static"    FORCE)
set(ZLIB_BUILD_TESTING  OFF CACHE BOOL "Disable zlib tests"    FORCE)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Disable zlib examples" FORCE)
set(ZLIB_INSTALL        OFF CACHE BOOL "Disable zlib install"  FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/gfx/ext/zlib)

# --- glad (submodule at gfx/ext/glad) ----------------------------------------
add_subdirectory(${CMAKE_SOURCE_DIR}/gfx/ext/glad)

# --- stb (header-only) -------------------------------------------------------
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_SOURCE_DIR}/gfx/ext/stb)

include(FetchContent)

# --- cpptrace (debug only, via FetchContent) ----------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CPPTRACE_GET_SYMBOLS_WITH_LIBDWARF OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_GET_SYMBOLS_WITH_ADDR2LINE ON  CACHE BOOL "" FORCE)
    set(CPPTRACE_GET_SYMBOLS_WITH_LIBDL    OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_DEMANGLE_WITH_CXXABI      ON  CACHE BOOL "" FORCE)
    set(CPPTRACE_USE_EXTERNAL_LIBDWARF     OFF CACHE BOOL "" FORCE)
    set(CPPTRACE_INSTALL                   OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        cpptrace
        GIT_REPOSITORY https://github.com/jeremy-rifkin/cpptrace.git
        GIT_TAG        v0.8.3
    )
    FetchContent_MakeAvailable(cpptrace)
    # Prevent cpptrace from adding install rules to our prefix
    if(cpptrace_SOURCE_DIR)
        set_property(DIRECTORY "${cpptrace_SOURCE_DIR}" PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()
endif()

# =============================================================================
# 1. Protobuf codegen target
# =============================================================================
set(PROTO_DIR     ${CMAKE_SOURCE_DIR}/gfx/idl)
set(PROTO_HDR_OUT ${CMAKE_SOURCE_DIR}/gfx/include/proto)
set(PROTO_SRC_OUT ${CMAKE_SOURCE_DIR}/gfx/src/proto)

set(PROTO_NAMES shader debug_primitives pbr_meta)
set(PROTO_SRCS)
set(PROTO_HDRS)
foreach(name IN LISTS PROTO_NAMES)
    list(APPEND PROTO_SRCS ${PROTO_SRC_OUT}/${name}.pb.cc)
    list(APPEND PROTO_HDRS ${PROTO_HDR_OUT}/${name}.pb.h)
endforeach()

add_custom_target(GenerateProto
    COMMAND bash ${CMAKE_SOURCE_DIR}/.scripts/generate_proto.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    BYPRODUCTS ${PROTO_SRCS} ${PROTO_HDRS}
    COMMENT "Generating protobuf C++ and Python code..."
)
set_source_files_properties(${PROTO_SRCS} PROPERTIES GENERATED TRUE)

# =============================================================================
# 2. gfx_proto -- compiled proto messages (STATIC)
# =============================================================================
add_library(gfx_proto STATIC ${PROTO_SRCS})
add_dependencies(gfx_proto GenerateProto)
target_include_directories(gfx_proto PUBLIC
    ${PROTO_HDR_OUT}
)
# protobuf::libprotobuf carries its own include dirs via INTERFACE_INCLUDE_DIRECTORIES
target_link_libraries(gfx_proto PUBLIC protobuf::libprotobuf)

# =============================================================================
# 3. gfx_common -- platform utilities (STATIC)
# =============================================================================
add_library(gfx_common STATIC
    ${CMAKE_SOURCE_DIR}/gfx/src/common/log.cc
    ${CMAKE_SOURCE_DIR}/gfx/src/common/mem.cc
    ${CMAKE_SOURCE_DIR}/gfx/src/common/sys.cc
    ${CMAKE_SOURCE_DIR}/gfx/src/common/struct.cc
    $<$<PLATFORM_ID:Linux>:${CMAKE_SOURCE_DIR}/gfx/src/common/linux/sys.cc>
    $<$<PLATFORM_ID:Windows>:${CMAKE_SOURCE_DIR}/gfx/src/common/win/sys.cc>
)
target_include_directories(gfx_common PUBLIC
    ${CMAKE_SOURCE_DIR}/gfx/include/common
)
target_link_libraries(gfx_common PUBLIC zlibstatic)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(gfx_common PUBLIC cpptrace::cpptrace)
    target_compile_options(gfx_common PUBLIC -g)
    target_link_options(gfx_common PUBLIC -rdynamic)
endif()

# =============================================================================
# 4. gfx_gl -- OpenGL renderer (STATIC)
# =============================================================================
add_library(gfx_gl STATIC
    ${CMAKE_SOURCE_DIR}/gfx/src/gl/core.cc
    ${CMAKE_SOURCE_DIR}/gfx/src/gl/debug.cc
    ${CMAKE_SOURCE_DIR}/gfx/src/gl/pbr.cc
    ${CMAKE_SOURCE_DIR}/gfx/src/gl/symbol.cc
)
target_include_directories(gfx_gl PUBLIC
    ${CMAKE_SOURCE_DIR}/gfx/include/gl
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/gfx/ext/glm
)
target_link_libraries(gfx_gl PUBLIC
    gfx_common
    glad
    stb
    gfx_proto
    ${OPENGL_LIBRARIES}
)

# =============================================================================
# 5. Example executables
# =============================================================================
add_executable(helloworld ${CMAKE_SOURCE_DIR}/gfx/src/exp/00_helloworld.cc)
add_executable(window     ${CMAKE_SOURCE_DIR}/gfx/src/exp/01_window.cc)
add_executable(cube       ${CMAKE_SOURCE_DIR}/gfx/src/exp/02_cube.cc)

foreach(target IN ITEMS helloworld window cube)
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}/gfx/include/exp
    )
    target_link_libraries(${target} PRIVATE
        gfx_gl
        ${SDL2_LIBRARIES}
    )
endforeach()

# =============================================================================
# 6. Asset pipeline -- wheel -> venv -> Amake.py
#
# Stamp-file ordering:
#   AMAKE_WHEEL_STAMP -> VENV_STAMP -> GenerateAssets
# =============================================================================
set(BUILD_AM_DIR      ${CMAKE_SOURCE_DIR}/build/BUILD_AM)
set(VENV_DIR          ${BUILD_AM_DIR}/.venv)
set(VENV_PYTHON       ${VENV_DIR}/bin/python)
set(VENV_STAMP        ${VENV_DIR}/.installed)
set(REQUIREMENTS_FILE ${CMAKE_SOURCE_DIR}/sbx/requirements.txt)

set(AMAKE_PKG_DIR     ${CMAKE_SOURCE_DIR}/sbx/amake)
set(AMAKE_DIST_DIR    ${AMAKE_PKG_DIR}/dist)
set(AMAKE_WHEEL_STAMP ${BUILD_AM_DIR}/.amake_wheel_built)

# Track all Python sources so the wheel rebuilds when any .py file changes.
file(GLOB_RECURSE AMAKE_PY_SOURCES
    "${CMAKE_SOURCE_DIR}/sbx/amake/*.py"
    "${CMAKE_SOURCE_DIR}/sbx/amake/pyproject.toml"
    "${CMAKE_SOURCE_DIR}/sbx/amake/setup.py"
    "${CMAKE_SOURCE_DIR}/sbx/Amake.py"
)

# 6a. Build the GfxExpAmake wheel into sbx/amake/dist/
add_custom_command(
    OUTPUT  ${AMAKE_WHEEL_STAMP}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_AM_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${AMAKE_PKG_DIR}/build
    # Set HOME to BUILD_AM_DIR so pip's user-install (.local) and cache (.cache)
    # land inside build/ (gitignored) rather than the repo root.
    COMMAND ${CMAKE_COMMAND} -E env HOME=${BUILD_AM_DIR}
            ${Python3_EXECUTABLE} -m pip install build --quiet
    COMMAND ${CMAKE_COMMAND} -E env HOME=${BUILD_AM_DIR}
            ${Python3_EXECUTABLE} -m build --wheel ${AMAKE_PKG_DIR}
            --outdir ${AMAKE_DIST_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${AMAKE_WHEEL_STAMP}
    DEPENDS ${AMAKE_PY_SOURCES}
    COMMENT "Building GfxExpAmake wheel..."
    VERBATIM
)
add_custom_target(BuildAmakeWheel DEPENDS ${AMAKE_WHEEL_STAMP})

# 6b. Create venv and install the wheel + requirements
add_custom_command(
    OUTPUT  ${VENV_STAMP}
    COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
    COMMAND ${VENV_PYTHON} -m pip install --upgrade pip --quiet
    COMMAND ${VENV_PYTHON} -m pip install --find-links ${AMAKE_DIST_DIR} --force-reinstall -r ${REQUIREMENTS_FILE} --quiet
    COMMAND ${CMAKE_COMMAND} -E touch ${VENV_STAMP}
    DEPENDS ${AMAKE_WHEEL_STAMP}
    COMMENT "Setting up Python venv with GfxExpAmake..."
    VERBATIM
)
add_custom_target(SetupVenv ALL DEPENDS ${VENV_STAMP})
add_dependencies(SetupVenv BuildAmakeWheel)

# 6c. Run Amake.py -- outputs to build/BUILD_AM/
add_custom_target(GenerateAssets
    # Step 1: run Amake.py — writes the Makefile into build/BUILD_AM/
    COMMAND ${VENV_PYTHON} ${CMAKE_SOURCE_DIR}/sbx/Amake.py
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/sbx
    # Step 2: run the generated Makefile — produces build/BUILD_AM/assets/
    COMMAND make -j8 -C ${BUILD_AM_DIR}
    DEPENDS SetupVenv
    COMMENT "Generating assets -> build/BUILD_AM/assets/ ..."
)

add_dependencies(helloworld GenerateAssets)
add_dependencies(window     GenerateAssets)
add_dependencies(cube       GenerateAssets)

# =============================================================================
# 7. Install rules -> build/INSTALL/
#    Run: cmake --install build/BUILD_CM
# =============================================================================

# Core static libs -> build/INSTALL/core/lib/
install(TARGETS gfx_proto  ARCHIVE DESTINATION core/lib)
install(TARGETS gfx_common ARCHIVE DESTINATION core/lib)
install(TARGETS gfx_gl     ARCHIVE DESTINATION core/lib)

# Core headers -> build/INSTALL/core/include/
install(DIRECTORY ${CMAKE_SOURCE_DIR}/gfx/include/common/ DESTINATION core/include/common)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/gfx/include/gl/     DESTINATION core/include/gl)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/gfx/include/proto/  DESTINATION core/include/proto)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/gfx/include/exp/    DESTINATION core/include/exp)

# Example binaries -> build/INSTALL/examples/bin/
install(TARGETS helloworld window cube RUNTIME DESTINATION examples/bin)

# Amake wheel -> build/INSTALL/core/py/
install(DIRECTORY ${AMAKE_DIST_DIR}/
    DESTINATION core/py
    FILES_MATCHING PATTERN "gfxexpamake-*.whl"
)

# Symlink: build/INSTALL/assets -> ../../BUILD_AM
# Symlink: build/INSTALL/assets -> ../../BUILD_AM/assets
# Relative from build/INSTALL/assets: two levels up = build/, then BUILD_AM/assets
install(CODE "
    set(_link \"\${CMAKE_INSTALL_PREFIX}/assets\")
    set(_target \"../../BUILD_AM/assets\")
    if(NOT EXISTS \"\${_link}\")
        execute_process(
            COMMAND \${CMAKE_COMMAND} -E create_symlink \${_target} \${_link}
        )
        message(STATUS \"Created symlink: \${_link} -> \${_target}\")
    endif()
")

# =============================================================================
# 8. CPack -- RPM for the cube example
#    Run from build/BUILD_CM: cpack -G RPM
# =============================================================================
set(CPACK_GENERATOR                   "TGZ;RPM")
set(CPACK_PACKAGE_NAME                "cube-example")
set(CPACK_PACKAGE_VERSION             "0.1.0")
set(CPACK_PACKAGE_RELEASE             "1")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OpenGL cube rendering demo")
set(CPACK_PACKAGE_VENDOR              "gfx_exp")
set(CPACK_RPM_PACKAGE_LICENSE         "MIT")
set(CPACK_RPM_PACKAGE_GROUP           "Applications/Graphics")
set(CPACK_RPM_COMPONENT_INSTALL       ON)
set(CPACK_COMPONENTS_ALL              cube)

# Absolute path so the RPM lands in build/PACKAGE/examples/ regardless of cwd
set(CPACK_OUTPUT_FILE_PREFIX          "${CMAKE_SOURCE_DIR}/build/PACKAGE/examples")

set(PACKAGING_DIR ${CMAKE_SOURCE_DIR}/ctx/packaging)

# Real binary -> /usr/libexec/cube-example/cube
install(TARGETS cube
    RUNTIME DESTINATION libexec/cube-example
    COMPONENT cube
)

# Wrapper script -> /usr/bin/cube-example  (sets CWD to assets dir before exec)
install(PROGRAMS ${PACKAGING_DIR}/cube-example
    DESTINATION bin
    COMPONENT cube
)

# Desktop entry -> /usr/share/applications/
install(FILES ${PACKAGING_DIR}/cube-example.desktop
    DESTINATION share/applications
    COMPONENT cube
)

# Assets -> /usr/share/cube-example/assets/  (resolve all symlinks; relative links
# from RelativeLinkingTool would break on a foreign install root otherwise).
# file(COPY) does not honour DESTDIR automatically, so we prepend $ENV{DESTDIR}
# explicitly — CPack uses DESTDIR to stage files into its temp tree.
install(CODE "
    message(STATUS \"Copying assets (resolving symlinks)...\")
    file(COPY \"${BUILD_AM_DIR}/assets/\"
        DESTINATION \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/share/cube-example/assets\"
        FOLLOW_SYMLINK_CHAIN
    )
" COMPONENT cube)

cpack_add_component(cube
    DISPLAY_NAME "Cube Example"
    DESCRIPTION  "OpenGL cube renderer demo with built assets"
)

include(CPack)

# =============================================================================
# 9. Convenience run targets
# =============================================================================
foreach(target IN ITEMS helloworld window cube)
    add_custom_target(run.${target}
        COMMAND ${CMAKE_BINARY_DIR}/bin/${target}
        DEPENDS ${target}
        WORKING_DIRECTORY ${BUILD_AM_DIR}
        COMMENT "Running ${target}..."
    )
endforeach()
