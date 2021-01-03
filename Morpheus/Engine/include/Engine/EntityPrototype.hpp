#pragma once

#include <functional>

#include <nlohmann/json.hpp>
#include <entt/entt.hpp>
namespace Morpheus {
	class IEntityPrototype;
	class SceneHeirarchy;
	class Engine;

	typedef std::function<IEntityPrototype*(Engine*)> prototype_factory_t;

	template <typename T>
	IEntityPrototype* DefaultPrototypeFactory(Engine* engine) {
		return new T(engine);
	}

	class EntityPrototypeManager {
	private:
		std::unordered_map<std::string, prototype_factory_t> mFactories;
		std::unordered_map<std::string, IEntityPrototype*> mPrototypes;

	public:
		inline void RegisterPrototypeFactory(const std::string& typeName, prototype_factory_t factory) {
			auto insertIt = mFactories.insert(std::pair<std::string, prototype_factory_t>(typeName, factory));

			if (insertIt.second) {
				return;
			} else {
				throw std::runtime_error(typeName + " has already been registered!");
			}
		}

		inline void RemovePrototypeFactory(const std::string& typeName) {
			mFactories.erase(typeName);
		}

		void RemovePrototype(const std::string& typeName);
		entt::entity Spawn(const std::string& typeName, Engine* en, SceneHeirarchy* scene);
		~EntityPrototypeManager();

		friend class IEntityPrototype;
	};

	class IEntityPrototype { 
	private:
		EntityPrototypeManager* mFactory;
		std::unordered_map<std::string, IEntityPrototype*>::iterator mIterator;
		uint mRefCount = 1;

	protected:
		virtual entt::entity InternalSpawn(Engine* en, SceneHeirarchy* scene) const = 0;
		virtual entt::entity InternalClone(entt::entity ent, SceneHeirarchy* scene) const = 0;
	
	public:
		virtual ~IEntityPrototype() { }
		entt::entity Spawn(Engine* en, SceneHeirarchy* scene);
		entt::entity Clone(entt::entity ent, SceneHeirarchy* scene);
	
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

		friend class EntityPrototypeManager;
	};

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

		inline EntityPrototypeComponent& operator=(EntityPrototypeComponent&& component) {
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