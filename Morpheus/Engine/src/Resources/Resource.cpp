#include <Engine/Resources/Resource.hpp>
#include <Engine/Resources/ResourceManager.hpp>

#include <fstream>

namespace Morpheus {

	void ReadBinaryFile(const std::string& source, std::vector<uint8_t>& out) {
		auto stream = std::ifstream(source, std::ios::binary | std::ios::ate);
	
		size_t data_size = 0;

		if (stream.is_open()) {
			std::streamsize size = stream.tellg();
			data_size = (size_t)(size + 1);
			stream.seekg(0, std::ios::beg);

			out.resize(data_size);
			if (!stream.read((char*)&out[0], size))
			{
				std::runtime_error("Could not read file");
			}

			stream.close();

			out[size] = 0;
		}
	}

	void IResource::Release() {
		auto value = mRefCount.fetch_sub(1);
		assert(value >= 1);
		if (value == 1) {
			mManager->RequestUnload(this);
		}
	}

	PipelineResource* IResource::ToPipeline() {
		return nullptr;
	}

	GeometryResource* IResource::ToGeometry() {
		return nullptr;
	}

	MaterialResource* IResource::ToMaterial() {
		return nullptr;
	}

	TextureResource* IResource::ToTexture() {
		return nullptr;
	}

	CollisionShapeResource* IResource::ToCollisionShape() {
		return nullptr;
	}

	ShaderResource* IResource::ToShader() {
		return nullptr;
	}
}