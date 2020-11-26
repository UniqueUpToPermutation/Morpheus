#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Transform.hpp>

namespace Morpheus {
	void EditorCameraController::OnUpdate(const UpdateEvent& e) {
		mAzimuth += e.ElapsedTime;

		PushToCamera();
	}

	EditorCameraController::EditorCameraController(SceneHeirarchy* scene) :
		mScene(scene) {
		mScene->GetDispatcher()->sink<UpdateEvent>().
			connect<&EditorCameraController::OnUpdate>(this);
	}

	EditorCameraController::EditorCameraController(const EditorCameraController& other) :
		mScene(other.mScene),
		mElevation(other.mElevation),
		mAzimuth(other.mAzimuth) {
		mScene->GetDispatcher()->sink<UpdateEvent>().
			connect<&EditorCameraController::OnUpdate>(this);
	}

	EditorCameraController::~EditorCameraController() {
		mScene->GetDispatcher()->sink<UpdateEvent>().
			disconnect<&EditorCameraController::OnUpdate>(this);
	}

	void EditorCameraController::PushToCamera() {
		auto registry = mScene->GetRegistry();
		auto entity = entt::to_entity(*registry, *this);
		auto transform = registry->try_get<Transform>(entity);
		if (transform) {
			auto rotate_azimuth =  DG::Quaternion::RotationFromAxisAngle(
				DG::float3(0.0f, 1.0f, 0.0f), mAzimuth);
			auto rotate_elevation = DG::Quaternion::RotationFromAxisAngle(
				DG::float3(1.0f, 0.0f, 0.0f), mElevation);
			transform->mRotation = rotate_elevation * rotate_azimuth;
		}
	}
}