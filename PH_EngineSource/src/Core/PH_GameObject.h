#pragma once

#include "PH_CORE.h"
#include "PH_Mesh.h"
#include "Memory/PH_Memory.h"
#include "Containers/PH_String.h"
#include "Containers/PH_DynamicArray.h"

#include "PH_MATH.h"

namespace PhosphorEngine {

	using namespace PhosphorEngine::Containers;
	using namespace PhosphorEngine::Memory;

	// Forward declaration 
	class PH_GameObject;
	class PH_ObjectBuilder;

	struct  TransformComponent
	{
		Math::vec3 translation{1.f};
		Math::vec3 scale{ 1.f,1.f,1.f };
		Math::vec3 rotation{};
		
		Math::mat4x4 mat4();
		Math::mat3x3 normalMatrix();
	};

	struct  PointLightComponent {
		PointLightComponent() { lightIntensity = 1.f; Emit = true; }

		float lightIntensity = 1.0f;
		uint8 Emit : 1;
	};

	struct  DirectionalLightComponent {
		DirectionalLightComponent() { lightIntensity = 1.f; Emit = true; }

		Math::mat4x4 getProjection() { return Math::orthoLH_ZO(-shadowExtent.x, shadowExtent.x, -shadowExtent.y, shadowExtent.y, -shadowExtent.z, shadowExtent.z); }

		float lightIntensity = 1.0f;
		Math::vec3 lightDirection{ 1.f };
		Math::vec3 shadowExtent{ 1.f };
		uint8 Emit : 1;
	};

	class PH_GameObject
	{
	public:
		PH_GameObject(const PH_GameObject&) = delete;
		PH_GameObject& operator=(const PH_GameObject&) = delete;
		PH_GameObject(PH_GameObject&&) = default;
		PH_GameObject& operator=(PH_GameObject&&) = default;
		PH_GameObject(uint32 objectID, PH_String ObjectName) : id(objectID), Name(ObjectName) {}

		const uint32 getId() { return id; }

		PH_INLINE void AddTag(PH_String TagToAdd) {
			tags.push_back(TagToAdd);
		};

		PH_INLINE void AddTag(PH_DynamicArray<PH_String> TagsToAdd) {
			for(auto& tag : TagsToAdd)
				tags.push_back(tag);
		};

		PH_INLINE const PH_DynamicArray<PH_String>& GetTags() const { return tags; };

		PH_INLINE bool HasTag(PH_String searchTag) const 
		{ 
			for (auto& tag : tags)
			{
				if (searchTag == tag)
				{
					return true;
				};
			};
			return false;
		};

		Math::vec3 color;
		TransformComponent transform{};

		PH_SharedMemoryHandle<PH_Mesh> model{};
		PH_UniqueMemoryHandle<PointLightComponent> pointLight{};
		PH_UniqueMemoryHandle<DirectionalLightComponent> directionalLightComponent{};
		uint32 customSortKey;
		PH_INLINE PH_String GetName() const { return Name; };

	private:
		friend PH_ObjectBuilder;
		uint32 id;
		PH_DynamicArray<PH_String> tags;
		PH_String Name;
	};

	class PH_ObjectBuilder
	{
	public:
		using ObjectHandle = PH_UniqueMemoryHandle<PH_GameObject>;
		using Map = PH_Map<uint32, ObjectHandle>;

		PH_INLINE PH_STATIC_LOCAL ObjectHandle createGameObject(PH_String ObjectName) {
			auto* GameObject = ph_new<PH_GameObject>(PH_MEM_TAG_GAME_OBJECT, currentObjectId++, ObjectName);
			return ObjectHandle(GameObject);
		}

		PH_STATIC_LOCAL PH_ObjectBuilder::ObjectHandle createPointLight(
			Math::vec3 lightColor = Math::vec3(1.f),
			float intensity = 1.f,
			float radius = 1.f);

		PH_STATIC_LOCAL PH_ObjectBuilder::ObjectHandle createDirectionalLight(
			Math::vec3 lightColor = Math::vec3(1.f), 
			Math::vec3 lightDirection = Math::vec3{ 1.f },
			Math::vec3 shadowExtent = Math::vec3{ 1.f },
			float intensity = 1.f);
	private:
		PH_INLINE PH_STATIC_LOCAL uint32 currentObjectId = 0;
	};
}