#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/DynamicGlobalsBuffer.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/RendererTransformCache.hpp>
#include <Engine/Brdf.hpp>
#include <Engine/Resources/EmbeddedFileLoader.hpp>
#include <Engine/Systems/Renderer.hpp>
#include <Engine/Resources/Geometry.hpp>

#include <shaders/BasicStructures.hlsl>

namespace Morpheus {
	struct DefaultMaterialData {
		MaterialDesc mDesc;
		DG::IShaderResourceBinding* mBinding;
		std::atomic<int> mRefCount;
	};

	VertexLayout DefaultStaticMeshLayout();

	class DefaultRenderer : public ISystem, 
		public IRenderer, public IVertexFormatProvider {
	private:
		TransformCacheUpdater mUpdater;
		VertexLayout mStaticMeshLayout = DefaultStaticMeshLayout();

		entt::registry mMaterialRegistry;

		EmbeddedFileLoader mLoader;
		Graphics* mGraphics;
		uint mInstanceBatchSize = 512;

		struct Resources {
			DynamicGlobalsBuffer<HLSL::CameraAttribs> mCameraData;

			Handle<DG::IShader>		mStaticMeshVS = nullptr;

			struct {
				Handle<DG::IShader>					mLambertPS = nullptr;
				Handle<DG::IShader>					mLambertVS = nullptr;
				Handle<DG::IPipelineState>			mStaticMeshPipeline = nullptr;
			} mLambert;

			struct {
				Handle<DG::IShader> 				mCookTorrencePS = nullptr;
				Handle<DG::IPipelineState> 			mStaticMeshPipeline = nullptr;
			} mCookTorrenceIBL;

			struct {
				Handle<DG::IShader> 				mVS = nullptr;
				Handle<DG::IShader>					mPS = nullptr;
				Handle<DG::IPipelineState>			mPipeline = nullptr;
				Handle<DG::IShaderResourceBinding> 	mSkyboxBinding = nullptr;
				DG::IShaderResourceVariable*		mTexture = nullptr;
			} mSkybox;

			Handle<DG::ISampler> 	mDefaultSampler = nullptr;
			Handle<DG::IBuffer> 	mInstanceBuffer = nullptr;
			Handle<DG::ITexture> 	mBlackTexture = nullptr;
			Handle<DG::ITexture> 	mWhiteTexture = nullptr;
			Handle<DG::ITexture> 	mNormalTexture = nullptr;

			DG::ITextureView* 		mBlackSRV = nullptr;
			DG::ITextureView*		mWhiteSRV = nullptr;
			DG::ITextureView*		mDefaultNormalSRV = nullptr;

			CookTorranceLUT 		mCookTorrenceLUT;
		} mResources;

		bool bIsInitialized = false;

		void CreateLambertPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps);
		void CreateCookTorrenceIBLPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps);
		void CreateSkyboxPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps);
		void InitializeDefaultResources();

		ParameterizedTask<RenderParams> BeginRender();
		ParameterizedTask<RenderParams> DrawBackground();

	public:
		inline auto& Resources() {
			return mResources;
		}

		inline TransformCacheUpdater& CacheUpdater() {
			return mUpdater;
		}

		ParameterizedTaskGroup<RenderParams>* CreateRenderGroup();

		inline Graphics* GetGraphics() {
			return mGraphics;
		}

		Task Startup(SystemCollection& collection) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;
		
		inline DefaultRenderer(Graphics& graphics) : mGraphics(&graphics) {
		}

		inline ~DefaultRenderer() {
			Shutdown();
		}
	
		const VertexLayout& GetStaticMeshLayout() const override;
		MaterialId CreateUnmanagedMaterial(const MaterialDesc& desc) override;

		void AddMaterialRef(MaterialId id) override;
		void ReleaseMaterial(MaterialId id) override;

		GraphicsCapabilityConfig GetCapabilityConfig() const;

		uint GetMaxAnisotropy() const;
		uint GetMSAASamples() const;

		DG::TEXTURE_FORMAT GetIntermediateFramebufferFormat() const;
		DG::TEXTURE_FORMAT GetIntermediateDepthbufferFormat() const;
	};
}