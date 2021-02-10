#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ResourceManager.hpp>

namespace Morpheus {
	void IResource::Release() {
		auto value = mRefCount.fetch_sub(1);
		if (value == 1) {
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

	CollisionShapeResource* IResource::ToCollisionShape() {
		return nullptr;
	}

	ShaderResource* IResource::ToShader() {
		return nullptr;
	}
}