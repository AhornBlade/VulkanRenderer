#include <core/core.hpp>

int main()
{
	auto instance = vkr::createInstance();
	auto physicalDevice = instance.getPhysicalDevice();
	//auto physicalDevice = instance.getPhysicalDevice(0);//another way
	//auto physicalDevice = instance.getPhysicalDevice(
	//	[](const vk::PhysicalDeviceProperties& properties)->uint32_t
	//	{
	//		if (std::ranges::includes(properties.deviceName, std::string("NVIDIA")))
	//			return 100;
	//		else if (std::ranges::includes(properties.deviceName, std::string("AMD")))
	//			return 50;
	//		else
	//			return 0;
	//	});// another way

	std::vector<vkr::QueueRequirement> queueRequirements;
	queueRequirements.push_back({ vk::QueueFlagBits::eGraphics, 5, 1.0f });
	queueRequirements.push_back({ vk::QueueFlagBits::eCompute, 1, 0.5f, true });
	queueRequirements.push_back({ vk::QueueFlagBits::eTransfer, 2 });

	vkr::QueueFamilyInfos queueFamilyInfos{ physicalDevice, queueRequirements };

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = queueFamilyInfos;

	// compile time error
	//std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = vkr::QueueFamilyInfos{ physicalDevice, queueRequirements };

	auto device = vkr::createDevice(physicalDevice, queueRequirements);
	//auto device = vkr::createDevice(physicalDevice);// another way GPU only mode
	//auto device = vkr::createDevice(physicalDevice, queueFamilyInfos);// another way
	//auto device = vkr::createDevice(physicalDevice, queueCreateInfos);// another way

}