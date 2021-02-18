#pragma once

#include <im3d.h>

#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Renderer.hpp>
#include <Engine/DynamicGlobalsBuffer.hpp>

namespace Morpheus {

	struct Im3dGlobals {
		DG::float4x4 mViewProjection;
		DG::float2 mScreenSize;
	};

	class Im3dRendererFactory {
	private:
		DG::IPipelineState* mPipelineStateVertices = nullptr;
		DG::IPipelineState* mPipelineStateLines = nullptr;
		DG::IPipelineState* mPipelineStateTriangles = nullptr;

	public:
		friend class Im3dRenderer;

		inline ~Im3dRendererFactory() {
			if (mPipelineStateVertices)
				mPipelineStateVertices->Release();
			if (mPipelineStateLines)
				mPipelineStateLines->Release();
			if (mPipelineStateTriangles)
				mPipelineStateTriangles->Release();
		}

		void Initialize(DG::IRenderDevice* device,
			DG::TEXTURE_FORMAT backbufferColorFormat,
			DG::TEXTURE_FORMAT backbufferDepthFormat,
			uint backbufferMSAASamples = 1);
	};

	class Im3dRenderer {
	private:
		DG::IPipelineState* mPipelineStateVertices;
		DG::IPipelineState* mPipelineStateLines;
		DG::IPipelineState* mPipelineStateTriangles;
		DynamicGlobalsBuffer<Im3dGlobals> mGlobals;
		DG::IBuffer* mGeometryBuffer;
		uint mBufferSize;

	public:
		Im3dRenderer(DG::IRenderDevice* device, 
			Im3dRendererFactory* factory,
			uint bufferSize = 200u);

		void Draw(DG::IDeviceContext* deviceContext,
			const Im3dGlobals& globals,
			Im3d::Context* im3dContext = &Im3d::GetContext());

		inline ~Im3dRenderer() {
			mPipelineStateVertices->Release();
			mPipelineStateTriangles->Release();
			mPipelineStateLines->Release();
		}

		inline uint GetBufferSize() const {
			return mBufferSize;
		}
	};
}