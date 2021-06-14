#pragma once

#include <im3d.h>

#include <Engine/Entity.hpp>
#include <Engine/DynamicGlobalsBuffer.hpp>
#include <Engine/ThreadPool.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/Resources/EmbeddedFileLoader.hpp>

#include "BasicMath.hpp"

#define DEFAULT_IM3D_BUFFER_SIZE 200u

namespace DG = Diligent;

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

		inline Im3dGlobalsBuffer(Graphics& graphics) :
			DynamicGlobalsBuffer<Im3dGlobals>(graphics.Device()) {
		}

		void Write(DG::IDeviceContext* context, 
			const DG::float4x4& viewProjection,
			const DG::float2& screenSize);
		void WriteWithoutTransformCache(DG::IDeviceContext* context,
			Graphics& graphics,
			const Camera& camera);
		void Write(DG::IDeviceContext* context,
			Graphics& graphics,
			entt::entity camera,
			entt::registry* registry);
	};

	struct Im3dShaders {
		Handle<DG::IShader> mTrianglesVS;
		Handle<DG::IShader> mOtherVS;
		Handle<DG::IShader> mPointsGS;
		Handle<DG::IShader> mLinesGS;
		Handle<DG::IShader> mTrianglesPS;
		Handle<DG::IShader> mLinesPS;
		Handle<DG::IShader> mPointsPS;

		static ResourceTask<Im3dShaders> LoadDefault(DG::IRenderDevice* device, 
			IVirtualFileSystem* system = EmbeddedFileLoader::GetGlobalInstance());

		static inline ResourceTask<Im3dShaders> LoadDefault(Graphics& graphics,
			IVirtualFileSystem* system = EmbeddedFileLoader::GetGlobalInstance()) {
			return LoadDefault(graphics.Device(), system);
		}
	};

	struct Im3dState;

	struct Im3dPipeline {
		Handle<DG::IPipelineState> mPipelineStateVertices;
		Handle<DG::IPipelineState> mPipelineStateLines;
		Handle<DG::IPipelineState> mPipelineStateTriangles;
		Handle<DG::IShaderResourceBinding> mVertexSRB;
		Handle<DG::IShaderResourceBinding> mLinesSRB;
		Handle<DG::IShaderResourceBinding> mTriangleSRB;
		Im3dShaders mShaders;

		Im3dPipeline(DG::IRenderDevice* device,
			Im3dGlobalsBuffer* globals,
			DG::TEXTURE_FORMAT backbufferColorFormat,
			DG::TEXTURE_FORMAT backbufferDepthFormat,
			uint samples,
			const Im3dShaders& shaders);

		inline Im3dPipeline(Graphics& graphics,
			Im3dGlobalsBuffer* globals,
			DG::TEXTURE_FORMAT backbufferColorFormat,
			DG::TEXTURE_FORMAT backbufferDepthFormat,
			uint samples,
			const Im3dShaders& shaders) :
			Im3dPipeline(graphics.Device(), globals, backbufferColorFormat,
				backbufferDepthFormat, samples, shaders) {
		}

		inline Im3dPipeline(Graphics& graphics,
			Im3dGlobalsBuffer* globals,
			uint samples,
			const Im3dShaders& shaders) :
			Im3dPipeline(graphics.Device(), globals, 
				graphics.SwapChain()->GetDesc().ColorBufferFormat,
				graphics.SwapChain()->GetDesc().DepthBufferFormat,
				samples,
				shaders) {
		}

		inline Im3dPipeline(Graphics& graphics,
			Im3dGlobalsBuffer* globals,
			const Im3dShaders& shaders) : Im3dPipeline(graphics.Device(), globals, 
				graphics.SwapChain()->GetDesc().ColorBufferFormat,
				graphics.SwapChain()->GetDesc().DepthBufferFormat,
				1,
				shaders) {
		}

		inline Im3dPipeline() {
		}

		Im3dState CreateState();

		static ResourceTask<Im3dPipeline> LoadDefault(DG::IRenderDevice* device, 
			IVirtualFileSystem* system = EmbeddedFileLoader::GetGlobalInstance());
	};

	class Im3dRenderer {
	private:
		Handle<DG::IBuffer> mGeometryBuffer;
		uint mBufferSize;

	public:
		Im3dRenderer(DG::IRenderDevice* device,
			uint bufferSize = DEFAULT_IM3D_BUFFER_SIZE);

		inline Im3dRenderer(Graphics& graphics,
			uint bufferSize = DEFAULT_IM3D_BUFFER_SIZE) : 
			Im3dRenderer(graphics.Device(), bufferSize) {
		}

		void Draw(DG::IDeviceContext* deviceContext,
			const Im3dPipeline& state,
			Im3d::Context* im3dContext = &Im3d::GetContext());

		inline uint GetBufferSize() const {
			return mBufferSize;
		}
	};
}