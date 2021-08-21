#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Graphics.hpp>
#include <Engine/Buffer.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/RendererTransformCache.hpp>
#include <Engine/LightProbeProcessor.hpp>
#include <Engine/Resources/EmbeddedFileLoader.hpp>
#include <Engine/Renderer.hpp>
#include <Engine/Resources/Geometry.hpp>

#include <shaders/BasicStructures.hlsl>

namespace Morpheus {
	VertexLayout DefaultInstancedStaticMeshLayout();

	class DefaultRenderer : 
		public ISystem, 
		public IRenderer, 
		public IVertexFormatProvider {
	public:
		struct MaterialApplyParams {
			LightProbe* mGlobalLightProbe;
		};

	private:
		struct Material {
			MaterialDesc mDesc;
			DG::IPipelineState* mPipeline = nullptr;
			DG::IShaderResourceBinding* mBinding = nullptr;
			DG::IShaderResourceVariable* mIrradianceSHVariable = nullptr;
			DG::IShaderResourceVariable* mEnvMapVariable = nullptr;
			std::atomic<int> mRefCount = 0;

			inline Material(const MaterialDesc& desc,
				DG::IPipelineState* pipeline,
				DG::IShaderResourceBinding* binding) :
				mDesc(desc),
				mPipeline(pipeline),
				mBinding(binding) {
			}

			inline Material() {
			}

			inline Material(Material&& data) :
				mDesc(std::move(data.mDesc)) {
				std::swap(data.mPipeline, mPipeline);
				std::swap(data.mBinding, mBinding);
				std::swap(data.mIrradianceSHVariable, mIrradianceSHVariable);
				std::swap(data.mEnvMapVariable, mEnvMapVariable);
				mRefCount = data.mRefCount.exchange(mRefCount);
			}

			inline Material& operator=(Material&& data) {
				mDesc = std::move(data.mDesc);
				std::swap(data.mPipeline, mPipeline);
				std::swap(data.mBinding, mBinding);
				std::swap(data.mIrradianceSHVariable, mIrradianceSHVariable);
				std::swap(data.mEnvMapVariable, mEnvMapVariable);
				mRefCount = data.mRefCount.exchange(mRefCount);
				return *this;
			}
		};

		TransformCacheUpdater mUpdater;
		VertexLayout mStaticMeshLayout = DefaultInstancedStaticMeshLayout();

		std::mutex mMaterialAddRefMutex;
		std::mutex mMaterialReleaseMutex;
		std::vector<MaterialId> mMaterialsToAddRef;
		std::vector<MaterialId> mMaterialsToRelease;

		std::shared_mutex mMaterialsMutex;
		std::unordered_map<MaterialId, Material> mMaterials;
		MaterialId mCurrentMaterialId = 0;

		MaterialId CreateNewMaterialId();

		EmbeddedFileLoader mLoader;
		RealtimeGraphics* mGraphics;
		uint mInstanceBatchSize = 512;

		std::shared_ptr<Task> mRenderGroup;

		struct Resources {
			DynamicGlobalsBuffer<HLSL::ViewAttribs> 	mViewData;
			DynamicGlobalsBuffer<HLSL::MaterialAttribs> mMaterialData;

			Handle<DG::IShader>						mStaticMeshVS = nullptr;

			struct {
				Handle<DG::IShader>					mLambertPS = nullptr;
				Handle<DG::IPipelineState>			mStaticMeshPipeline = nullptr;
			} mLambertIBL;

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

		void OnApplyCookTorrence(Material& mat, const MaterialApplyParams& params);
		void OnApplyLambert(Material& mat, const MaterialApplyParams& params);

		Material CreateCookTorrenceMaterial(const MaterialDesc& desc);
		Material CreateLambertMaterial(const MaterialDesc& desc);

		void CreateLambertPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps);
		void CreateCookTorrenceIBLPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps);
		void CreateSkyboxPipeline(Handle<DG::IShader> vs, Handle<DG::IShader> ps);
		void InitializeDefaultResources();

		TaskNode BeginRender(Future<RenderParams> params);
		TaskNode DrawBackground(Future<RenderParams> params);
		TaskNode DrawStaticMeshes(Future<RenderParams> params);

	public:
		void ApplyMaterial(DG::IDeviceContext* context, MaterialId id, 
			const MaterialApplyParams& params);
		std::shared_ptr<Task> Render(Future<RenderParams> params);

		std::unique_ptr<Task> Startup(SystemCollection& collection) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;
	
		const VertexLayout& GetStaticMeshLayout() const override;
		MaterialId CreateUnmanagedMaterial(const MaterialDesc& desc) override;

		void AddMaterialRef(MaterialId id) override;
		void ReleaseMaterial(MaterialId id) override;
		MaterialDesc GetMaterialDesc(MaterialId id) override;

		GraphicsCapabilityConfig GetCapabilityConfig() const;

		uint GetMaxAnisotropy() const;
		uint GetMSAASamples() const;

		DG::TEXTURE_FORMAT GetIntermediateFramebufferFormat() const;
		DG::TEXTURE_FORMAT GetIntermediateDepthbufferFormat() const;

		inline auto& Resources() {
			return mResources;
		}

		inline auto& CacheUpdater() {
			return mUpdater;
		}

		inline RealtimeGraphics* GetGraphics() {
			return mGraphics;
		}

		inline DefaultRenderer(RealtimeGraphics& graphics) : 
			mGraphics(&graphics) {
		}

		inline ~DefaultRenderer() {
			Shutdown();
		}
	};
}