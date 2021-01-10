#include <Engine/Renderer.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Transform.hpp>
#include <Engine/Skybox.hpp>
#include <Engine/Brdf.hpp>
#include <Engine/PostProcessor.hpp>

#include <Engine/Systems/RendererBridge.hpp>

namespace DG = Diligent;

#define DEFAULT_INSTANCE_BATCH_SIZE 1024

using float2 = DG::float2;
using float3 = DG::float3;
using float4 = DG::float4;
using float4x4 = DG::float4x4;

#include <shaders/BasicStructures.hlsl>

#include "MapHelper.hpp"

namespace Morpheus {

	template <typename T>
	class DynamicGlobalsBuffer {
	private:
		DG::IBuffer* mBuffer;

	public:
		inline DynamicGlobalsBuffer(DG::IRenderDevice* device) {
			DG::BufferDesc CBDesc;
			CBDesc.Name           = "VS constants CB";
			CBDesc.uiSizeInBytes  = sizeof(T);
			CBDesc.Usage          = DG::USAGE_DYNAMIC;
			CBDesc.BindFlags      = DG::BIND_UNIFORM_BUFFER;
			CBDesc.CPUAccessFlags = DG::CPU_ACCESS_WRITE;

			mBuffer = nullptr;
			device->CreateBuffer(CBDesc, nullptr, &mBuffer);
		}

		inline ~DynamicGlobalsBuffer() {
			mBuffer->Release();
		}

		inline DG::IBuffer* Get() {
			return mBuffer;
		}

		inline void Write(DG::IDeviceContext* context, const T& t) {
			DG::MapHelper<T> data(context, mBuffer, DG::MAP_WRITE, DG::MAP_FLAG_DISCARD);
			*data = t;
		}
	};

	class DefaultRenderer : public IRenderer {
	private:
		DynamicGlobalsBuffer<RendererGlobalData> mGlobals;

		Engine* mEngine;
		DG::IBuffer* mInstanceBuffer;
		Transform mIdentityTransform;
		CookTorranceLUT mCookTorranceLut;
		PostProcessor mPostProcessor;

		DG::ITexture* mFrameBuffer;
		DG::ITexture* mMSAADepthBuffer;
		DG::ITexture* mResolveBuffer;

		TextureResource* mBlackTexture;
		TextureResource* mWhiteTexture;
		TextureResource* mDefaultNormalTexture;

		DG::ISampler* mDefaultSampler;

		bool bUseSHIrradiance;

		void RenderStaticMeshes(entt::registry* registry,
			RendererBridge* renderBridge,
			LightProbe* globalLightProbe);
		void RenderSkybox(SkyboxComponent* skybox);
		void ReallocateIntermediateFramebuffer(uint width, uint height);
		void WriteGlobalData(EntityNode cameraNode, LightProbe* globalLightProbe);

	public:
		DefaultRenderer(Engine* engine, 
			uint instanceBatchSize = DEFAULT_INSTANCE_BATCH_SIZE);
		~DefaultRenderer();

		void OnWindowResized(uint width, uint height) override;
		
		void RequestConfiguration(DG::EngineD3D11CreateInfo* info) override;
		void RequestConfiguration(DG::EngineD3D12CreateInfo* info) override;
		void RequestConfiguration(DG::EngineGLCreateInfo* info) override;
		void RequestConfiguration(DG::EngineVkCreateInfo* info) override;
		void RequestConfiguration(DG::EngineMtlCreateInfo* info) override;
		
		void InitializeSystems(Scene* scene) override;
		void Initialize();
		void Render(Scene* scene, EntityNode cameraNode) override;

		DG::IBuffer* GetGlobalsBuffer() override;
		DG::FILTER_TYPE GetDefaultFilter() const override;
		uint GetMaxAnisotropy() const override;
		uint GetMSAASamples() const override;

		DG::TEXTURE_FORMAT GetIntermediateFramebufferFormat() const override;
		DG::TEXTURE_FORMAT GetIntermediateDepthbufferFormat() const override;
		DG::ITextureView* GetLUTShaderResourceView() override;
		bool GetUseSHIrradiance() const override;
		bool GetUseIBL() const override;

		DG::IRenderDevice* GetDevice() override;
		DG::IDeviceContext* GetImmediateContext() override;
	};
}