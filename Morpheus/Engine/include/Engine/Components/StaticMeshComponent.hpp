#pragma once

#include <Engine/Entity.hpp>
#include <Engine/Resources/Resource.hpp>

namespace Morpheus {
	struct StaticMeshComponent {
		Handle<Material> mMaterial;
		Handle<Geometry> mGeometry;

		static void RegisterMetaData();
		static void BuildResourceTable(const Frame* frame, FrameHeader* header);

		static void Serialize(entt::registry* registry, 
			std::ostream& stream,
			const FrameHeader& header,
			const SerializationSet& subset);
		static void Deserialize(entt::registry* registry, 
			std::istream& stream,
			const FrameHeader& header);
	};
}