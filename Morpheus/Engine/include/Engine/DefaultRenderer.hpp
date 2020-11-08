#include <Engine/Renderer.hpp>
#include <Engine/Camera.hpp>

namespace DG = Diligent;

namespace Morpheus {

	struct DefaultRendererGlobals
	{
		DG::float4x4 mWorld;
		DG::float4x4 mView;
		DG::float4x4 mProjection;
		DG::float4x4 mWorldViewProjection;
		DG::float4x4 mWorldView;
		DG::float4x4 mWorldViewProjectionInverse;
		DG::float4x4 mViewProjectionInverse;
		DG::float4x4 mWorldInverseTranspose;
		DG::float4x4 mWorldViewInverseTranspose;
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
			const DG::float4x4& world,
			const DG::float4x4& view,
			const DG::float4x4& projection,
			const DG::float3& eye,
			float time);

		inline DG::IBuffer* GetBuffer() {
			return mGlobalsBuffer;
		}
	};
	class DefaultRenderer : public Renderer {
	private:
		PerspectiveLookAtCamera mCamera;
		Engine* mEngine;
		GlobalsBuffer mGlobalsBuffer;

		PipelineResource* mPipeline;
		GeometryResource* mGeometry;
		DG::IShaderResourceBinding* mSRB;

	public:
		DefaultRenderer(Engine* engine);
		~DefaultRenderer();
		
		void RequestConfiguration(DG::EngineD3D11CreateInfo* info) override;
		void RequestConfiguration(DG::EngineD3D12CreateInfo* info) override;
		void RequestConfiguration(DG::EngineGLCreateInfo* info) override;
		void RequestConfiguration(DG::EngineVkCreateInfo* info) override;
		void RequestConfiguration(DG::EngineMtlCreateInfo* info) override;

		void Initialize();
		void Render() override;

		DG::IBuffer* GetGlobalsBuffer() override;
		DG::FILTER_TYPE GetDefaultFilter() override;
		uint GetMaxAnisotropy() override;
	};
}