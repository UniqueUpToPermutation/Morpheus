#include <Engine/Components/StaticMeshComponent.hpp>
#include <Engine/Resources/FrameIO.hpp>
#include <Engine/Resources/Texture.hpp>
#include <Engine/Resources/Geometry.hpp>
#include <Engine/Resources/Material.hpp>

#include <cereal/archives/portable_binary.hpp>

using namespace entt;

namespace Morpheus {
	void StaticMeshComponent::RegisterMetaData() {
		meta<StaticMeshComponent>()
			.type("StaticMeshComponent"_hs);

		MakeCopyableComponentType<StaticMeshComponent>();
		MakeSerializableComponentType<StaticMeshComponent>();
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

		ResourceId materialId;
		ResourceId geometryId;

		archive(materialId);
		archive(geometryId);

		StaticMeshComponent component;
		component.mMaterial = dependencies->GetDependency(materialId).TryCast<Material>();
		component.mGeometry = dependencies->GetDependency(geometryId).TryCast<Geometry>();
		return component;
	}
}