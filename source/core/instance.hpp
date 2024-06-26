#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <functional>

namespace vkr
{	
	struct InstanceCreateInfo
	{
		InstanceCreateInfo();

		std::vector<const char*> enabledLayers;
		std::vector<const char*> enabledExtensions;
	};

	class Instance
	{
	public:
		Instance(std::span<const char* const> enabledLayers, std::span<const char* const> enabledExtensions);
		Instance(const InstanceCreateInfo& createInfo);

		inline operator vk::raii::Instance& () noexcept { return instance; }

		vk::raii::PhysicalDevice getPhysicalDevice(uint32_t index) const;

		vk::raii::PhysicalDevice getPhysicalDevice(
			std::function<uint32_t(const vk::PhysicalDeviceProperties&)> score) const;

        // get best default physical device
		vk::raii::PhysicalDevice getPhysicalDevice() const;

	private:
		vk::raii::Context context;
		vk::raii::Instance instance;
		vk::raii::DebugUtilsMessengerEXT debugMessenger;

		vk::raii::Instance getInstance(std::span<const char* const> enabledLayers, std::span<const char* const> enabledExtensions)const;
		vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsCreateInfo() const;
	};

	template<class ... Ts>
	inline Instance createInstance()
	{
		InstanceCreateInfo createInfo{};

		(Ts::setInstanceCreateInfo(createInfo), ...);

		return Instance{ createInfo };
	}


}; //namesapce vkr