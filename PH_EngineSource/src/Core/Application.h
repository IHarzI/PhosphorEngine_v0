#pragma once

#include "PH_CORE.h"
#include "PH_Window.h"
#include "PH_Device.h"
#include "Renderer/PH_Renderer.h"
#include "PH_GameObject.h"
#include "Containers/PH_RingBuffer.h"

#include "Core/PH_GameWorld.h"
#include "Renderer/PH_RenderScene.h"
#include "GUI/ImGUILayer.h"

namespace PhosphorEngine {
	class PH_MaterialSystem;

	struct ApplicationStats
	{
	public:
		ApplicationStats() { msPerFrame.Resize(60); };

		void OnFrameUpdate(float64 elapsed_time_in_seconds);

		PH_INLINE bool IsFrameInPeriod(uint32 Period)
		{
			// if 0 then in period, else false
			if (!((FrameCount + FrameCountOverflows * ULLONG_MAX) % Period))
				return true;

			return false;
		}

		uint64 FrameCount{};
		uint64 FrameCountOverflows{};

		PH_RingBuffer<float64> msPerFrame;

		float64 MS_Average;
		// Accumulated miliseconds(if > than 1 second, make average calc and reset all per sec data)
		float64 AccumulatedMs = 0;
		// Accumulated frames per second
		uint32 AccumulatedFramesPerSec = 0;
		uint32 FPS_Average = 0;
	};

	class Application
	{
	public:
		virtual void run();
		Application();
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;
	private:
		virtual void HandleInput(PH_Window* window, int key, int scancode, int action, int mods);
		void loadGameObjects();
		void loadTextures();

		const int WIDTH = 900;
		const int HEIGHT = 600;

		PH_Window appWindow{ WIDTH,HEIGHT,"Phosphor engine FTest" };

		PH_UniqueMemoryHandle<ImGUILayer> GUILayer{};

		ApplicationStats appStats{};

		PH_GameWorld GameWorld{};

		PH_RenderScene RenderScene{};

		PH_UniqueMemoryHandle<PH_MaterialSystem> MaterialSystem;

		bool shouldRecreatePipelines = false;
	};
}