#include "device.hpp"

#include <iostream>

namespace vkr
{
	DeviceCreateInfo::DeviceCreateInfo()
	{
#ifdef VULKAN_RENDERER_DEBUG
		enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
	}

	Device::Device(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& createInfo, std::span<const vk::DeviceQueueCreateInfo> queueCreateInfos)
		:vk::raii::Device{ getDevice(physicalDevice, createInfo, queueCreateInfos) },
		queueFamilies{ *this, queueCreateInfos }
	{
		std::cout << "Success to create device\n";
	}

	Device::Device(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& createInfo, const std::vector<vk::DeviceQueueCreateInfo>& queueCreateInfos)
		:Device{ physicalDevice, createInfo, static_cast<std::span<const vk::DeviceQueueCreateInfo>>(queueCreateInfos) } {}

	Device::Device(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& createInfo, const QueueFamilyInfos& queueFamilyInfos)
		:Device{ physicalDevice, createInfo, static_cast<std::vector<vk::DeviceQueueCreateInfo>>(queueFamilyInfos) } {}

	Device::Device(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& createInfo)
		:Device{ physicalDevice, createInfo, QueueFamilyInfos{physicalDevice} } {}

	Device::Device(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& createInfo, std::span<QueueRequirement> queueRequirements)
		:Device{ physicalDevice, createInfo, QueueFamilyInfos{physicalDevice, queueRequirements} } {}

	vk::raii::Device Device::getDevice(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& deviceCreateInfo, std::span<const vk::DeviceQueueCreateInfo> queueCreateInfos) const
	{
		vk::DeviceCreateInfo createInfo;
		createInfo.setPEnabledLayerNames(deviceCreateInfo.enabledLayers);
		createInfo.setPEnabledExtensionNames(deviceCreateInfo.enabledExtensions);
		createInfo.setPEnabledFeatures(&deviceCreateInfo.enabledFeatures);
		createInfo.setQueueCreateInfos(queueCreateInfos);

		return vk::raii::Device{ physicalDevice, createInfo };
	}

}// namespace vkr