#include "ImGUILayer.h"
#include "imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Systems/PH_CVarSystem.h"
#include "Memory/PH_MemoryStatistic.h"
#include "Core/PH_Profiler.h"
#include "Core/PH_REngineContext.h"

namespace PhosphorEngine {

	void ImGUILayer::Init()
	{
		// IMGUI SETUP
		{
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsLight();

			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000;
			pool_info.poolSizeCount = 11;// std::size(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;

			PH_VULKAN_CHECK(vkCreateDescriptorPool(PH_REngineContext::GetLogicalDevice(), &pool_info, nullptr, &ImguiPool));

			// Setup Platform/Renderer backends
			ImGui_ImplGlfw_InitForVulkan(AppWindow.getGLFWWindow(), true);
			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = PH_REngineContext::GetDevice().GetInstance();
			init_info.PhysicalDevice = PH_REngineContext::GetDevice().GetPhysicalDevice();
			init_info.Device = PH_REngineContext::GetDevice().GetDevice();
			init_info.QueueFamily = PH_REngineContext::GetDevice().GetGraphicsQueueIndex();
			init_info.Queue = PH_REngineContext::GetDevice().GetGraphicsQueue();
			init_info.DescriptorPool = ImguiPool;
			init_info.MinImageCount = 3;
			init_info.ImageCount = 3;
			init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			ImGui_ImplVulkan_Init(&init_info, PH_REngineContext::GetRenderer().getSwapChainRenderPass());
			auto cmdBuffer = PH_REngineContext::GetDevice().beginSingleTimeCommands();
			ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
			PH_REngineContext::GetDevice().endSingleTimeCommands(cmdBuffer);

			ImGui_ImplVulkan_DestroyFontUploadObjects();

		}
	}

	void ImGUILayer::Destroy()
	{
		ImGui_ImplVulkan_Shutdown();
		vkDestroyDescriptorPool(PH_REngineContext::GetLogicalDevice(), ImguiPool, PH_REngineContext::GetDevice().GetAllocator());
	}

	void ImGUILayer::Draw(ImGuiDrawInfo DrawInfo, VkCommandBuffer cmd)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Debug"))
			{
				if (ImGui::BeginMenu("CVAR"))
				{
					PH_CVarSystem::Get()->DrawImguiEditor();
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Stats"))
				{
					Memory::PH_GlobalMemoryStats::DrawImguiStats();
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Profiler"))
				{
					PH_Profiler::DrawImguiStats();
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Controls"))
				{
					ImGui::Text("Camera movement: [WASD] - move, \n[Arrows Keys] rotation, \n[Middle Mouse Button hold] - camera rotation by screen mouse input");
					ImGui::Text("Sandbox controls : Alt / [K] Toggle Light Point color, [K] Toggle emision of light point, Shift / [R} Move Vase");
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			ImGui::Text("FPS: %u, ms per frame: %f", DrawInfo.FPS_Average, DrawInfo.MS_Average);
			ImGui::EndMainMenuBar();
		}


		ImGui::Render();

		ImDrawData* draw_data = ImGui::GetDrawData();
		const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
		if (!is_minimized)
		{
			ImGui_ImplVulkan_RenderDrawData(draw_data, cmd);
		};
	}
};
