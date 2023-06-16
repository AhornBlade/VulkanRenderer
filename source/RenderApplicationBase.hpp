#pragma once

#include <memory>

namespace vkrenderer {

struct RendererApplicationBase
{
	RendererApplicationBase() = default;
	virtual ~RendererApplicationBase() = default;

	virtual void initialize() = 0;
	virtual bool shouldClose() = 0;
	virtual void loop() = 0;
};

#define loadExternClass(Type) \
namespace vkrenderer{ \
	std::unique_ptr<RendererApplicationBase> getApplicationPointer()\
	{\
		return std::unique_ptr<RendererApplicationBase>{new Type{}};\
	}\
}

}// namespace vkrenderer