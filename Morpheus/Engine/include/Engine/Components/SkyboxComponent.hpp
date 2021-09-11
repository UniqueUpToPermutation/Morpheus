#pragma once

#include <Engine/Resources/Texture.hpp>

namespace DG = Diligent;

namespace Morpheus {

	struct SkyboxComponent {
		Handle<Texture> mCubemap;

		SkyboxComponent() = default;

		inline SkyboxComponent(const Handle<Texture>& cubemap) : mCubemap(cubemap) {
		}

		static void BuildResourceTable(entt::registry* registry,
			FrameHeader& header,
			const SerializationSet& subset);
		static void Serialize(entt::registry* registry, 
			std::ostream& stream,
			const FrameHeader& header,
			const SerializationSet& subset);
		static void Deserialize(entt::registry* registry, 
			std::istream& stream,
			const FrameHeader& header);
	};
}