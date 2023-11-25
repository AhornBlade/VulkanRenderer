#include "instance.hpp"

#include <iostream>
#include <ranges>

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

	vk::raii::PhysicalDevice Instance::getPhysicalDevice(uint32_t index) const
	{
		vk::raii::PhysicalDevices physicalDevices{ instance };

		return physicalDevices[index];
	}

	vk::raii::PhysicalDevice Instance::getPhysicalDevice(std::function<uint32_t(const vk::PhysicalDeviceProperties&)> score) const
	{
		vk::raii::PhysicalDevices physicalDevices{ instance };

		return std::ranges::max(physicalDevices, {},
			[&score](const vk::raii::PhysicalDevice& physicalDevice)-> uint32_t
			{
				return score(physicalDevice.getProperties());
			});
	}

	vk::raii::PhysicalDevice Instance::getPhysicalDevice() const
	{
		return getPhysicalDevice([](const vk::PhysicalDeviceProperties& properties) -> uint32_t
			{
				uint32_t score = 0;
				switch (properties.deviceType)
				{
				case vk::PhysicalDeviceType::eDiscreteGpu:
					score += 100;
					break;
				case vk::PhysicalDeviceType::eIntegratedGpu:
					score += 50;
				case vk::PhysicalDeviceType::eCpu:
					score += 20;
				default:
					break;
				}
				return score;
			});
	}

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