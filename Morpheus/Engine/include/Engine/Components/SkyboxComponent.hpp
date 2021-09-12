#pragma once

#include <Engine/Resources/Texture.hpp>

namespace DG = Diligent;

namespace Morpheus {

	struct SkyboxComponent {
		Handle<Texture> mCubemap;

		SkyboxComponent() = default;

		inline SkyboxComponent(const Handle<Texture>& cubemap) : mCubemap(cubemap) {
		}

		static void RegisterMetaData();

		static void Serialize(
			SkyboxComponent& transform, 
			cereal::PortableBinaryOutputArchive& archive,
			IDependencyResolver* dependencies);

		static SkyboxComponent Deserialize(
			cereal::PortableBinaryInputArchive& archive,
			const IDependencyResolver* dependencies);
	};
}