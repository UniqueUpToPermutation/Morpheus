#include <Engine/Pipelines/PipelineFactory.hpp>
#include <Engine/Resources/PipelineResource.hpp>

namespace Morpheus {
	void ResourceCache<PipelineResource>::InitFactories() {
		mPipelineFactories["BasicTextured"] = &CreateBasicTexturedPipeline;
		mPipelineFactories["Skybox"] = &CreateSkyboxPipeline;
		mPipelineFactories["PBRStaticMesh"] = &CreateStaticMeshPBRPipeline;
		mPipelineFactories["White"] = &CreateWhitePipeline;
	}
}