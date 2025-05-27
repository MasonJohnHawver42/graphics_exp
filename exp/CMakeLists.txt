
find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)

set(VENV_DIR "${CMAKE_CURRENT_BINARY_DIR}/venv")

if(WIN32)
    set(VENV_PY "${VENV_DIR}/Scripts/python.exe")
    set(VENV_PIP "${VENV_DIR}/Scripts/pip.exe")
else()
    set(VENV_PY "${VENV_DIR}/bin/python")
    set(VENV_PIP "${VENV_DIR}/bin/pip")
endif()

if(NOT EXISTS ${VENV_DIR})
    message(STATUS "Creating virtual environment in ${VENV_DIR}")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -m venv ${VENV_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE venv_result
    )
    if(NOT venv_result EQUAL 0)
        message(FATAL_ERROR "Failed to create virtual environment.")
    endif()
endif()

execute_process(
  COMMAND ${VENV_PY} -m pip install --upgrade pip
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE upgrade_pip_result
)
if(NOT upgrade_pip_result EQUAL 0)
  message(WARNING "Failed to upgrade pip in the virtual environment.")
endif()

execute_process(
  COMMAND ${VENV_PIP} install -r ${CMAKE_CURRENT_SOURCE_DIR}/amake/requirements.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE pip_install_result
)
if(NOT pip_install_result EQUAL 0)
  message(FATAL_ERROR "Failed to install Python dependencies from amake/requirements.txt")
endif()

add_custom_target(build_assets
  COMMAND ${VENV_PY} ${CMAKE_CURRENT_SOURCE_DIR}/Amake.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building Assets"
)

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
    target_link_libraries(${target} common gl_exp)
    add_dependencies(${target} build_assets)
endforeach()