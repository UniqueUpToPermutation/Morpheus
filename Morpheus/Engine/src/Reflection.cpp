#include <Engine/Reflection.hpp>
#include <Engine/Systems/System.hpp>
#include <Engine/Resources/FrameIO.hpp>
#include <Engine/Resources/Texture.hpp>
#include <Engine/Resources/Geometry.hpp>
#include <Engine/Resources/Material.hpp>
#include <Engine/Components/Camera.hpp>
#include <Engine/Components/SkyboxComponent.hpp>
#include <Engine/Components/StaticMeshComponent.hpp>
#include <Engine/Components/Transform.hpp>

namespace Morpheus {

	// Copyable interface
	std::unordered_map<entt::meta_type,
		CopyableType, 
		MetaTypeHasher> gCopyableTypes;

	CopyableType GetCopyableType(const entt::meta_type& type) {
		auto it = gCopyableTypes.find(type);

		if (it != gCopyableTypes.end()) {
			return it->second;
		} else {
			return nullptr;
		}
	}

	void ForEachCopyableType(std::function<void(const CopyableType)> func) {
		for (auto& type : gCopyableTypes) {
			func(type.second);
		}
	}

	CopyableType AddCopyableType(CopyableType type) {
		auto it = gCopyableTypes.find(type->GetType());

		if (it != gCopyableTypes.end()) {
			return it->second;
		} else {
			gCopyableTypes.emplace_hint(it, type->GetType(), type);
			return type;
		}
	}

	// Serializable interface
	std::unordered_map<entt::meta_type,
		SerializableType,
		MetaTypeHasher> gSerializableTypes;

	SerializableType GetSerializableType(const entt::meta_type& type) {
		auto it = gSerializableTypes.find(type);

		if (it != gSerializableTypes.end()) {
			return it->second;
		} else {
			return nullptr;
		}
	}

	void ForEachSerializableType(std::function<void(const SerializableType)> func) {
		for (auto& type : gSerializableTypes) {
			func(type.second);
		}
	}

	SerializableType AddSerializableTypes(SerializableType type) {
		auto it = gSerializableTypes.find(type->GetType());

		if (it != gSerializableTypes.end()) {
			return it->second;
		} else {
			gSerializableTypes.emplace_hint(it, type->GetType(), type);
			return type;
		}
	}

	void Meta::Initialize() {
		Texture::RegisterMetaData();
		Geometry::RegisterMetaData();
		Material::RegisterMetaData();
		Frame::RegisterMetaData();
		Camera::RegisterMetaData();
		StaticMeshComponent::RegisterMetaData();
		Transform::RegisterMetaData();
		SkyboxComponent::RegisterMetaData();
	}
}