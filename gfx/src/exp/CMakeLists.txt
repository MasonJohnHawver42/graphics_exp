

set(EXECUTABLE_LIST
    helloworld 00_helloworld.cc
    window     01_window.cc
    cube       02_cube.cc
)

list(LENGTH EXECUTABLE_LIST len)
math(EXPR count "${len} / 2")
math(EXPR last_index "${count} - 1")

foreach(i RANGE 0 ${last_index})
    math(EXPR name_idx "2 * ${i}")
    math(EXPR file_idx "2 * ${i} + 1")

    list(GET EXECUTABLE_LIST ${name_idx} target)
    list(GET EXECUTABLE_LIST ${file_idx} file)

    add_executable(${target} ${file})
    target_include_directories(${target} PUBLIC ${CMAKE_SOURCE_DIR}/include/exp)
    target_link_libraries(${target} common gl_exp)
endforeach()