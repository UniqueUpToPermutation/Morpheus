#include <Engine/Renderer.hpp>

namespace DG = Diligent;

namespace Morpheus {
	class DefaultRenderer : public Renderer {
	private:
		Engine* mEngine;

		PipelineResource* mPipeline;

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
	};
}