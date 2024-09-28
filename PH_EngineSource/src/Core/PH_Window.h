#pragma once

#include "PH_CORE.h"
#include "Containers/PH_String.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "../vendor/glfw/include/GLFW/glfw3.h"

#include <string>
#include <functional>

namespace PhosphorEngine
{
	using namespace PhosphorEngine::Containers;

	class PH_Window;

	typedef std::function<void(PH_Window* window, int key, int scancode, int action, int mods)> PHkeyCallback;

	class PH_Window
	{
	public:
		PH_Window(int w, int h, PH_String name);
		~PH_Window();

		PH_Window(const PH_Window&) = delete;
		PH_Window &operator=(const PH_Window &) = delete;

		bool shoudClose() { return glfwWindowShouldClose(window); }

		void createWindowSurface(VkInstance instance_, VkSurfaceKHR* surface);

		bool wasWindowResized() const { return frameBufferResized; }
		void resetWindowResizedFlag() { frameBufferResized = false; }

		VkExtent2D getExtent() { return {static_cast<uint32_t>(width),static_cast<uint32_t>(height)}; }

		GLFWwindow* getGLFWWindow() const { return window; }

		void SetInputCallback(PHkeyCallback callback);
	private:
		static void frameBufferResizedCallback(GLFWwindow* window, int width, int height);
		static void inputCallback(GLFWwindow* Window, int32 key, int32 scancode, int32 action, int32 mods);
		GLFWwindow* window;

		int width;
		int height;
		bool frameBufferResized = false;

		PHkeyCallback InputCallbackFunction = nullptr;

		PH_String windowName;

		void initWindow();
	};

}

