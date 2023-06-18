#if defined (_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define VK_USE_PLATFORM_XCB_KHR
#elif defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#endif

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <RenderApplicationBase.hpp>

#include <iostream>

namespace vkr = ::vk::raii;

struct QueueRequirement
{
	vk::QueueFlags usage;
	uint32_t count;
	float priority = 0.5f;
};

class QueueFamily
{
public:
	QueueFamily(const vkr::PhysicalDevice& physicalDevice, uint32_t _queueFamilyIndex)
		:queueFamilyIndex{ _queueFamilyIndex } 
	{
		queueFamilyProperties = physicalDevice.getQueueFamilyProperties()[queueFamilyIndex];
	}

	QueueFamily(const vk::QueueFamilyProperties& _queueFamilyProperties, uint32_t _queueFamilyIndex)
		:queueFamilyProperties{ _queueFamilyProperties }, queueFamilyIndex{ _queueFamilyIndex } {}

	QueueFamily() = default;

	vk::DeviceQueueCreateInfo getDeviceQueueCreateInfo() const noexcept
	{
		vk::DeviceQueueCreateInfo createInfo{};
		createInfo.setQueuePriorities(queuePriorities);
		createInfo.setQueueFamilyIndex(queueFamilyIndex);
		return createInfo;
	}

	bool addQueue(vk::QueueFlags queueUsage, float priority)
	{
		if (!(queueUsage & queueFamilyProperties.queueFlags))
			return false;
		if (queuePriorities.size() == queueFamilyProperties.queueCount)
			return false;
		queuePriorities.push_back(priority);
		queueUsages.push_back(queueUsage);

		return true;
	}

	bool addQueueUsage(vk::QueueFlags requiredQueueUsage, float priority = 0.0f)
	{
		if (!(requiredQueueUsage & queueFamilyProperties.queueFlags))
			return false;
		for (uint32_t queueIndex = 0; queueIndex < queuePriorities.size(); queueIndex++)
		{
			auto& queueUsage = queueUsages[queueIndex];
			auto& queuePriority = queuePriorities[queueIndex];
			if (queueUsage & requiredQueueUsage) continue;
			queueUsage = queueUsage | requiredQueueUsage;
			queuePriority = max(queuePriority, priority);
			return true;
		}

		return false;
	}

	void createQueues(const vkr::Device& device)
	{
		for (uint32_t queueIndex = queues.size(); queueIndex < queuePriorities.size(); queueIndex++)
		{
			queues.emplace_back(device, queueFamilyIndex, queueIndex);
		}
	}

	friend class QueueFamilies;

private:
	vk::QueueFamilyProperties queueFamilyProperties;
	uint32_t queueFamilyIndex;
	std::vector<vkr::Queue> queues;
	std::vector<float> queuePriorities;
	std::vector<vk::QueueFlags> queueUsages;
};

class QueueFamilies : public std::vector<QueueFamily>
{
public:
	using std::vector<QueueFamily>::vector;

	explicit QueueFamilies(const vkr::PhysicalDevice& physicalDevice)
	{
		auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		for (uint32_t index = 0; index < queueFamilyProperties.size(); index++)
		{
			this->push_back({ queueFamilyProperties[index], index });
		}
	}

	std::vector<vk::DeviceQueueCreateInfo> getDeviceQueueCreateInfos()const noexcept
	{
		std::vector<vk::DeviceQueueCreateInfo> queueInfos{};
		for (const auto& queueFamily : *this)
		{
			auto queueInfo = queueFamily.getDeviceQueueCreateInfo();
			if (queueInfo.queueCount > 0)
				queueInfos.emplace_back(std::move(queueInfo));
		}
		return queueInfos;
	}

	bool addQueues(const std::vector<QueueRequirement>& queueRequirements)
	{
		for (auto& requirement : queueRequirements)
		{
			uint32_t finishRequirements = 0;

			for (auto& queueFamily : *this)
			{
				if (finishRequirements == requirement.count)
					break;
				if(queueFamily.addQueue(requirement.usage, requirement.priority))
					finishRequirements++;
			}

			for (auto& queueFamily : *this)
			{
				if (finishRequirements == requirement.count)
					break;
				if (queueFamily.addQueueUsage(requirement.usage, requirement.priority))
					finishRequirements++;
			}

			if (finishRequirements < requirement.count)
				return false;
		}
		return true;
	}

	void createQueues(const vkr::Device& device)
	{
		for (auto& queueFamily : *this)
		{
			queueFamily.createQueues(device);
		}
	}

	uint32_t getQueueCount() const noexcept
	{
		uint32_t count = 0;
		for (auto& queueFamily : *this)
		{
			count += queueFamily.queues.size();
		}
		return count;
	}
};

class Window
{
public:
	Window(int width, int height, const char* title)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		if (glfwVulkanSupported() == GLFW_FALSE)
		{
			std::cout << "Error: Vulkan is unabled to support GLFW\n";
		}

