add_subdirectory(core)

add_library(VulkanRenderer INTERFACE)

target_link_libraries(VulkanRenderer
	INTERFACE VulkanRenderer::core)