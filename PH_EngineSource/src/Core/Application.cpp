#include "Application.h"
#include "KeyControl.h"
#include "Renderer/SimpleRenderSystem.h"
#include "Renderer/LightSystem.h"
#include "Core/PH_Camera.h"
#include "PH_Buffer.h"
#include "Memory/PH_Memory.h"
#include "Containers/PH_DynamicArray.h"
#include "Utils/PH_Utils.h"
#include "PH_Profiler.h"
#include "Core/PH_Descriptors.h"
#include "Core/PH_Pipeline.h"
#include "PH_REngineContext.h"

#include "Core/PH_FrameInfo.h"
#include "Systems/PH_CVarSystem.h"
#include "Renderer/PH_MaterialSystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <sstream>

AutoCVar_Int CVAR_WireframeMode("rendering.wireframemode", "Render wireframes", 0, CVarFlags::EditCheckbox);

const uint32 SSBO_MaxObjects = 10000;

namespace PhosphorEngine {
	using namespace PhosphorEngine::Memory;
	using namespace PhosphorEngine::Containers;
	using namespace PhosphorEngine::Utils;

	void Application::run()
	{
		PH_INFO("Application start");
		auto& REngineContext = PH_REngineContext::GetRenderEngineContext();
		
		Time::HResStopWatch<> timer;
		timer.Count();
		PH_DynamicArray<PH_UniqueMemoryHandle<PH_Buffer>> uboBuffers{ PH_SwapChain::MAX_FRAMES_IN_FLIGHT };
		PH_DynamicArray<PH_UniqueMemoryHandle<PH_Buffer>> SSBO_Buffers{ PH_SwapChain::MAX_FRAMES_IN_FLIGHT };
		for (int i = 0; i < uboBuffers.size(); i++)
		{
			uboBuffers[i] = PH_UniqueMemoryHandle<PH_Buffer>::Create(
			sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}

		for (int i = 0; i < SSBO_Buffers.size(); i++)
		{
			SSBO_Buffers[i] = PH_UniqueMemoryHandle<PH_Buffer>::Create(
				sizeof(ObjectModelData),
				10000,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		};


		PH_PushBuffer dynamicDataBuffer{ sizeof(uint64),1024,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };

		auto globalSetLayout = PH_DescriptorSetLayout::Builder()
			.addBinding(
				0,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(
				1,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(
				2,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(
				3,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				VK_SHADER_STAGE_ALL_GRAPHICS
			)
			.SetLayoutSetNumber(0)
			.build();

		PH_DynamicArray<VkDescriptorSet> globalDescriptorSets(PH_SwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto UBObufferInfo = uboBuffers[i].Get()->descriptorInfo(sizeof(GlobalUbo), 0);
			auto SSBObufferInfo = SSBO_Buffers[i].Get()->descriptorInfo(sizeof(ObjectModelData)*10000, 0);
			auto DynamicBufferInfo = dynamicDataBuffer.GetBuffer().descriptorInfo(
				dynamicDataBuffer.GetBuffer().getAlignmentSize() * dynamicDataBuffer.GetBuffer().getInstanceCount(), 0);
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = REngineContext.GetTexturesMap().begin()->second->ImageView;
			imageInfo.sampler = REngineContext.GetTexturesMap().begin()->second->Sampler;

			PH_DescriptorWriter(*globalSetLayout)
				.writeBuffer(0, &UBObufferInfo)
				.writeBuffer(2, &SSBObufferInfo)
				.writeBuffer(3, &DynamicBufferInfo)
				.writeImage(1,&imageInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{ REngineContext.GetRenderer().getSwapChainRenderPass(),globalSetLayout->getDescriptorSetLayout() };

		LightSystem pointLightSystem{ REngineContext.GetRenderer().getSwapChainRenderPass(),globalSetLayout->getDescriptorSetLayout() };
		
        PH_Camera camera{};

        auto viewerObject = PH_ObjectBuilder::createGameObject("Camera");
        keyControl userCntrlr{};

		viewerObject->transform.translation.x -= 2.f;
		viewerObject->transform.translation.y -= 0.9f;
		viewerObject->transform.rotation.y -= 85.f;
		viewerObject->transform.translation.z = 3.f;

		GUILayer->Init();

		Time::HResStopWatch<float64, Time::timeFormat::scnds> frameTimer;
		frameTimer.Count();

		appWindow.SetInputCallback([this](PH_Window * Window, int32 key, int32 scancode, int32 action, int32 mods)
		{
				this->HandleInput(Window, key, scancode, action, mods);
		});
	
		while (!appWindow.shoudClose())
		{
			if ((bool)CVAR_WireframeMode.Get() != (bool)PH_REngineContext::GetDevice().GetRenderFeaturesConfig().drawWireframe)
			{
				PH_REngineContext::GetDevice().ToggleRenderFeature(WireframeRender);
				shouldRecreatePipelines = true;
			};

			if (shouldRecreatePipelines)
			{
				vkDeviceWaitIdle(PH_REngineContext::GetLogicalDevice());
				simpleRenderSystem.RecreatePipelines(PH_REngineContext::GetRenderer().getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout());
				pointLightSystem.RecreatePipelines(PH_REngineContext::GetRenderer().getSwapChainRenderPass(),globalSetLayout->getDescriptorSetLayout());
				shouldRecreatePipelines = false;
			}

			glfwPollEvents();

			userCntrlr.cursorCntrl(appWindow.getGLFWWindow()); 

			float64 frameTime = frameTimer.Stop();
			frameTimer.Count();
			// Move Camera
			userCntrlr.moveXYZ(appWindow.getGLFWWindow(), frameTime, viewerObject.GetReference());

            camera.setViewYXZ(viewerObject->transform.translation, viewerObject->transform.rotation);

            float aspect = PH_REngineContext::GetRenderer().getAspectRatio();
            camera.setPerspectiveProjection(Math::radians(60.f), aspect, 0.1f, 100.f);

			if (auto commandBuffer = PH_REngineContext::GetRenderer().beginFrame())
			{
					PH_PROFILE_SCOPE(FrameLoop)
					int frameIndex = PH_REngineContext::GetRenderer().getFrameIndex();
					PH_FrameInfo frameInfo{ frameIndex,
						frameTime,
						commandBuffer,
						&camera, globalDescriptorSets[frameIndex],
						&dynamicDataBuffer,
						&GameWorld };
					PH_REngineContext::UpdateFrameInfo(frameInfo);

					for (auto& Object : GameWorld.GetObjectMap())
					{
						if (auto model = Object.second->model.Get())
						{
							if (model->GetMeshName().find("cube") != PH_String::npos)
							{
								Object.second->transform.rotation.x += frameTime * 0.1f;
								Object.second->transform.rotation.z += frameTime * 0.25f;
								Object.second->transform.rotation.y += frameTime * Math::cos(0.1f);
							}
						}
					}
					// UPDATE PART
					{
						PH_PROFILE_SCOPE(FrameUpdate)
						GlobalUbo ubo {};
						ubo.CameraProjection = camera.getProjectionMatrix();
						ubo.CameraView= camera.getView();
						ubo.CameraInverseView= camera.getInverseViewMatrix();

						// Disco ambient
						//{
						//	ubo.ambientLightCol.r = Math::cos<float>(appStats.FrameCount % 30 * frameTime);
						//	ubo.ambientLightCol.b = Math::sin<float>(appStats.FrameCount % 30 * frameTime);
						//	ubo.ambientLightCol.w = Math::sin<float>(appStats.FrameCount % 90 * frameTime);
						//}

						pointLightSystem.update(&frameInfo, &ubo);

						uboBuffers[frameIndex]->writeByStagingBuffer(&ubo, uboBuffers[frameIndex].GetReference().getBufferSize());

						PH_Buffer SSBO_StagingBuffer{
							SSBO_Buffers[frameIndex]->getAlignmentSize(),
							SSBO_MaxObjects,
							VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT };

						SSBO_StagingBuffer.map();
						ObjectModelData* ModelData = (ObjectModelData*)SSBO_StagingBuffer.getMappedMemory();
						uint32 ModelIndex = 0;
						for (auto& Object : GameWorld.GetObjectMap())
						{
							ModelData[ModelIndex].modelMatrix = Object.second->transform.mat4();
							ModelData[ModelIndex].normalMatrix = Math::mat4x4{ Object.second->transform.normalMatrix(),{0.f},{0.f} };
							ModelIndex++;
						};
						SSBO_StagingBuffer.unmap();

						REngineContext.GetDevice().copyBuffer(SSBO_StagingBuffer.getBuffer(), SSBO_Buffers[frameIndex]->getBuffer(),
							SSBO_Buffers[frameIndex]->getAlignmentSize() * SSBO_MaxObjects);
					}
					
					//render
					{
						PH_PROFILE_SCOPE(FrameRender)

						REngineContext.GetRenderer().beginSwapChainRenderPass(commandBuffer);

						simpleRenderSystem.renderGameObjects(&frameInfo);
						pointLightSystem.render(&frameInfo);

						GUILayer->Draw({ appStats.MS_Average, appStats.FPS_Average }, commandBuffer);

						REngineContext.GetRenderer().endSwapChainRenderPass(commandBuffer);

						REngineContext.GetRenderer().endFrame();
					}
				appStats.OnFrameUpdate(frameTime);
			}
		}

		vkDeviceWaitIdle(REngineContext.GetLogicalDevice());
		GUILayer->Destroy();
		MaterialSystem->cleanup();

		PH_INFO("Application shutdown");
		std::stringstream AppReport{};
		AppReport << "\nApplication runtime is: " << timer.Stop().AsSeconds() << " seconds\n";
		PH_String AppReportsStr{}; 
		AppReport.str(AppReportsStr);
		PH_INFO(AppReportsStr.c_str());
	}

	Application::Application()
	{
		PH_REngineContext::Initialize(appWindow);

		MaterialSystem = PH_UniqueMemoryHandle<PH_MaterialSystem>::Create();
		MaterialSystem->buildDefaultTemplates();
		GUILayer = PH_UniqueMemoryHandle<ImGUILayer>::Create(appWindow);
		
		loadGameObjects();
		loadTextures();
	}

	Application::~Application()
	{
		// Will be deleted after applicaiton in PH_Main 
		//PH_REngineContext::Destroy();
	}

	void Application::loadGameObjects()
	{

		auto smooth_vase = PH_ObjectBuilder::createGameObject("SmoothVase");
		PH_SharedMemoryHandle<PH_Mesh> ObjectMesh = PH_Mesh::createMeshFromFile("assets/models/smooth_vase.obj", smooth_vase->getId(), "smooth_vase");

		smooth_vase->model = ObjectMesh;
		smooth_vase->transform.translation = { .0f,.0f,.5f };
		smooth_vase->transform.scale = { 1.f, 2.f, 1.f };
		smooth_vase->AddTag("ControlObj");
		GameWorld.AddObject(std::move(smooth_vase));

		auto flat_vase = PH_ObjectBuilder::createGameObject("FlatVase");
		ObjectMesh = PH_Mesh::createMeshFromFile("assets/models/flat_vase.obj", flat_vase->getId(), "flat_vase");

		flat_vase->model = ObjectMesh;
		flat_vase->transform.translation = { 1.0f,1.0f,2.5f };
		flat_vase->transform.scale = { 1.f, 1.f, 1.f };
		GameWorld.AddObject(std::move(flat_vase));

		auto Viking_room = PH_ObjectBuilder::createGameObject("VikingRoom");
		ObjectMesh = PH_Mesh::createMeshFromFile("assets/models/Viking_room.obj", Viking_room->getId(), "viking_room");

		Viking_room->model = ObjectMesh;
		Viking_room->transform.translation = { 1.0f,-2.0f,-5.5f };
		Viking_room->transform.rotation.x += Math::radians(90.f);
		Viking_room->transform.scale = { 1.f, 1.f, 1.f };
		GameWorld.AddObject(std::move(Viking_room));

		auto colored_cube = PH_ObjectBuilder::createGameObject("ColoredCube");
		ObjectMesh = PH_Mesh::createMeshFromFile("assets/models/colored_cube.obj", colored_cube->getId(), "colored_cube");

		colored_cube->model = ObjectMesh;
		colored_cube->transform.translation = { .3f,.2f,-11.5f };
		colored_cube->transform.scale = { 1.f, 1.f, 1.f };
		GameWorld.AddObject(std::move(colored_cube));

		auto floor = PH_ObjectBuilder::createGameObject("Floor");
		ObjectMesh = PH_Mesh::createMeshFromFile("assets/models/Quad.obj", floor->getId(), "quad");

		floor->model = ObjectMesh;
		floor->transform.translation = { .0f,1.5f,0.f };
		floor->transform.scale = { 15.f, 15.f, 15.f };
		GameWorld.AddObject(std::move(floor));

		ObjectMesh.Release();

		const int lightsCount = 5;

		Math::vec3 lightColors[lightsCount]{
			{ 0.9f,0.1f,0.2f },
			{ 0.2f,0.75f,0.56f },
			{ 0.17f,0.25f,0.86f },
			{ 0.5f,0.89f,0.56f },
			{ 1.f,1.f,1.f}
		};
		for (int i = 0; i < lightsCount; i++)
		{
			auto pointLight = PH_ObjectBuilder::createPointLight(lightColors[i], i * 0.15 + 0.2f);
			auto rotateLight = Math::Rotate(Math::mat4x4(1.f), (i * Math::two_pi<float>()) / lightsCount, { 0.f,-1.f,-.2f });
			pointLight->transform.translation = Math::vec3((rotateLight * Math::vec4(-1.f, -1.f, -1.f, 1.f)).xyz);
			pointLight->transform.scale.x = (i + 1)/7.f;
			pointLight->AddTag("RotatedLightPoint");
			GameWorld.AddObject(std::move(pointLight));
		}
		auto newPointLight = PH_ObjectBuilder::createPointLight({ 15.65f,6.8797f,2.87669f }, 2.f, .3f); 
		newPointLight->AddTag({ PH_String{"PointLight"},PH_String{"ControlLight"} });
		GameWorld.AddObject(std::move(newPointLight));
	}

	void Application::loadTextures()
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* PixelsOfVikingRoom = stbi_load("assets/textures/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		texChannels = STBI_rgb_alpha;
		VkDeviceSize imageSize = texWidth * texHeight * texChannels;

		if (!PixelsOfVikingRoom) {
			PH_ERROR("Failed to load texture image!");
		}
		
		auto Texture = PH_REngineContext::GetRenderer().CreateTexture(
			"VikingRoom", 
			texWidth, texHeight, texChannels,
			(const uint8*)PixelsOfVikingRoom, false);
		PH_REngineContext::GetTexturesMap().emplace(Texture.GetReference().ID, std::move(Texture));
		stbi_image_free(PixelsOfVikingRoom);
	}
	
	void Application::HandleInput(PH_Window* Window, int32 key, int32 scancode, int32 action, int32 mods)
	{
		if (key == keyControl::KeyMappings::KeyCodes::K && action == GLFW_PRESS)
		{
			for (auto& obj : GameWorld.GetObjectMap())
			{
				if (obj.second->HasTag("ControlLight"))
				{
					if (glfwGetKey(Window->getGLFWWindow(), keyControl::KeyMappings::KeyCodes::AltLeft) == GLFW_PRESS)
						obj.second->pointLight->lightIntensity *= -1.f;
					else
						obj.second->pointLight->Emit = obj.second->pointLight->Emit == true ? false : true;
				}
			}
		};
		if (key == keyControl::KeyMappings::KeyCodes::R && action == GLFW_PRESS)
		{
			for (auto& obj : GameWorld.GetObjectMap())
			{
				if (obj.second->HasTag("ControlObj"))
				{
					if (glfwGetKey(Window->getGLFWWindow(), keyControl::KeyMappings::KeyCodes::ShiftLeft) == GLFW_PRESS)
						obj.second->transform.translation.z += 1.f;
					else
						obj.second->transform.translation.z -= 1.f;
				}
			}
		};
		if (key == keyControl::KeyMappings::KeyCodes::ShiftLeft && action == GLFW_PRESS)
		{
			if (glfwGetKey(Window->getGLFWWindow(), keyControl::KeyMappings::KeyCodes::AltLeft) == GLFW_PRESS)
			{
				PH_REngineContext::GetDevice().ToggleRenderFeature(WireframeRender);
				CVAR_WireframeMode.Set(PH_REngineContext::GetDevice().GetRenderFeaturesConfig().drawWireframe);
				shouldRecreatePipelines = true;
			}
		}
	}
	
	void ApplicationStats::OnFrameUpdate(float64 elapsed_time_in_seconds)
	{
		if (FrameCount + 1 >= ULONG_MAX)
		{
			FrameCount = 0;
			FrameCountOverflows++;
		}
		else
		{
			FrameCount++;
		}

		float64 elapsedInMS = elapsed_time_in_seconds * 1000.0;

		if (msPerFrame.GetSize() == msPerFrame.GetCapacity() && !(AccumulatedMs > 1000.0))
		{
			msPerFrame.Resize(msPerFrame.GetSize() * 1.5f);
		}

		msPerFrame.PushBack(elapsedInMS);
		AccumulatedMs += elapsedInMS;
		AccumulatedFramesPerSec++;

		if (AccumulatedMs > 1000.0)
		{
			MS_Average = AccumulatedMs;
			MS_Average /= float64(AccumulatedFramesPerSec);
			FPS_Average = AccumulatedFramesPerSec;
			AccumulatedFramesPerSec = 0;
			AccumulatedMs = 0.0;
			msPerFrame.Clear();

			PH_Profiler::CalculateAverageStats(FPS_Average);
			//PH_INFO("Average ms per frame: %f fps: %i ", MS_Average, FPS_Average);
		};
	}

}
