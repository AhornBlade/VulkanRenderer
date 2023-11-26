#include <core/instance.hpp>
#include <core/queue.hpp>
#include <core/device.hpp>


int main()
{
	auto instance = vkr::createInstance();
	auto physicalDevice = instance.getPhysicalDevice();

	std::vector<vkr::QueueRequirement> queueRequirements;
	queueRequirements.push_back({ vk::QueueFlagBits::eGraphics, 5, 1.0f });
	queueRequirements.push_back({ vk::QueueFlagBits::eCompute, 1, 0.5f, true });
	queueRequirements.push_back({ vk::QueueFlagBits::eTransfer, 2 });

	vkr::QueueFamilyInfos queueFamilyInfos{ physicalDevice, queueRequirements };

	auto device = vkr::createDevice(physicalDevice, queueRequirements);

}