#include "instance.hpp"

#include <iostream>

namespace vkr
{

	Instance::Instance(std::span<const char* const> enabledLayers, std::span<const char* const> enabledExtensions)
		:context{},
		instance{ getInstance(enabledLayers,enabledExtensions) },
#ifdef VULKAN_RENDERER_DEBUG
		debugMessenger{ instance, getDebugUtilsCreateInfo() }
#else
		debugMessenger{ nullptr }
#endif
	{
	}

	Instance::Instance(const InstanceCreateInfo& createInfo)
		:Instance{ createInfo.enabledLayers, createInfo.enabledExtensions } {}

	vk::raii::Instance Instance::getInstance(
		std::span<const char* const> enabledLayers, 
		std::span<const char* const> enabledExtensions)const
	{
		vk::InstanceCreateInfo createInfo;
		createInfo.setPEnabledLayerNames(enabledLayers);
		createInfo.setPEnabledExtensionNames(enabledExtensions);
#ifdef VULKAN_RENDERER_DEBUG
		auto debugInfo = getDebugUtilsCreateInfo();
		createInfo.setPNext(&debugInfo);
#endif
		return vk::raii::Instance{ context, createInfo };
	}

	vk::DebugUtilsMessengerCreateInfoEXT Instance::getDebugUtilsCreateInfo() const
	{
		vk::DebugUtilsMessengerCreateInfoEXT debugInfo{};
		debugInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
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
		return debugInfo;
	}

	InstanceCreateInfo getDefaultInstanceCreateInfo()
	{
		InstanceCreateInfo createInfo;
#ifdef VULKAN_RENDERER_DEBUG
		createInfo.enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
		createInfo.enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
		return createInfo;
	}

}; //namesapce vkr