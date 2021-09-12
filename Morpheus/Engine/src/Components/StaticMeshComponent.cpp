#include <Engine/Components/StaticMeshComponent.hpp>
#include <Engine/Resources/FrameIO.hpp>

#include <cereal/archives/portable_binary.hpp>

using namespace entt;

namespace Morpheus {
	void StaticMeshComponent::RegisterMetaData() {
		meta<StaticMeshComponent>()
			.type("StaticMeshComponent"_hs);
	}
		
	void StaticMeshComponent::Serialize(
		StaticMeshComponent& component, 
		cereal::PortableBinaryOutputArchive& archive,
		IDependencyResolver* dependencies) {
		
		auto materialId = dependencies->AddDependency(
			component.mMaterial.DownCast<IResource>());
		auto geometryId = dependencies->AddDependency(
			component.mGeometry.DownCast<IResource>());
	
		archive(materialId);
		archive(geometryId);
	}

	StaticMeshComponent StaticMeshComponent::Deserialize(
		cereal::PortableBinaryInputArchive& archive,
		const IDependencyResolver* dependencies) {

		
	}
}