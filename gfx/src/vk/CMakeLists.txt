add_library(vk_exp STATIC 
    core.cpp 
    core.hpp
    draw.cpp
    draw.hpp
    game.cpp
    game.hpp
)

# Include directories
target_include_directories(vk_exp PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/.. 
    ${SDL2_INCLUDE_DIRS} 
    ${Vulkan_INCLUDE_DIRS} 
    ${CMAKE_SOURCE_DIR}/ext/glm
)

# Link libraries
target_link_libraries(vk_exp 
    common 
    ${SDL2_LIBRARIES} 
    ${Vulkan_LIBRARIES} 
    vk-bootstrap::vk-bootstrap
)