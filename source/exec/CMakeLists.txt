add_library(VulkanRenderer-exec INTERFACE)
add_library(VulkanRenderer::exec ALIAS VulkanRenderer-exec)

target_include_directories(VulkanRenderer-exec
	INTERFACE ..)