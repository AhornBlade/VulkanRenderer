#include <vulkan/vulkan_raii.hpp>

#include <RenderApplicationBase.hpp>

#include <iostream>

namespace vkr = ::vk::raii;

class VulkanApplication : public vkrenderer::RendererApplicationBase
{
public:
	VulkanApplication()
	{
		createInstance();
	}
	
	virtual void initialize() override
	{
		
	}

	virtual bool shouldClose() override
	{
		return true;
	}

	virtual void loop() override
	{

	}

private:
	vkr::Context context{};
	vkr::Instance instance{nullptr};
	vkr::DebugUtilsMessengerEXT debugMessenger{nullptr};

	void createInstance()
	{
		std::vector<const char*> enabledExtensions{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME
		};
		std::vector<const char*> enabledLayers{
			"VK_LAYER_KHRONOS_validation"
		};

		vk::DebugUtilsMessengerCreateInfoEXT debugInfo{};
		debugInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo);
		debugInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		debugInfo.setPfnUserCallback([](
			VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)->VkBool32
			{
				if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
				{
					std::cout << "Info: " << pCallbackData->pMessage << '\n';
				}
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				{
					std::cout << "Warning: " << pCallbackData->pMessage << '\n';
				}
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				{
					std::cout << "Error: " << pCallbackData->pMessage << '\n';
				}

				return VK_FALSE;
			});

		vk::InstanceCreateInfo createInfo{};
		createInfo.setPEnabledExtensionNames({ static_cast<uint32_t>(enabledExtensions.size()), enabledExtensions.data() });
		createInfo.setPEnabledLayerNames({ static_cast<uint32_t>(enabledLayers.size()), enabledLayers.data() });
		createInfo.setPNext(&debugInfo);
		
		instance = vkr::Instance{ context, createInfo };
		
		debugMessenger = instance.createDebugUtilsMessengerEXT(debugInfo);
	}
};

loadExternClass(VulkanApplication);