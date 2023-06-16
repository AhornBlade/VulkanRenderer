#include "RenderApplicationBase.hpp"

namespace vkrenderer {
	extern std::unique_ptr<RendererApplicationBase> getApplicationPointer();
}

int main()
{
	auto application = vkrenderer::getApplicationPointer();
	application->initialize();
	while (!application->shouldClose())
	{
		application->loop();
	}
}