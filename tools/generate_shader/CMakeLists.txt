add_executable(generate_shader main.cpp)
target_link_libraries(generate_shader
    PRIVATE spirv-reflect-wrapper)