add_library(common STATIC 
    log.cc
    sys.cc
    mem.cc
)

if(UNIX AND NOT APPLE)
    target_sources(common PRIVATE linux/sys.cc)
elseif(WIN32)
    target_sources(common PRIVATE win/sys.cc)
endif()

target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR}/include/common)

target_link_libraries(common PUBLIC 
    zlib
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(common PUBLIC cpptrace::cpptrace)
    target_compile_definitions(common PUBLIC CPPTRACE_GET_SYMBOLS_WITH_LIBDL)
    target_compile_options(common PUBLIC -g)
    target_link_options(common PUBLIC "-rdynamic")
endif()