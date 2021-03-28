#pragma once

#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>

namespace DG = Diligent;

namespace Morpheus {

	class SkyboxComponent {
	private:
		DG::RefCntAutoPtr<DG::IShaderResourceBinding> mResourceBinding;
		RefHandle<PipelineResource> mPipeline;
		RefHandle<TextureResource> mCubemap;

		void LoadPipeline(ResourceManager* manager);

	public:
		inline SkyboxComponent(ResourceManager* manager) :
			mCubemap(nullptr) {
			LoadPipeline(manager);
		}

		inline SkyboxComponent(TextureResource* resource) :
			mCubemap(resource) {
			LoadPipeline(resource->GetManager());
		}

		void SetCubemap(TextureResource* resource);

		inline TextureResource* GetCubemap() {
			return mCubemap.RawPtr();
		}

		inline PipelineResource* GetPipeline() {
			return mPipeline.RawPtr();
		}

		inline DG::IShaderResourceBinding* GetResourceBinding() {
			return mResourceBinding.RawPtr();
		}
	};
}