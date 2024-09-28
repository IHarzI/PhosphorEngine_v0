#pragma once

#include "Core/PH_CORE.h"
#include "Core/PH_Window.h"

namespace PhosphorEngine {

	struct ImGuiDrawInfo {
		float64 MS_Average;
		uint32 FPS_Average;
	};

	class ImGUILayer
	{
	public:
		ImGUILayer( PH_Window& window) :
			AppWindow(window) {};

		void Init();

		void Draw(ImGuiDrawInfo DrawInfo, VkCommandBuffer cmd);

		void Destroy();

	private:

		VkDescriptorPool ImguiPool{};

		PH_Window& AppWindow;
	};

};