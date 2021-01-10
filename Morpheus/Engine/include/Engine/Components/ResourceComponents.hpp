#pragma once

#include <Engine/Resources/Resource.hpp>
#include <Engine/Components/RefCountComponent.hpp>

namespace Morpheus {
	typedef RefCountComponent<TextureResource> TextureComponent;
	typedef RefCountComponent<PipelineResource> PipelineComponent;
	typedef RefCountComponent<MaterialResource> MaterialComponent;
	typedef RefCountComponent<GeometryResource> GeometryComponent;
}