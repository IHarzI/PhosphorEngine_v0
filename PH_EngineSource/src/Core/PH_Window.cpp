#include "PH_Assert.h"
#include "PH_Window.h"

namespace PhosphorEngine {

	PH_Window::PH_Window(int w, int h, PH_String name)
		: width(w),height(h), windowName(name)
	{
		initWindow();
	}

	PH_Window::~PH_Window()
	{
		glfwDestroyWindow(window);
		glfwTerminate();

	}

	void PH_Window::createWindowSurface(VkInstance instance_, VkSurfaceKHR* surface)
	{
		PH_VULKAN_CHECK_MSG (glfwCreateWindowSurface(instance_, window, nullptr, surface), 
			"Failed to create window surface!");
	}

	void PH_Window::SetInputCallback(PHkeyCallback callback)
	{
		InputCallbackFunction = callback;
	}

	void PH_Window::frameBufferResizedCallback(GLFWwindow* window, int width, int height)
	{
		PH_Window* Ph_Window = reinterpret_cast<PH_Window*>(glfwGetWindowUserPointer(window));

		PH_ASSERT_MSG(Ph_Window, "Wrong window user pointer in glfwWindow");
		
		Ph_Window->frameBufferResized = true;
		Ph_Window->width = width;
		Ph_Window->height = height;
	}

	void PH_Window::inputCallback(GLFWwindow* Window, int32 key, int32 scancode, int32 action, int32 mods)
	{
		PH_Window* Ph_Window = reinterpret_cast<PH_Window*>(glfwGetWindowUserPointer(Window));

		PH_ASSERT_MSG(Ph_Window, "Wrong window user pointer in glfwWindow");

		if (Ph_Window->InputCallbackFunction)
		{
			Ph_Window->InputCallbackFunction(Ph_Window, key, scancode, action, mods);
		}
	}

	void PH_Window::initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, frameBufferResizedCallback);
		glfwSetKeyCallback(window, inputCallback);
	}

}
