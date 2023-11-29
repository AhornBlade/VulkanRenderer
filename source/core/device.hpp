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
			std::span<const vk::DeviceQueueCreateInfo> queueCreateInfos);

		Device(const vk::raii::PhysicalDevice& physicalDevice,
			const DeviceCreateInfo& createInfo,
			const std::vector<vk::DeviceQueueCreateInfo>& queueCreateInfos);

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
			std::span<const vk::DeviceQueueCreateInfo> queueCreateInfos) const;
	};

	//if sizeof...(args) is 0, it's GPU only mode
	template<class ... Ts>
	Device createDevice(const vk::raii::PhysicalDevice& physicalDevice, auto&& ... args)
		requires requires{ Device{ physicalDevice, DeviceCreateInfo{}, args... }; }
	{
		DeviceCreateInfo createInfo{};
		(Ts::setDeviceCreateInfo(createInfo), ...);
		return Device{ physicalDevice, createInfo, args... };
	}

}// namespace vkr