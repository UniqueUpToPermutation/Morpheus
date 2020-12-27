#include <Engine/Skybox.hpp>

namespace Morpheus {
	void SkyboxComponent::LoadPipeline(ResourceManager* manager) {
		mPipeline = manager->Load<PipelineResource>("Skybox");

		mPipeline->GetState()->CreateShaderResourceBinding(&mResourceBinding, true);
		if (mCubemap) {
			auto textureVar = mResourceBinding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture");
			if (textureVar)
				textureVar->Set(mCubemap->GetShaderView());
		}
	}

	void SkyboxComponent::SetCubemap(TextureResource* resource) {
		if (mCubemap)
			mCubemap->Release();
		mCubemap = resource;
		mCubemap->AddRef();
		mResourceBinding->GetVariableByName(DG::SHADER_TYPE_PIXEL, "mTexture")->Set(mCubemap->GetShaderView());
	}
}