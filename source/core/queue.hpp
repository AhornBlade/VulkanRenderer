#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace vkr
{
	struct QueueRequirement
	{
		vk::QueueFlags queueType;
		uint32_t queueCount = 1;
		float priority = 0.5f;
		bool required = false;
	};

	struct QueueFamilyInfo
	{
		uint32_t queueFamilyIndex;
		std::vector<float> queuePriorities;

		operator vk::DeviceQueueCreateInfo() const&;
		operator vk::DeviceQueueCreateInfo() && = delete;
	};

	class QueueFamilyInfos : public std::vector<QueueFamilyInfo>
	{
	public:
		using std::vector<QueueFamilyInfo>::vector;

		//GPU only mode
		explicit QueueFamilyInfos(const vk::raii::PhysicalDevice& physicalDevice);

		QueueFamilyInfos(const vk::raii::PhysicalDevice& physicalDevice,
			std::span<QueueRequirement> queueRequirements);

		operator std::vector<vk::DeviceQueueCreateInfo>() const&;
		operator std::vector<vk::DeviceQueueCreateInfo>() && = delete;
	};

	class Queue : public vk::raii::Queue
	{
	public:
		using vk::raii::Queue::Queue;
	};

	class QueueFamily : public std::vector<Queue>
	{
	public:
		QueueFamily(const vk::raii::Device& device, const vk::DeviceQueueCreateInfo& queueCreateInfo);

		inline uint32_t getQueueFamilyIndex() const noexcept { return queueFamilyIndex; }

	private:
		uint32_t queueFamilyIndex;
	};

	class QueueFamilies : public std::vector<QueueFamily>
	{
	public:
		QueueFamilies(const vk::raii::Device& device, std::span<const vk::DeviceQueueCreateInfo> queueCreateInfos);

	};

}; // namespace vkr