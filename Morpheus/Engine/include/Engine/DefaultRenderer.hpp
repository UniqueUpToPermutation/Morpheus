#include <Engine/Renderer.hpp>
#include <Engine/Camera.hpp>
#include <Engine/StaticMeshComponent.hpp>
#include <Engine/Transform.hpp>
#include <Engine/Skybox.hpp>
#include <Engine/Brdf.hpp>

namespace DG = Diligent;

#define DEFAULT_INSTANCE_BATCH_SIZE 1024

namespace Morpheus {

	struct DefaultRendererGlobals
	{
		DG::float4x4 mView;
		DG::float4x4 mProjection;
		DG::float4x4 mViewProjection;
		DG::float4x4 mViewProjectionInverse;
		DG::float3 mEye;
		float mTime;
	};

	class GlobalsBuffer {
	private:
		DG::IBuffer* mGlobalsBuffer;
	public:
		GlobalsBuffer(DG::IRenderDevice* renderDevice);
		~GlobalsBuffer();

		void Update(DG::IDeviceContext* context,
			const DG::float4x4& view,
			const DG::float4x4& projection,
			const DG::float3& eye,
			float time);

		inline DG::IBuffer* GetBuffer() {
			return mGlobalsBuffer;
		}
	};

	struct StaticMeshCache {
		StaticMeshComponent* mStaticMesh;
		Transform* mTransform;
	};

	class DefaultRenderCache : public RenderCache {
	public:
		std::vector<StaticMeshCache> mStaticMeshes;
		SkyboxComponent* mSkybox;

		inline DefaultRenderCache() :
			mSkybox(nullptr) {
		}
		
		~DefaultRenderCache() {
		}
	};

	class DefaultRenderer : public Renderer {
	private:
		Engine* mEngine;
		GlobalsBuffer mGlobalsBuffer;
		DG::IBuffer* mInstanceBuffer;
		Transform mIdentityTransform;
		CookTorranceLUT mCookTorranceLut;

		void RenderStaticMeshes(std::vector<StaticMeshCache>& cache);
		void RenderSkybox(SkyboxComponent* skybox);

	public:
		DefaultRenderer(Engine* engine, 
			uint instanceBatchSize = DEFAULT_INSTANCE_BATCH_SIZE);
		~DefaultRenderer();
		
		void RequestConfiguration(DG::EngineD3D11CreateInfo* info) override;
		void RequestConfiguration(DG::EngineD3D12CreateInfo* info) override;
		void RequestConfiguration(DG::EngineGLCreateInfo* info) override;
		void RequestConfiguration(DG::EngineVkCreateInfo* info) override;
		void RequestConfiguration(DG::EngineMtlCreateInfo* info) override;

		void Initialize();
		void Render(RenderCache* cache, EntityNode cameraNode) override;

		DG::IBuffer* GetGlobalsBuffer() override;
		DG::FILTER_TYPE GetDefaultFilter() override;
		uint GetMaxAnisotropy() override;

		RenderCache* BuildRenderCache(SceneHeirarchy* scene) override;
	};
}