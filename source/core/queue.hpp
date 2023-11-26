#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace vkr
{
	struct QueueRequirement
	{
		vk::QueueFlags queueType;
		uint32_t queueCount = 1;
		float priority = 1.0f;
		bool required = false;
	};

	struct QueueFamilyInfo
	{
		uint32_t queueFamilyIndex;
		std::vector<float> queuePriorities;

		operator vk::DeviceQueueCreateInfo() const;
	};

	class QueueFamilyInfos : public std::vector<QueueFamilyInfo>
	{
	public:
		using std::vector<QueueFamilyInfo>::vector;

		//GPU only mode
		QueueFamilyInfos(const vk::raii::PhysicalDevice& physicalDevice);
		QueueFamilyInfos(const vk::raii::PhysicalDevice& physicalDevice,
			std::span<QueueRequirement> queueRequirements);

		operator std::vector<vk::DeviceQueueCreateInfo>() const;
	};

}; // namespace vkr