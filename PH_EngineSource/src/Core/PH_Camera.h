#pragma once

#include "PH_CORE.h"
#include "PH_MATH.h"

#include <map>


namespace PhosphorEngine {

	class  PH_Camera
	{
	public:
		void setOrthographicProjection(float left, float right, float top, float bottom, float near, float far);

		void setPerspectiveProjection(float fovy, float aspect, float near, float far);

		void setViewDirection(Math::vec3 position, Math::vec3 direction, Math::vec3 up = Math::vec3{0.f, -1.f, 0.f});

		void setViewTarget(Math::vec3 position, Math::vec3 target, Math::vec3 up = Math::vec3{ 0.f, -1.f, 0.f });

		void setViewYXZ(Math::vec3 position, Math::vec3 rotation);

		const Math::mat4x4& getProjectionMatrix() const { return projectionMatrix; }
		const Math::mat4x4& getView() const { return viewMatrix; }	
		const Math::mat4x4 getInverseViewMatrix()const { return inverseViewMatrix; };
		const Math::vec3 getPosition() const { return inverseViewMatrix.vectors[3].xyz; }

	private:
		Math::mat4x4 projectionMatrix{1.f};
		Math::mat4x4 viewMatrix{ 1.f };
		Math::mat4x4 inverseViewMatrix{ 1.f };
	};
}