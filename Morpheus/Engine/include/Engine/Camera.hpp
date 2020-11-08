#pragma once

#include <Engine/Engine.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class Camera {
	public:
		virtual DG::float4x4 GetView() const = 0;
		virtual DG::float4x4 GetProjection(Engine* engine) const = 0;
		virtual DG::float3 GetEye() const = 0;
	};

	class PerspectiveLookAtCamera : public Camera {
	public:
		DG::float3 mEye = DG::float3{0.0f, 0.0f, 0.0f};
		DG::float3 mLookAt = DG::float3{0.0f, 0.0f, 1.0f};
		DG::float3 mUp = DG::float3{0.0f, 1.0f, 0.0f};

		float mFieldOfView = DG::PI_F / 4.0f;
		float mNearPlane = 0.1f;
		float mFarPlane = 100.0f;

		DG::float4x4 GetView() const override;
		DG::float4x4 GetProjection(Engine* engine) const override;
		DG::float3 GetEye() const override;
	};
}