add_executable(test test.cpp)
target_link_libraries(test
	PUBLIC Vulkan::Headers)
	
add_executable(test_core test_core.cpp)
target_link_libraries(test_core
	PUBLIC VulkanRenderer::core)

add_executable(test_exec test_exec.cpp)
target_link_libraries(test_exec
	PUBLIC VulkanRenderer::exec
	PUBLIC Catch2::Catch2WithMain)

add_subdirectory(test_generate_shader)