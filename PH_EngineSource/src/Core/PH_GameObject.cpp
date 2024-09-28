#include "PH_GameObject.h"

namespace PhosphorEngine {

	Math::mat4x4 TransformComponent::mat4()
	{
		const float c3 = Math::cos(rotation.z);
		const float s3 = Math::sin(rotation.z);
		const float c2 = Math::cos(rotation.x);
		const float s2 = Math::sin(rotation.x);
		const float c1 = Math::cos(rotation.y);
		const float s1 = Math::sin(rotation.y);
		return Math::mat4x4{
			{
				scale.x * (c1 * c3 + s1 * s2 * s3),
				scale.x * (c2 * s3),
				scale.x * (c1 * s2 * s3 - c3 * s1),
				0.0f,
			},
			{
				scale.y * (c3 * s1 * s2 - c1 * s3),
				scale.y * (c2 * c3),
				scale.y * (c1 * c3 * s2 + s1 * s3),
				0.0f,
			},
			{
				scale.z * (c2 * s1),
				scale.z * (-s2),
				scale.z * (c1 * c2),
				0.0f,
			},
			{translation.x, translation.y, translation.z, 1.0f} };
	}

	Math::mat3x3 TransformComponent::normalMatrix()
	{
		const float c3 = Math::cos(rotation.z);
		const float s3 = Math::sin(rotation.z);
		const float c2 = Math::cos(rotation.x);
		const float s2 = Math::sin(rotation.x);
		const float c1 = Math::cos(rotation.y);
		const float s1 = Math::sin(rotation.y);
		const Math::vec3 invScale = 1.f/scale;
		return Math::mat3x3{
			{
				invScale.x * (c1 * c3 + s1 * s2 * s3),
				invScale.x * (c2 * s3),
				invScale.x * (c1 * s2 * s3 - c3 * s1),
			},
			{
				invScale.y * (c3 * s1 * s2 - c1 * s3),
				invScale.y * (c2 * c3),
				invScale.y * (c1 * c3 * s2 + s1 * s3),
			},
			{
				invScale.z * (c2 * s1),
				invScale.z * (-s2),
				invScale.z * (c1 * c2),
			}};
		return Math::mat3x3();
	}
	
	PH_ObjectBuilder::ObjectHandle PH_ObjectBuilder::createPointLight(Math::vec3 lightColor, float intensity, float radius)
	{
		auto gameObj = createGameObject(PH_String("PointLight") + StringUtilities::NumberToString(currentObjectId));
		gameObj->color = lightColor;
		gameObj->transform.scale.x = radius;
		gameObj->pointLight = PH_UniqueMemoryHandle<PointLightComponent>::CreateWithTag(PH_MEM_TAG_GAME_OBJECT);
		gameObj->pointLight->lightIntensity = intensity;
		gameObj->pointLight->Emit = true;
		return std::move(gameObj);
	}

	PH_ObjectBuilder::ObjectHandle PH_ObjectBuilder::createDirectionalLight(Math::vec3 lightColor, Math::vec3 lightDirection, Math::vec3 shadowExtent, float intensity)
	{
		auto gameObj = createGameObject(PH_String("PointLight") + StringUtilities::NumberToString(currentObjectId));

		gameObj->color = lightColor;
		gameObj->directionalLightComponent = PH_UniqueMemoryHandle<DirectionalLightComponent>::CreateWithTag(PH_MEM_TAG_GAME_OBJECT);
		gameObj->directionalLightComponent->lightDirection = lightDirection;
		gameObj->directionalLightComponent->shadowExtent = shadowExtent;
		gameObj->directionalLightComponent->lightIntensity = intensity;
		gameObj->pointLight->Emit = true;
		return std::move(gameObj);
	}

}