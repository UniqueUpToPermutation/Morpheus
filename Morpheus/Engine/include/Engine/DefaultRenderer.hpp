#include <Engine/Renderer.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Components/SkyboxComponent.hpp>
#include <Engine/Brdf.hpp>
#include <Engine/PostProcessor.hpp>
#include <Engine/DynamicGlobalsBuffer.hpp>
#include <Engine/Systems/RendererBridge.hpp>

namespace DG = Diligent;

#define DEFAULT_INSTANCE_BATCH_SIZE 1024

#include <Engine/RendererGlobalsData.hpp>

#include "MapHelper.hpp"

namespace Morpheus {

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
		uint mInstanceBatchSize;

		bool bUseSHIrradiance;

		void RenderStaticMeshes(entt::registry* registry,
			DefaultRendererBridge* renderBridge,
			LightProbe* globalLightProbe);
		void RenderSkybox(SkyboxComponent* skybox);
		void ReallocateIntermediateFramebuffer(uint width, uint height);
		void WriteGlobalData(EntityNode cameraNode, LightProbe* globalLightProbe);

	public:
		DefaultRenderer(uint instanceBatchSize = DEFAULT_INSTANCE_BATCH_SIZE);
		~DefaultRenderer();

		void OnWindowResized(uint width, uint height) override;
		
		void RequestConfiguration(DG::EngineD3D11CreateInfo* info) override;
		void RequestConfiguration(DG::EngineD3D12CreateInfo* info) override;
		void RequestConfiguration(DG::EngineGLCreateInfo* info) override;
		void RequestConfiguration(DG::EngineVkCreateInfo* info) override;
		void RequestConfiguration(DG::EngineMtlCreateInfo* info) override;
		
		void InitializeSystems(Scene* scene) override;
		void Initialize(Engine* engine) override;
		void Render(Scene* scene, EntityNode cameraNode, const RenderPassTargets& targets) override;

		DG::IBuffer* GetGlobalsBuffer() override;
		DG::FILTER_TYPE GetDefaultFilter() const override;
		uint GetMaxAnisotropy() const override;
		uint GetMSAASamples() const override;
		uint GetMaxRenderThreadCount() const override;

		DG::TEXTURE_FORMAT GetIntermediateFramebufferFormat() const override;
		DG::TEXTURE_FORMAT GetIntermediateDepthbufferFormat() const override;
		DG::TEXTURE_FORMAT GetBackbufferColorFormat() const override;
		DG::TEXTURE_FORMAT GetBackbufferDepthFormat() const override;
		DG::ITextureView* GetLUTShaderResourceView() override;
		bool GetUseSHIrradiance() const override;
		bool GetUseIBL() const override;

		DG::IRenderDevice* GetDevice() override;
		DG::IDeviceContext* GetImmediateContext() override;
	};
}