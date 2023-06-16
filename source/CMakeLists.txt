add_library(VulkanRenderer main.cpp)
target_link_libraries(VulkanRenderer
	PUBLIC Vulkan::Headers)
target_include_directories(VulkanRenderer
	INTERFACE .)