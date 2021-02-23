#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Resources/PipelineResource.hpp>
#include <Engine/Renderer.hpp>

namespace Morpheus {
	void ResourceCache<PipelineResource>::InitFactories() {
		mPipelineFactories["BasicTextured"] = &CreateBasicTexturedPipeline;
		mPipelineFactories["Skybox"] = &CreateSkyboxPipeline;
		mPipelineFactories["PBRStaticMesh"] = &CreateStaticMeshPBRPipeline;
		mPipelineFactories["White"] = &CreateWhitePipeline;
	}

	std::vector<DG::IShaderResourceBinding*> GenerateSRBs(
		DG::IPipelineState* pipelineState, IRenderer* renderer) {

		std::vector<DG::IShaderResourceBinding*> srbs;
		srbs.resize(renderer->GetMaxRenderThreadCount());

		for (uint i = 0; i < srbs.size(); ++i) {
			DG::IShaderResourceBinding* srb = nullptr;
			pipelineState->CreateShaderResourceBinding(&srb, true);
			srbs[i] = srb;
		}

		return srbs;
	}
}