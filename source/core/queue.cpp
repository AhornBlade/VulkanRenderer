#include "queue.hpp"

#include <ranges>

namespace vkr
{
	QueueFamilyInfo::operator vk::DeviceQueueCreateInfo() const
	{
		vk::DeviceQueueCreateInfo createInfo;
		createInfo.setQueueFamilyIndex(queueFamilyIndex);
		createInfo.setQueuePriorities(queuePriorities);
		return createInfo;
	}

	QueueFamilyInfos::QueueFamilyInfos(const vk::raii::PhysicalDevice& physicalDevice)
	{
		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++)
		{
			QueueFamilyInfo info;
			info.queueFamilyIndex = queueFamilyIndex;
			info.queuePriorities.resize(queueFamilyProperties[queueFamilyIndex].queueCount, 1.0f);
			push_back(info);
		}
	}

	QueueFamilyInfos::QueueFamilyInfos(const vk::raii::PhysicalDevice& physicalDevice, std::span<QueueRequirement> queueRequirements)
	{
		std::ranges::sort(queueRequirements,
			[](const QueueRequirement& first, const QueueRequirement& second) -> bool
			{
				if (first.required > second.required) return true;
				if (first.required < second.required) return false;
				return first.queueCount > second.queueCount;
			});

		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		auto addQueueRequirement = [&](const QueueRequirement& requirement) -> bool
			{
				for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++)
				{
					auto& properties = queueFamilyProperties[queueFamilyIndex];
					if ((properties.queueFlags & requirement.queueType) == requirement.queueType
						&& properties.queueCount >= requirement.queueCount)
					{
						properties.queueCount -= requirement.queueCount;
						QueueFamilyInfo info{};
						info.queuePriorities.resize(requirement.queueCount, requirement.priority);
						info.queueFamilyIndex = queueFamilyIndex;
						push_back(info);
						return true;
					}
				}
				return false;
			};

		for (const auto& requirement : queueRequirements)
		{
			if ((!addQueueRequirement(requirement)) && requirement.required)
				throw std::runtime_error("Failed to find required queues");
		}
	}

	QueueFamilyInfos::operator std::vector<vk::DeviceQueueCreateInfo>() const
	{
		std::vector<vk::DeviceQueueCreateInfo> createInfos{ size() };
		std::ranges::copy(*this, createInfos.data());
		return createInfos;
	}

}; // namespace vkr