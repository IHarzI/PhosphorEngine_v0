#pragma once

#include "Core/PH_FrameInfo.h"
#include "Containers/PH_StaticArray.h"

namespace PhosphorEngine {

	struct DrawContext {
		PH_FrameInfo& FrameInfo;
	};

	class IRenderable
	{
		virtual void Draw(DrawContext DrawContext) = 0;
	};

	enum class MeshpassType : uint8 {
		None = 0,
		Forward = 1,
		Transparency = 2,
		DirectionalShadow = 3,
		COUNT
	};

	template<typename T>
	struct PerPassData {
	public:
		T& operator[](MeshpassType pass)
		{
			switch (pass)
			{
			case MeshpassType::Forward:
				return data[0];
			case MeshpassType::Transparency:
				return data[1];
			case MeshpassType::DirectionalShadow:
				return data[2];
			}
			PH_ASSERT(false);
			return data[0];
		};

		void clear(T&& val)
		{
			for (int i = 0; i < 3; i++)
			{
				data[i] = val;
			}
		}

	private:
		PH_StaticArray<T, 3> data;
	};

	struct RenderBounds {
		Math::vec3 origin;
		float radius;
		Math::vec3 extents;
		bool valid;
	};

	struct GPUObjectData {
		Math::mat4x4 modelMatrix;
		Math::vec4 origin_rad; // bounds
		Math::vec4 extents;  // bounds
	};

	struct DrawCullData
	{
		Math::mat4x4 viewMat;
		float P00, P11, znear, zfar; // symmetric projection parameters
		float frustum[4]; // data for left/right/top/bottom frustum planes
		float lodBase, lodStep; // lod distance i = base * pow(step, i)
		float pyramidWidth, pyramidHeight; // depth pyramid size in texels

		uint32 drawCount;

		int cullingEnabled;
		int lodEnabled;
		int occlusionEnabled;
		int distanceCheck;
		int AABBcheck;
		float aabbmin_x;
		float aabbmin_y;
		float aabbmin_z;
		float aabbmax_x;
		float aabbmax_y;
		float aabbmax_z;

	};
}