#include "device.hpp"

namespace vkr
{
	DeviceCreateInfo::DeviceCreateInfo()
	{
#ifdef VULKAN_RENDERER_DEBUG
		enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
	}

	Device::Device(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& createInfo, const QueueFamilyInfos& queueFamilyInfos)
		:vk::raii::Device{ getDevice(physicalDevice, createInfo, queueFamilyInfos) },
		queueFamilies{ *this, queueFamilyInfos }
	{
	}

	Device::Device(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& createInfo)
		:Device{ physicalDevice, createInfo, QueueFamilyInfos{physicalDevice} } {}

	Device::Device(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& createInfo, std::span<QueueRequirement> queueRequirements)
		:Device{ physicalDevice, createInfo, QueueFamilyInfos{physicalDevice, queueRequirements} } {}

	vk::raii::Device Device::getDevice(const vk::raii::PhysicalDevice& physicalDevice, const DeviceCreateInfo& deviceCreateInfo, const QueueFamilyInfos& queueFamilyInfos) const
	{
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = queueFamilyInfos;

		vk::DeviceCreateInfo createInfo;
		createInfo.setPEnabledLayerNames(deviceCreateInfo.enabledLayers);
		createInfo.setPEnabledExtensionNames(deviceCreateInfo.enabledExtensions);
		createInfo.setPEnabledFeatures(&deviceCreateInfo.enabledFeatures);
		createInfo.setQueueCreateInfos(queueCreateInfos);
		
		return vk::raii::Device{ physicalDevice, createInfo };
	}
}// namespace vkr