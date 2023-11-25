
add_executable(test test.cpp)
target_link_libraries(test
	PUBLIC Vulkan::Headers)
	
add_executable(test_core test_core.cpp)
target_link_libraries(test_core
	PUBLIC VulkanRenderer::core)
