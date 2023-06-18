add_library(VulkanRenderer main.cpp)
target_link_libraries(VulkanRenderer
	PUBLIC Vulkan::Headers
	PUBLIC glfw)
target_include_directories(VulkanRenderer
	INTERFACE .)