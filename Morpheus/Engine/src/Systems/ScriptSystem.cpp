#include <Engine/Systems/ScriptSystem.hpp>
#include <Engine/Components/ScriptComponent.hpp>
#include <Engine/Scene.hpp>

namespace Morpheus {
	void ScriptSystem::Startup(Scene* scene) {
	}

	void ScriptSystem::Shutdown(Scene* scene) {
		auto registry = scene->GetRegistry();
		auto view = registry->view<ScriptComponent>();

		ScriptDestroyEvent ev;
		ev.mEngine = mEngine;
		ev.mScene = scene;

		for (auto scriptEntity : view) {
			auto& scriptComponent = view.get<ScriptComponent>(scriptEntity);

			ev.mEntity = EntityNode(registry, scriptEntity);

			for (auto& script : scriptComponent.mScripts) {
				script.mOnDestroy(ev);
			}
		}
	}

	void ScriptSystem::OnSceneBegin(const SceneBeginEvent& args) {
		auto registry = args.mSender->GetRegistry();
		auto view = registry->view<ScriptComponent>();

		ScriptBeginEvent ev;
		ev.mEngine = mEngine;
		ev.mScene = args.mSender;

		for (auto scriptEntity : view) {
			auto& scriptComponent = view.get<ScriptComponent>(scriptEntity);

			ev.mEntity = EntityNode(registry, scriptEntity);

			for (auto& script : scriptComponent.mScripts) {
				script.mOnBegin(ev);
			}
		}
	}

	void ScriptSystem::OnFrameBegin(const FrameBeginEvent& args) {
	}

	void ScriptSystem::OnSceneUpdate(const UpdateEvent& args) {
		auto registry = args.mSender->GetRegistry();
		auto view = registry->view<ScriptComponent>();

		ScriptUpdateEvent ev;
		ev.mEngine = mEngine;
		ev.mScene = args.mSender;
		ev.mCurrTime = args.mCurrTime;
		ev.mElapsedTime = args.mElapsedTime;

		for (auto scriptEntity : view) {
			auto& scriptComponent = view.get<ScriptComponent>(scriptEntity);
			ev.mEntity = EntityNode(registry, scriptEntity);

			for (auto& script : scriptComponent.mScripts) {
				script.mOnUpdate(ev);
			}
		}
	}
}