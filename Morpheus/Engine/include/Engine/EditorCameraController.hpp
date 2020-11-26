#pragma once

#include <Engine/SceneHeirarchy.hpp>

#include <entt/entt.hpp>

namespace Morpheus {
	class EditorCameraController {
	private:
		SceneHeirarchy* mScene;
		float mElevation = 	0.0f;
		float mAzimuth = 	0.0f;

		void PushToCamera();

	public:
		void OnUpdate(const UpdateEvent& e);

		EditorCameraController(SceneHeirarchy* scene);
		EditorCameraController(const EditorCameraController& other);
		~EditorCameraController();
	};
}