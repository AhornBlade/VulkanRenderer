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

		operator vk::DeviceQueueCreateInfo() const;
	};

	class QueueFamilyInfos : public std::vector<QueueFamilyInfo>
	{
	public:
		using std::vector<QueueFamilyInfo>::vector;

		//GPU only mode
		explicit QueueFamilyInfos(const vk::raii::PhysicalDevice& physicalDevice);

		QueueFamilyInfos(const vk::raii::PhysicalDevice& physicalDevice,
			std::span<QueueRequirement> queueRequirements);

		operator std::vector<vk::DeviceQueueCreateInfo>() const;
	};

	class Queue : public vk::raii::Queue
	{
	public:
		using vk::raii::Queue::Queue;
	};

	class QueueFamily : public std::vector<Queue>
	{
	public:
		QueueFamily(const vk::raii::Device& device, const QueueFamilyInfo& queueFamilyInfo);

		inline uint32_t getQueueFamilyIndex() const noexcept { return queueFamilyIndex; }

	private:
		uint32_t queueFamilyIndex;
	};

	class QueueFamilies : public std::vector<QueueFamily>
	{
	public:
		QueueFamilies(const vk::raii::Device& device, std::span<const QueueFamilyInfo> queueFamilyInfos);

	};

}; // namespace vkr