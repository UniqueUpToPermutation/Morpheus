#pragma once

#include "BasicMath.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"

#include <Engine/Resources/ResourceManager.hpp>

using float2 = Diligent::float2;
using float3 = Diligent::float3;
using float4 = Diligent::float4;

#include <shaders/PostProcessorStructures.hlsl>

namespace DG = Diligent;

namespace Morpheus {
	class PostProcessor {
	private:
		DG::IPipelineState* mPipeline;
		DG::IShaderResourceBinding* mShaderResources;
		DG::IBuffer* mParameterBuffer;
		DG::IShaderResourceVariable* mInputFramebuffer;

	public:
		PostProcessor(DG::IRenderDevice* device);
		~PostProcessor();

		void SetAttributes(DG::IDeviceContext* context,
			const PostProcessorParams& params);

		void Initialize(DG::IRenderDevice* device, 
			DG::TEXTURE_FORMAT renderTargetColorFormat,
			DG::TEXTURE_FORMAT depthStencilFormat);

		void Draw(DG::IDeviceContext* context,
			DG::ITextureView* inputShaderResource);
	};
}