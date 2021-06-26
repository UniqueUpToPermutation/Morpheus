#pragma once

#include <Engine/Entity.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/Systems/Renderer.hpp>

namespace Morpheus {
	struct StaticMeshComponent {
		Material mMaterial;
		Handle<Geometry> mGeometry;
	};
}