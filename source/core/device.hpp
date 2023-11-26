#pragma once

#include "queue.hpp"

namespace vkr
{
	struct DeviceCreateInfo
	{
		DeviceCreateInfo();

		std::vector<const char*> enabledLayers;
		std::vector<const char*> enabledExtensions;
		vk::PhysicalDeviceFeatures enabledFeatures;
	};

	class Device : public vk::raii::Device
	{
	public:
		Device(const vk::raii::PhysicalDevice& physicalDevice,
			const DeviceCreateInfo& createInfo,
			const QueueFamilyInfos& queueFamilyInfos);

		//GPU only mode
		Device(const vk::raii::PhysicalDevice& physicalDevice,
			const DeviceCreateInfo& createInfo);

		Device(const vk::raii::PhysicalDevice& physicalDevice,
			const DeviceCreateInfo& createInfo,
			std::span<QueueRequirement> queueRequirements);

	private:
		QueueFamilies queueFamilies;

		vk::raii::Device getDevice(const vk::raii::PhysicalDevice& physicalDevice,
			const DeviceCreateInfo& createInfo,
			const QueueFamilyInfos& queueFamilyInfos) const;
	};

	template<class ... Ts>
	Device createDevice(const vk::raii::PhysicalDevice& physicalDevice, std::span<QueueRequirement> queueRequirements)
	{
		DeviceCreateInfo createInfo{};
		(Ts::setDeviceCreateInfo(createInfo), ...);
		return Device{ physicalDevice, createInfo, queueRequirements };
	}

	//GPU only mode
	template<class ... Ts>
	Device createDevice(const vk::raii::PhysicalDevice& physicalDevice)
	{
		DeviceCreateInfo createInfo{};
		(Ts::setDeviceCreateInfo(createInfo), ...);
		return Device{ physicalDevice, createInfo };
	}

}// namespace vkr