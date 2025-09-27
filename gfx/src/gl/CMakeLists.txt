add_library(stb INTERFACE 
    ${CMAKE_SOURCE_DIR}/ext/stb/stb_image.h
)

target_include_directories(stb INTERFACE ${CMAKE_SOURCE_DIR}/ext/stb)

# file(GLOB DATA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/data/*.c++")

add_library(gl_exp STATIC 
    core.cc
    debug.cc
    pbr.cc
    symbol.cc
)

# Include directories
target_include_directories(gl_exp PUBLIC 
    ${CMAKE_SOURCE_DIR}/include/gl
    ${CMAKE_SOURCE_DIR}/include/gl/data
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS} 
    ${CMAKE_SOURCE_DIR}/ext/glm
)

# Link libraries
target_link_libraries(gl_exp 
    common 
    ${SDL2_LIBRARIES} 
    ${OPENGL_LIBRARIES}
    glad
    stb
)
