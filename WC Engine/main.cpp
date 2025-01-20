//#define GLM_FORCE_INTRINSICS 
#pragma comment(lib, "spirv-cross-core")

#include <Windows.h>
#include <commdlg.h>

#pragma warning( push )
#pragma warning( disable : 4702) // Disable unreachable code
#define GLFW_INCLUDE_NONE
//#define GLM_FORCE_PURE
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SILENT_WARNINGS
#define flecs_STATIC

#define MSDFGEN_PUBLIC // ???

#include "Application.h"

//DANGEROUS!
#pragma warning(push, 0)

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_write.h>

#define VOLK_IMPLEMENTATION 
#include <Volk/volk.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#pragma warning(pop)

namespace wc
{
	Application app;

	int main()
	{
		Log::Init();

#ifdef MSVC  // Visual Studio
		std::filesystem::current_path("../../../../WC Engine/workdir");
#elif defined(CLION)  // CLion
		std::filesystem::current_path("../../WC Engine/workdir");
#else  // Default or other IDEs
		std::filesystem::current_path("../../../../WC Engine/workdir");
#endif

		glfwSetErrorCallback([](int err, const char* description) { WC_CORE_ERROR(description); /*WC_DEBUGBREAK();*/ });
		//glfwSetMonitorCallback([](GLFWmonitor* monitor, int event)
		//	{
		//		if (event == GLFW_CONNECTED)
		//		else if (event == GLFW_DISCONNECTED)
		//	});
		if (!glfwInit())
			return 1;

		if (VulkanContext::Create())
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

			app.Start();

			VulkanContext::Destroy();
		}

		glfwTerminate();

		return 0;
	}
}

int main()
{
	return wc::main();
}