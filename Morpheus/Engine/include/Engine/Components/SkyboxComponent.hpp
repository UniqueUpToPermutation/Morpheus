#pragma once

#include <Engine/Resources/TextureResource.hpp>
#include <Engine/Resources/ResourceManager.hpp>
#include <Engine/Resources/PipelineResource.hpp>

namespace DG = Diligent;

namespace Morpheus {

	class SkyboxComponent {
	private:
		DG::RefCntAutoPtr<DG::IShaderResourceBinding> mResourceBinding;
		PipelineResource* mPipeline;
		TextureResource* mCubemap;

		void LoadPipeline(ResourceManager* manager);

	public:
		inline SkyboxComponent(ResourceManager* manager) :
			mCubemap(nullptr) {
			LoadPipeline(manager);
		}

		inline SkyboxComponent(TextureResource* resource) :
			mCubemap(resource) {
			resource->AddRef();
			LoadPipeline(resource->GetManager());
		}

		inline SkyboxComponent(const SkyboxComponent& other) :
			mCubemap(other.mCubemap), 
			mResourceBinding(other.mResourceBinding), 
			mPipeline(other.mPipeline) {
			if (mCubemap)
				mCubemap->AddRef();
			mPipeline->AddRef();
		}

		inline ~SkyboxComponent() {
			if (mCubemap)
				mCubemap->Release();
			mPipeline->Release();
		}

		void SetCubemap(TextureResource* resource);

		inline TextureResource* GetCubemap() {
			return mCubemap;
		}

		inline PipelineResource* GetPipeline() {
			return mPipeline;
		}

		inline DG::IShaderResourceBinding* GetResourceBinding() {
			return mResourceBinding.RawPtr();
		}
	};
}