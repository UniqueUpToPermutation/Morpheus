#pragma once

#include <Engine/Scene.hpp>

namespace Morpheus {
	struct ScriptUpdateEvent {
		EntityNode mEntity;
		double mCurrTime;
		double mElapsedTime;
		Engine* mEngine;
		Scene* mScene;
	};

	struct ScriptBeginEvent {
		EntityNode mEntity;
		Engine* mEngine;
		Scene* mScene;
	};

	struct ScriptDestroyEvent {
		EntityNode mEntity;
		Engine* mEngine;
		Scene* mScene;
	};

	typedef std::function<void(const ScriptBeginEvent&)> script_begin_t;
	typedef std::function<void(const ScriptDestroyEvent&)> script_destroy_t;
	typedef std::function<void(const ScriptUpdateEvent&)> script_update_t;

	struct ScriptInstance {
		entt::hashed_string mScriptName;
		script_begin_t mOnBegin;
		script_update_t mOnUpdate;
		script_destroy_t mOnDestroy;
	};

	class ScriptFactory {
	private:
		std::unordered_map<entt::hashed_string::hash_type, ScriptInstance> mFactoryMap;
	
	public:
		inline void AddScript(entt::hashed_string name, 
			const script_begin_t& onBegin, 
			const script_update_t& onUpdate,
			const script_destroy_t& onDestroy) {
			mFactoryMap.emplace(name.value(), ScriptInstance{name, onBegin, onUpdate, onDestroy});		
		}
		inline ScriptInstance Spawn(entt::hashed_string::hash_type name) {
			return mFactoryMap[name];
		}
		inline ScriptInstance Spawn(entt::hashed_string name) {
			return mFactoryMap[name.value()];
		}

		template <typename T>
		void AddScript() {
			AddScript(T::Name(), &T::OnBegin, &T::OnUpdate, &T::OnDestroy);
		}
	};

	class ScriptComponent {
	private:
		std::vector<ScriptInstance> mScripts;

	public:
		inline ScriptComponent& AddScript(const ScriptInstance& script) {
			mScripts.push_back(script);
			return *this;
		}

		inline ScriptComponent& AddScript(entt::hashed_string name, 
			const script_begin_t& onBegin, 
			const script_update_t& onUpdate,
			const script_destroy_t& onDestroy) {
			mScripts.emplace_back(ScriptInstance{name, onBegin, onUpdate, onDestroy});
			return *this;		
		}

		template <typename T>
		ScriptComponent& AddScript() {
			return AddScript(T::Name(), &T::OnBegin, &T::OnUpdate, &T::OnDestroy);
		}
		
		friend class ScriptSystem;
	};
}