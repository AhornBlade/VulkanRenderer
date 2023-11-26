add_library(VulkanRenderer-core instance.cpp   "queue.cpp" "device.cpp")
add_library(VulkanRenderer::core ALIAS VulkanRenderer-core)

target_link_libraries(VulkanRenderer-core
	PUBLIC Vulkan::Headers)

target_include_directories(VulkanRenderer-core
	INTERFACE ..)