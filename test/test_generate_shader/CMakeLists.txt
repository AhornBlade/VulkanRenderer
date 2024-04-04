add_executable(test_generate_shader main.cpp)

target_link_libraries(test_generate_shader
    glm::glm)

GENERATE_SHADERS(shaders test_generate_shader)