#include <core/instance.hpp>
#include <core/queue.hpp>


int main()
{
	auto instance = vkr::createInstance();
	auto physicalDevice = instance.getPhysicalDevice();

	std::vector<vkr::QueueRequirement> queueRequirements;
	queueRequirements.push_back({ vk::QueueFlagBits::eGraphics, 20 });
	queueRequirements.push_back({ vk::QueueFlagBits::eCompute, 1, 1.0f, true });
	queueRequirements.push_back({ vk::QueueFlagBits::eTransfer, 2 });

	vkr::QueueFamilyInfos queueFamilyInfos{ physicalDevice, queueRequirements };
}