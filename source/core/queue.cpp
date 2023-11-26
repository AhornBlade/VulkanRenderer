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
		auto addQueueFamilyInfo = [&](const QueueRequirement& requirement, uint32_t queueFamilyIndex)
			{
				for (auto& dstInfo : *this)
				{
					if (queueFamilyIndex == dstInfo.queueFamilyIndex)
					{
						dstInfo.queuePriorities.resize(
							dstInfo.queuePriorities.size() + requirement.queueCount,
							requirement.priority);
						return;
					}
				}

				QueueFamilyInfo queueFamilyInfo;
				queueFamilyInfo.queueFamilyIndex = queueFamilyIndex;
				queueFamilyInfo.queuePriorities.resize(requirement.queueCount, requirement.priority);
				push_back(queueFamilyInfo);
			};
		auto addQueueRequirement = [&](const QueueRequirement& requirement) -> bool
			{
				for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); queueFamilyIndex++)
				{
					auto& properties = queueFamilyProperties[queueFamilyIndex];
					if ((properties.queueFlags & requirement.queueType) == requirement.queueType
						&& properties.queueCount >= requirement.queueCount)
					{
						properties.queueCount -= requirement.queueCount;
						addQueueFamilyInfo(requirement, queueFamilyIndex);
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

		for (auto& queueFamilyInfo : *this)
		{
			std::ranges::sort(queueFamilyInfo.queuePriorities, std::ranges::greater());
		}
	}

	QueueFamilyInfos::operator std::vector<vk::DeviceQueueCreateInfo>() const
	{
		std::vector<vk::DeviceQueueCreateInfo> createInfos{ size() };
		std::ranges::copy(*this, createInfos.data());
		return createInfos;
	}

	QueueFamily::QueueFamily(const vk::raii::Device& device, const QueueFamilyInfo& queueFamilyInfo)
		: queueFamilyIndex{ queueFamilyInfo.queueFamilyIndex }
	{
		for (uint32_t queueIndex = 0; queueIndex < queueFamilyInfo.queuePriorities.size(); queueIndex++)
		{
			push_back(Queue{ device, queueFamilyIndex, queueIndex });
		}
	}

	QueueFamilies::QueueFamilies(const vk::raii::Device& device, std::span<const QueueFamilyInfo> queueFamilyInfos)
	{
		for (const auto& queueFamilyInfo : queueFamilyInfos)
		{
			push_back(QueueFamily{ device, queueFamilyInfo });
		}
	}

}; // namespace vkr