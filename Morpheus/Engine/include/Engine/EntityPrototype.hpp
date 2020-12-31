#pragma once

#include <functional>

#include <nlohmann/json.hpp>
#include <entt/entt.hpp>

namespace Morpheus {
	class IEntityPrototype;
	class SceneHeirarchy;
	class Engine;

	typedef std::function<IEntityPrototype*(Engine*)> prototype_factory_t;

	class EntityPrototypeManager {
	private:
		std::unordered_map<std::string, prototype_factory_t> mFactories;
		std::unordered_map<std::string, IEntityPrototype*> mPrototypes;

	public:
		inline void RegisterPrototypeFactory(const std::string& typeName, prototype_factory_t factory) {
			mFactories[typeName] = factory;
		}

		inline void RegisterPrototype(const std::string& typeName, IEntityPrototype* prototypes) {
			mPrototypes[typeName] = prototypes;
		}

		inline void RemovePrototypeFactory(const std::string& typeName) {
			mFactories.erase(typeName);
		}

		inline void RemovePrototype(const std::string& typeName);
		inline entt::entity Spawn(const std::string& typeName, Engine* en, SceneHeirarchy* scene);
		inline ~EntityPrototypeManager();

		friend class IEntityPrototype;
	};

	class IEntityPrototype { 
	private:
		EntityPrototypeManager* mFactory;
		std::unordered_map<std::string, IEntityPrototype*>::iterator mIterator;
		uint mRefCount;
	
	public:
		virtual ~IEntityPrototype() { }
		virtual entt::entity Spawn(Engine* en, SceneHeirarchy* scene) const = 0;
		virtual entt::entity Clone(entt::entity ent) const = 0;
	
		inline void AddRef() {
			++mRefCount;
		}

		inline void Release() {
			--mRefCount;
			if (mRefCount == 1 && mFactory) {
				mFactory->mPrototypes.erase(mIterator);
				delete this;
			} else if (mRefCount == 0) {
				delete this;
			}
		}
	};

	entt::entity EntityPrototypeManager::Spawn(const std::string& typeName, 
		Engine* en, SceneHeirarchy* scene) {

		auto prototypeIt = mPrototypes.find(typeName);
		if (prototypeIt != mPrototypes.end()) {
			return prototypeIt->second->Spawn(en, scene);
		}
		else {
			auto prototypeFactoryIt = mFactories.find(typeName);
			if (prototypeFactoryIt != mFactories.end()) {
				auto prototype = prototypeFactoryIt->second(en);
				mPrototypes[typeName] = prototype;
				return prototype->Spawn(en, scene);
			} else {
				throw std::runtime_error("Type name could not be found!");
			}
		}
	}

	void EntityPrototypeManager::RemovePrototype(const std::string& typeName) {
		auto prototypeIt = mPrototypes.find(typeName);
		if (prototypeIt != mPrototypes.end()) {
			mPrototypes.erase(prototypeIt);
			prototypeIt->second->Release();
		}
	}

	EntityPrototypeManager::~EntityPrototypeManager() {
		for (auto it : mPrototypes) {
			it.second->Release();
		}
	}

	struct EntityPrototypeComponent {
		IEntityPrototype* mPrototype;

		inline EntityPrototypeComponent(IEntityPrototype* prototype) : mPrototype(prototype) {
			mPrototype->AddRef();
		}

		inline EntityPrototypeComponent(const EntityPrototypeComponent& component) : mPrototype(component.mPrototype) {
			mPrototype->AddRef();
		}

		inline EntityPrototypeComponent(EntityPrototypeComponent&& component) : mPrototype(component.mPrototype) {
			mPrototype->AddRef();
		}

		inline EntityPrototypeComponent& operator=(const EntityPrototypeComponent& component) {
			mPrototype->Release();
			mPrototype = component.mPrototype;
			mPrototype->AddRef();
			return *this;
		}

		inline ~EntityPrototypeComponent() {
			mPrototype->Release();
		}
	};
}