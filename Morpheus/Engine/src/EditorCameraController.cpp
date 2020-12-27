#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Transform.hpp>

namespace Morpheus {
	void EditorCameraController::OnUpdate(const UpdateEvent& e) {

		auto registry = e.mEngine->GetScene()->GetRegistry();
		auto entity = entt::to_entity(*registry, *this);
		auto transform = registry->try_get<Transform>(entity);

		auto& input = e.mEngine->GetInputController();
		const auto& mouseState = input.GetMouseState();
		const auto& lastState = input.GetLastMouseState();

		if (transform) {
			auto viewVec = GetViewVector();

			auto up = DG::float3(0.0f, 1.0f, 0.0f);
			auto sideways = DG::cross(viewVec, up);
			sideways = DG::normalize(sideways);
			auto viewUp = DG::cross(viewVec, sideways);
			viewUp = DG::normalize(viewUp);

			if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_LEFT) {
				mAzimuth -= mMouseRotationSpeedX * (float)(mouseState.PosX - lastState.PosX);
				mElevation += mMouseRotationSpeedY * (float)(mouseState.PosY - lastState.PosY);

				transform->mRotation = GetViewQuat();
			}

			if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT) {
				transform->mTranslation -= mMousePanSpeedX * (float)(mouseState.PosX - lastState.PosX) * sideways;
				transform->mTranslation -= mMousePanSpeedY * (float)(mouseState.PosY - lastState.PosY) * viewUp;
			}

			if (input.IsKeyDown(InputKeys::MoveForward)) {
				transform->mTranslation += (float)(mKeyPanSpeedZ * e.mElapsedTime) * viewVec;
			}

			if (input.IsKeyDown(InputKeys::MoveBackward)) {
				transform->mTranslation -= (float)(mKeyPanSpeedZ * e.mElapsedTime) * viewVec;
			}

			if (input.IsKeyDown(InputKeys::MoveLeft)) {
				transform->mTranslation -= (float)(mKeyPanSpeedX * e.mElapsedTime) * sideways;
			}

			if (input.IsKeyDown(InputKeys::MoveRight)) {
				transform->mTranslation += (float)(mKeyPanSpeedX * e.mElapsedTime) * sideways;
			}
		}
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

	DG::Quaternion EditorCameraController::GetViewQuat() const {
		auto rotate_azimuth =  DG::Quaternion::RotationFromAxisAngle(
			DG::float3(0.0f, 1.0f, 0.0f), mAzimuth);
		auto rotate_elevation = DG::Quaternion::RotationFromAxisAngle(
			DG::float3(1.0f, 0.0f, 0.0f), mElevation);
		return rotate_azimuth * rotate_elevation;
	}

	DG::float3 EditorCameraController::GetViewVector() const {
		return GetViewQuat().RotateVector(DG::float3(0.0f, 0.0f, 1.0f));
	}

}