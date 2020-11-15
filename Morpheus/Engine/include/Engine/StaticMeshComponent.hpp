#pragma once

#include <Engine/StaticMeshResource.hpp>
#include <Engine/MaterialResource.hpp>
#include <Engine/PipelineResource.hpp>

namespace Morpheus {
	class StaticMeshComponent {
	private:
		StaticMeshResource* mResource;

	public:
		inline StaticMeshComponent(StaticMeshResource* resource) :
			mResource(resource) {
			resource->AddRef();
		}

		inline StaticMeshComponent(const StaticMeshComponent& component) :
			mResource(component.mResource) {
			mResource->AddRef();
		}

		inline StaticMeshComponent() :
			mResource(nullptr) {
		}

		inline ~StaticMeshComponent() {
			if (mResource)
				mResource->Release();
		}

		inline void SetMesh(StaticMeshResource* resource) {
			if (mResource)
				mResource->Release();
			mResource = resource;
			resource->AddRef();
		}

		inline StaticMeshResource* GetMesh() {
			return mResource;
		}

		inline PipelineResource* GetPipeline() {
			return mResource->GetMaterial()->GetPipeline();
		}

		inline MaterialResource* GetMaterial() {
			return mResource->GetMaterial();
		}
	};
}