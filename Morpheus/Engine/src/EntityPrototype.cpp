#include <Engine/EntityPrototype.hpp>
#include <Engine/SceneHeirarchy.hpp>

namespace Morpheus {
	entt::entity EntityPrototypeManager::Spawn(const std::string& typeName, 
		Engine* en, SceneHeirarchy* scene) {

		auto prototypeIt = mPrototypes.find(typeName);
		IEntityPrototype* prototype = nullptr;
		if (prototypeIt != mPrototypes.end()) {
			prototype = prototypeIt->second;
		}
		else {
			auto prototypeFactoryIt = mFactories.find(typeName);
			if (prototypeFactoryIt != mFactories.end()) {
				prototype = prototypeFactoryIt->second(en);
				mPrototypes[typeName] = prototype;
			} else {
				throw std::runtime_error("Type name could not be found!");
			}
		}

		return prototype->Spawn(en, scene);
	}

	void EntityPrototypeManager::RemovePrototype(const std::string& typeName) {
		auto prototypeIt = mPrototypes.find(typeName);
		if (prototypeIt != mPrototypes.end()) {
			prototypeIt->second->mFactory = nullptr;
			mPrototypes.erase(prototypeIt);
			prototypeIt->second->Release();
		}
	}

	EntityPrototypeManager::~EntityPrototypeManager() {
		for (auto it : mPrototypes) {
			it.second->Release();
		}
	}

	entt::entity IEntityPrototype::Spawn(Engine* en, SceneHeirarchy* scene) {
		auto entity = InternalSpawn(en, scene);
		auto registry = scene->GetRegistry();
		registry->emplace<EntityPrototypeComponent>(entity, this);
		return entity;
	}

	entt::entity IEntityPrototype::Clone(entt::entity ent, SceneHeirarchy* scene) {
		auto entity = InternalClone(ent, scene);
		auto registry = scene->GetRegistry();
		registry->emplace<EntityPrototypeComponent>(entity, this);
		return entity;
	}
}