		window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	}

	~Window()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	bool shouldClose() const
	{
		return glfwWindowShouldClose(window);
	}

	auto getSurfaceCreateInfo()const noexcept
	{
#ifdef VK_USE_PLATFORM_WIN32_KHR
		vk::Win32SurfaceCreateInfoKHR createInfo{};
		createInfo.setHinstance(GetModuleHandle(nullptr));
		createInfo.setHwnd(glfwGetWin32Window(window));
		return createInfo;
#endif
	}

	void show()
	{
		glfwPollEvents();
	}

	std::vector<const char*> getRequiredExtensions() const
	{
		uint32_t extensionCount;
		const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		return std::vector<const char*>(extensions, extensions + extensionCount);
	}

private:
	GLFWwindow* window = nullptr;
};

class VulkanApplication : public vkrenderer::RendererApplicationBase
{
public:
	VulkanApplication()
	{
		createInstance();
		createSurface();
		choosePhysicalDevice();
		createDevice();
	}
	
	virtual void initialize() override
	{
		
	}

	virtual bool shouldClose() override
	{
		return window.shouldClose();
	}

	virtual void loop() override
	{
		window.show();
	}

private:
	Window window{800, 600, "triangle"};
	vkr::Context context{};
	vkr::Instance instance{nullptr};
	vkr::DebugUtilsMessengerEXT debugMessenger{nullptr};
	vkr::SurfaceKHR surface{nullptr};
	vkr::PhysicalDevice physicalDevice{nullptr};
	vkr::Device device{nullptr};
	QueueFamilies queueFamilies;

	void createInstance()
	{
		auto enabledExtensions = window.getRequiredExtensions();
		enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

		std::vector<const char*> enabledLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		vk::DebugUtilsMessengerCreateInfoEXT debugInfo{};
		debugInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo);
		debugInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		debugInfo.setPfnUserCallback([](
			VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)->VkBool32
			{
				if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
				{
					//std::cout << "Info: " << pCallbackData->pMessage << '\n';
				}
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				{
					std::cout << "Warning: " << pCallbackData->pMessage << '\n';
				}
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				{
					std::cout << "Error: " << pCallbackData->pMessage << '\n';
				}

				return VK_FALSE;
			});

		vk::InstanceCreateInfo createInfo{};
		createInfo.setPEnabledExtensionNames(enabledExtensions);
		createInfo.setPEnabledLayerNames(enabledLayers);
		createInfo.setPNext(&debugInfo);
		
		instance = vkr::Instance{ context, createInfo };
		
		debugMessenger = instance.createDebugUtilsMessengerEXT(debugInfo);

		std::cout << "Info: Success to create instance\n";
	}

	void createSurface()
	{
		auto createInfo = window.getSurfaceCreateInfo();
		surface = vkr::SurfaceKHR{ instance, createInfo };

		std::cout << "Info: Success to create Win32 surface\n";
	}

	void choosePhysicalDevice()
	{
		auto enabledPhysicalDevices = instance.enumeratePhysicalDevices();
		if (enabledPhysicalDevices.empty())
		{
			std::cout << "Error: Failed to find physical devices\n";
		}

		for (auto enabledPhysicalDevice : enabledPhysicalDevices)
		{
			auto properties = enabledPhysicalDevice.getProperties();
			if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			{
				physicalDevice = enabledPhysicalDevice;
				break;
			}
		}
		if (!(*physicalDevice))
		{
			physicalDevice = enabledPhysicalDevices[0];
		}

		std::cout << "Info: Using physical device " << physicalDevice.getProperties().deviceName << '\n';
	}

	void createDevice()
	{
		std::vector<const char*> enabledExternsions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		std::vector<const char*> enabledLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		auto enabledFeatures = physicalDevice.getFeatures();

		std::vector<QueueRequirement> queueRequirements{
			{vk::QueueFlagBits::eGraphics, 1, 1.0f},
			{ vk::QueueFlagBits::eTransfer, 1 }
		};

		queueFamilies = QueueFamilies{ physicalDevice };
		queueFamilies.addQueues(queueRequirements);

		auto queueInfos = queueFamilies.getDeviceQueueCreateInfos();

		vk::DeviceCreateInfo createInfo{};
		createInfo.setPEnabledExtensionNames(enabledExternsions);
		createInfo.setPEnabledLayerNames(enabledLayers);
		createInfo.setPEnabledFeatures(&enabledFeatures);
		createInfo.setQueueCreateInfos(queueInfos);

		device = physicalDevice.createDevice(createInfo);

		std::cout << "Info: Success to create device\n";

		queueFamilies.createQueues(device);

		std::cout << "Info: Success to create " << queueFamilies.getQueueCount() << " queue(s)\n";
	}

};

loadExternClass(VulkanApplication);