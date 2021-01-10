#include <Engine/Resource.hpp>
#include <Engine/ResourceManager.hpp>

namespace Morpheus {
	void IResource::Release() {
		--mRefCount;
		if (mRefCount == 0) {
			mManager->RequestUnload(this);
		}
	}

	PipelineResource* IResource::ToPipeline() {
		return nullptr;
	}

	GeometryResource* IResource::ToGeometry() {
		return nullptr;
	}

	MaterialResource* IResource::ToMaterial() {
		return nullptr;
	}

	TextureResource* IResource::ToTexture() {
		return nullptr;
	}
}