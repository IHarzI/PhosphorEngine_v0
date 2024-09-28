#pragma once

#include "PH_MATH.h"

#include "vulkan/vulkan.h"

#include "Containers/PH_StaticArray.h"

namespace PhosphorEngine {

	using namespace PhosphorEngine::Containers;

	#define MAX_POINT_LIGHTS 10
	#define MAX_DIRECTIONAL_LIGHTS 5

	struct  PointLight {
		Math::vec4 position{};
		Math::vec4 color{};
	};

	struct  DirectionalLight {
		Math::vec4 position{};
		Math::vec4 color{};
		Math::vec3 shadowExtent;
		Math::vec3 direction{};

		Math::mat4x4 getProjection() {
			return Math::orthoLH_ZO(-shadowExtent.x, shadowExtent.x, -shadowExtent.y, shadowExtent.y, -shadowExtent.z, shadowExtent.z);
		};
		Math::mat4x4 getView() {
			Math::vec3 CenterV = position.xyz + direction;
			Math::vec3 Up{ 0.f,-1.f,0.f }; 
			return Math::LookAt(position.xyz, CenterV, Up);
		}
	};

	struct  GlobalUbo {
		Math::mat4x4 CameraProjection{ 1.f };
		Math::mat4x4 CameraView{ 1.f };
		Math::mat4x4 CameraInverseView{ 1.f };
		Math::mat4x4 DirectionalShadowMatrix{ 0.f };
		Math::mat4x4 MainLightProjection{ 1.f };
		Math::mat4x4 MainLightView{ 1.f };
		Math::vec4 ambientLightCol{ 0.5f,0.6f,0.7f,0.01f };
		int32 numPointLight{ 0 };
		int32 numDirectionalLight{ 0 };
		float DirectionalLightShadowCast = 0.f;
		int32 padding{};
		PH_StaticArray<PointLight, MAX_POINT_LIGHTS> pointLights;

		// Curently supported shadows from 1 directional light, main direcitonal light is first in array
		PH_StaticArray<DirectionalLight, MAX_DIRECTIONAL_LIGHTS> DirectionalLights;
	};

	struct ObjectModelData
	{
		Math::mat4x4 modelMatrix{};
		Math::mat4x4 normalMatrix{};
	};

	class PH_Camera;
	class PH_GameWorld;
	class PH_GameObject;
	struct PH_ImageTexture;
	class PH_PushBuffer;

	struct PH_FrameInfo{
		PH_FrameInfo(
			int32 frameIndex, 
			float64 frameTime, 
			VkCommandBuffer CommandBuffer,
			PH_Camera* Camera, 
			VkDescriptorSet globalDescriptorSet, 
			PH_PushBuffer* DynamicData,
			PH_GameWorld* GameWorld,
			PH_GameObject* Skybox = nullptr,
			PH_ImageTexture* EnvMap = nullptr,
			bool EnableBloom = false,
			bool DisplaySkybox = false)
			: 
			frameIndex(frameIndex), 
			frameTime(frameTime), 
			commandBuffer(CommandBuffer), 
			camera(Camera), 
			globalDescriptorSet(globalDescriptorSet),
			dynamicData(DynamicData),
			GameWorld(GameWorld),
			Skybox(Skybox),
			EnvMap(EnvMap)
		{
			Bloom = EnableBloom;
			DisplaySkybox = DisplaySkybox;
		};

		int32 frameIndex;
		float32 frameTime;
		VkCommandBuffer commandBuffer;
		PH_Camera* camera;
		VkDescriptorSet globalDescriptorSet;
		PH_PushBuffer* dynamicData;
		PH_GameWorld* GameWorld;
		PH_GameObject* Skybox;
		PH_ImageTexture* EnvMap;

		uint8 Bloom : 1;
		uint8 DisplaySkybox : 1;
	};
}