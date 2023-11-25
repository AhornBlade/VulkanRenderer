#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace vkr
{	
	struct InstanceCreateInfo
	{
		std::vector<const char*> enabledLayers;
		std::vector<const char*> enabledExtensions;
	};

	class Instance
	{
	public:
		Instance(std::span<const char* const> enabledLayers, std::span<const char* const> enabledExtensions);
		Instance(const InstanceCreateInfo& createInfo);

	private:
		vk::raii::Context context;
		vk::raii::Instance instance;
		vk::raii::DebugUtilsMessengerEXT debugMessenger;

		vk::raii::Instance getInstance(std::span<const char* const> enabledLayers, std::span<const char* const> enabledExtensions)const;
		vk::DebugUtilsMessengerCreateInfoEXT getDebugUtilsCreateInfo() const;
	};

	InstanceCreateInfo getDefaultInstanceCreateInfo();

	template<class ... Ts>
	inline Instance createInstance()
	{
		auto createInfo = getDefaultInstanceCreateInfo();

		(Ts::setInstanceCreateInfo(createInfo), ...);

		return Instance{ createInfo };
	}


}; //namesapce vkr