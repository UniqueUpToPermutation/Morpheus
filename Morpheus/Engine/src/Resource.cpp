#include <Engine/Resource.hpp>
#include <Engine/ResourceManager.hpp>

namespace Morpheus {
	void Resource::Release() {
		--mRefCount;
		if (mRefCount == 0) {
			mManager->RequestUnload(this);
		}
	}

	PipelineResource* Resource::ToPipeline() {
		return nullptr;
	}

	GeometryResource* Resource::ToGeometry() {
		return nullptr;
	}

	MaterialResource* Resource::ToMaterial() {
		return nullptr;
	}

	TextureResource* Resource::ToTexture() {
		return nullptr;
	}

	StaticMeshResource* Resource::ToStaticMesh() {
		return nullptr;
	}
}