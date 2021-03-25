#pragma once

#include "BasicMath.hpp"

#include <Engine/DynamicGlobalsBuffer.hpp>

namespace DG = Diligent;

using float2 = DG::float2;
using float3 = DG::float3;
using float4 = DG::float4;
using float4x4 = DG::float4x4;

namespace Morpheus {
	#include <shaders/BasicStructures.hlsl>
}

namespace Morpheus {
	class Camera;

	void WriteRenderGlobalsData(
		DynamicGlobalsBuffer<RendererGlobalData>* globals,
		DG::IDeviceContext* context,
		const DG::float2& viewportSize,
		Camera* camera,
		const DG::float4x4& projectionMatrix,
		const DG::float4x4* cameraTransform,
		LightProbe* globalLightProbe);
}