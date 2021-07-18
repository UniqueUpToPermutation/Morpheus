#pragma once

#include "SwapChain.h"

#include <Engine/Defines.hpp>
#include <Engine/Entity.hpp>

#include <shaders/BasicStructures.hlsl>

namespace Morpheus {

	HLSL::CameraAttribs CreateCameraAttribs(const DG::float2& viewportSize,
		const DG::float4x4& view, const DG::float4x4& projection,
		const DG::float4& eye, float nearPlane, float farPlane);

	DG::float4x4 GetSurfacePretransformMatrix(DG::ISwapChain* swapChain, 
		const DG::float3& f3CameraViewAxis);
	DG::float4x4 GetAdjustedProjectionMatrix(DG::ISwapChain* swapChain, 
		bool bIsGL, float FOV, float NearPlane, float FarPlane);
	DG::float4x4 GetAdjustedOrthoMatrix(
		bool bIsGL, const DG::float2& fCameraSize, float zNear, float zFar);

	enum class CameraType {
		PERSPECTIVE,
		ORTHOGRAPHIC
	};

	class Camera {
	private:
		DG::float3 mEye = DG::float3{0.0f, 0.0f, 0.0f};
		DG::float3 mLookAt = DG::float3{0.0f, 0.0f, 1.0f};
		DG::float3 mUp = DG::float3{0.0f, 1.0f, 0.0f};

		float mFieldOfView = DG::PI_F / 4.0f;
		float mNearPlane = 0.1f;
		float mFarPlane = 100.0f;

		DG::float2 mOrthoSize = DG::float2{0.0f, 0.0f};

		CameraType mType;

	public:
		Camera(CameraType type = CameraType::PERSPECTIVE);
		Camera(const Camera& other);

		// Does not take into account the transform of this node
		DG::float4x4 GetView() const;
		// Does not take into account the transform of this node
		DG::float4x4 GetProjection(DG::ISwapChain* swapChain, bool bIsGL) const;
		DG::float4x4 GetProjection(RealtimeGraphics& graphics) const;
		
		DG::float3 GetEye() const;

		inline void SetEye(const DG::float3& eye) {
			mEye = eye;
		}

		inline void SetEye(float x, float y, float z) {
			mEye.x = x;
			mEye.y = y;
			mEye.z = z;
		}

		inline void LookAt(const DG::float3& target) { 
			mLookAt = target;
		}

		inline void LookAt(float x, float y, float z) {
			mLookAt.x = x;
			mLookAt.y = y;
			mLookAt.z = z;
		}

		inline float GetFieldOfView() const {
			return mFieldOfView;
		}

		inline void SetFieldOfView(float value) {
			mFieldOfView = value;
		}

		inline CameraType GetType() const {
			return mType;
		}

		inline void SetType(CameraType type) {
			mType = type;
		}

		inline void SetOrthoSize(const DG::float2& size) {
			mOrthoSize = size;
		}

		inline void SetOrthoSize(float width, float height) {
			mOrthoSize.x = width;
			mOrthoSize.y = height;
		}

		inline DG::float2 GetOrthoSize() const {
			return mOrthoSize;
		}

		inline void SetClipPlanes(float fNear, float fFar) {
			mNearPlane = fNear;
			mFarPlane = fFar;
		}

		inline DG::float3 GetLookAt() const {
			return mLookAt;
		}

		inline float GetFarZ() const { 
			return mFarPlane;
		}

		inline float GetNearZ() const {
			return mNearPlane;
		}

		inline void SetFarPlane(const float z) {
			mFarPlane = z;
		}

		inline void SetNearPlane(const float z) {
			mNearPlane = z;
		}

		void ComputeTransformations(
			entt::entity entity,
			entt::registry* registry,
			DG::ISwapChain* swapChain,
			bool bIsGL,
			DG::float3* eye,
			DG::float3* lookAt,
			DG::float4x4* view,
			DG::float4x4* proj,
			DG::float4x4* viewProj);

		void ComputeTransformations(
			entt::entity entity,
			entt::registry* registry,
			RealtimeGraphics& graphics,
			DG::float3* eye,
			DG::float3* lookAt,
			DG::float4x4* view,
			DG::float4x4* proj,
			DG::float4x4* viewProj);
		
		HLSL::CameraAttribs GetLocalAttribs(DG::ISwapChain* swapChain, bool bIsGL);
		HLSL::CameraAttribs GetTransformedAttribs(entt::entity entity, entt::registry* registry, DG::ISwapChain* swapChain, bool bIsGL);
	
		HLSL::CameraAttribs GetLocalAttribs(RealtimeGraphics& graphics);
		HLSL::CameraAttribs GetTransformedAttribs(entt::entity entity, entt::registry* registry, RealtimeGraphics& graphics);
	};
}