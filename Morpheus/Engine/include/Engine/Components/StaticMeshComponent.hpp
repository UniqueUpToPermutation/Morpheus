#pragma once

#include <Engine/Entity.hpp>
#include <Engine/Resources/Resource.hpp>

namespace Morpheus {
	struct StaticMeshComponent {
		Handle<Material> mMaterial;
		Handle<Geometry> mGeometry;

		static void RegisterMetaData();
		
		static void Serialize(
			StaticMeshComponent& component, 
			cereal::PortableBinaryOutputArchive& archive,
			IDependencyResolver* dependencies);
		static StaticMeshComponent Deserialize(
			cereal::PortableBinaryInputArchive& archive,
			const IDependencyResolver* dependencies);
	};
}