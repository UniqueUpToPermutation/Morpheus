#pragma once

#include <Engine/Components/ScriptComponent.hpp>

namespace Morpheus {
	class ScriptSystem : public ISystem {
	private:
		Engine* mEngine;

	public:
		ScriptSystem(Engine* engine) {
			mEngine = engine;
		}

		void Startup(Scene* scene) override;
		void Shutdown(Scene* scene) override;
		void OnSceneBegin(const SceneBeginEvent& args) override;
		void OnFrameBegin(const FrameBeginEvent& args) override;
		void OnSceneUpdate(const UpdateEvent& e) override;
	};
}