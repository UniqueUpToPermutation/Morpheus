#pragma once

#include <im3d.h>

#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Renderer.hpp>
#include <Engine/DynamicGlobalsBuffer.hpp>

namespace Morpheus {

	class Camera;

	struct Im3dGlobals {
		DG::float4x4 mViewProjection;
		DG::float2 mScreenSize;
	};

	class Im3dGlobalsBuffer : public DynamicGlobalsBuffer<Im3dGlobals> {
	public:
		inline Im3dGlobalsBuffer() : 
			DynamicGlobalsBuffer<Im3dGlobals>() {
		}

		inline Im3dGlobalsBuffer(DG::IRenderDevice* device) : 
			DynamicGlobalsBuffer<Im3dGlobals>(device) {
		}

		inline void Write(DG::IDeviceContext* context, 
			const DG::float4x4& viewProjection,
			const DG::float2& screenSize) {
			DynamicGlobalsBuffer<Im3dGlobals>::Write(context,
				Im3dGlobals{viewProjection, screenSize});
		}

		void Write(DG::IDeviceContext* context,
			Camera* camera,
			Engine* engine);
	};

	class Im3dRendererFactory {
	private:
		DG::IPipelineState* mPipelineStateVertices = nullptr;
		DG::IPipelineState* mPipelineStateLines = nullptr;
		DG::IPipelineState* mPipelineStateTriangles = nullptr;
		DG::IShaderResourceBinding* mVertexSRB = nullptr;
		DG::IShaderResourceBinding* mLinesSRB = nullptr;
		DG::IShaderResourceBinding* mTriangleSRB = nullptr;

	public:
		friend class Im3dRenderer;

		inline ~Im3dRendererFactory() {
			if (mPipelineStateVertices)
				mPipelineStateVertices->Release();
			if (mPipelineStateLines)
				mPipelineStateLines->Release();
			if (mPipelineStateTriangles)
				mPipelineStateTriangles->Release();
			if (mVertexSRB)
				mVertexSRB->Release();
			if (mLinesSRB)
				mLinesSRB->Release();
			if (mTriangleSRB)
				mTriangleSRB->Release();
		}

		// Make sure that the Im3dGlobalsBuffer remains in scope for
		// the lifetime of all Im3dRenderers
		void Initialize(DG::IRenderDevice* device,
			Im3dGlobalsBuffer* globals,
			DG::TEXTURE_FORMAT backbufferColorFormat,
			DG::TEXTURE_FORMAT backbufferDepthFormat,
			uint backbufferMSAASamples = 1);
	};

	class Im3dRenderer {
	private:
		DG::IPipelineState* mPipelineStateVertices;
		DG::IPipelineState* mPipelineStateLines;
		DG::IPipelineState* mPipelineStateTriangles;
		DG::IShaderResourceBinding* mVertexSRB;
		DG::IShaderResourceBinding* mLinesSRB;
		DG::IShaderResourceBinding* mTriangleSRB;
		DG::IBuffer* mGeometryBuffer;
		uint mBufferSize;

	public:
		Im3dRenderer(DG::IRenderDevice* device, 
			Im3dRendererFactory* factory,
			uint bufferSize = 200u);

		void Draw(DG::IDeviceContext* deviceContext,
			Im3d::Context* im3dContext = &Im3d::GetContext());

		inline ~Im3dRenderer() {
			mPipelineStateVertices->Release();
			mPipelineStateTriangles->Release();
			mPipelineStateLines->Release();
			mVertexSRB->Release();
			mLinesSRB->Release();
			mTriangleSRB->Release();
		}

		inline uint GetBufferSize() const {
			return mBufferSize;
		}
	};
}