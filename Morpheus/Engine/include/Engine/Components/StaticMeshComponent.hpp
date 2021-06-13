#pragma once

#include <Engine/Entity.hpp>
#include <Engine/Resources/Resource.hpp>
#include <Engine/Renderer.hpp>

namespace Morpheus {
	struct StaticMeshComponent {
		Material mMaterial;
		Handle<Geometry> mGeometry;
	};
}