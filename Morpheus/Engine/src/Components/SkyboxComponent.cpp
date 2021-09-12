#include <Engine/Components/SkyboxComponent.hpp>
#include <Engine/Resources/FrameIO.hpp>
#include <Engine/Resources/Texture.hpp>

#include <cereal/archives/portable_binary.hpp>

using namespace entt;

namespace Morpheus {
	void SkyboxComponent::RegisterMetaData() {
		meta<SkyboxComponent>()
			.type("SkyboxComponent"_hs);

		MakeCopyableComponentType<SkyboxComponent>();
		MakeSerializableComponentType<SkyboxComponent>();
	}

	void SkyboxComponent::Serialize(
		SkyboxComponent& component, 
		cereal::PortableBinaryOutputArchive& archive,
		IDependencyResolver* dependencies) {
		
		auto textureId = dependencies->AddDependency(
			component.mCubemap.DownCast<IResource>());

		archive(textureId);
	}

	SkyboxComponent SkyboxComponent::Deserialize(
		cereal::PortableBinaryInputArchive& archive,
		const IDependencyResolver* dependencies) {

		ResourceId textureId;

		archive(textureId);

		SkyboxComponent component;
		component.mCubemap = dependencies->GetDependency(textureId).TryCast<Texture>();
		return component;
	}
}