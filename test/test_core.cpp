#include <core/instance.hpp>


int main()
{
	auto instance = vkr::createInstance();
	auto physicalDevice = instance.getPhysicalDevice();
}