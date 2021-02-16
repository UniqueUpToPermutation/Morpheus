#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Components/Transform.hpp>

namespace Morpheus {
	void EditorCameraController::OnUpdate(const UpdateEvent& e) {

		auto& input = GetEngine()->GetInputController();
		const auto& mouseState = input.GetMouseState();
		const auto& lastState = input.GetLastMouseState();

		auto oldTransform = mCameraNode.TryGet<Transform>(); 

		if (oldTransform) {
			auto viewVec = GetViewVector();

			auto up = DG::float3(0.0f, 1.0f, 0.0f);
			auto sideways = DG::cross(viewVec, up);
			sideways = DG::normalize(sideways);
			auto viewUp = DG::cross(viewVec, sideways);
			viewUp = DG::normalize(viewUp);

			DG::Quaternion newRotation = oldTransform->GetRotation();
			DG::float3 newTranslation = oldTransform->GetTranslation();

			if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_LEFT) {
				mAzimuth -= mMouseRotationSpeedX * (float)(mouseState.PosX - lastState.PosX);
				mElevation += mMouseRotationSpeedY * (float)(mouseState.PosY - lastState.PosY);

				newRotation = GetViewQuat();
			}

			if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT) {
				newTranslation -= mMousePanSpeedX * (float)(mouseState.PosX - lastState.PosX) * sideways;
				newTranslation -= mMousePanSpeedY * (float)(mouseState.PosY - lastState.PosY) * viewUp;
			}

			if (input.IsKeyDown(InputKeys::MoveForward)) {
				newTranslation += (float)(mKeyPanSpeedZ * e.mElapsedTime) * viewVec;
			}

			if (input.IsKeyDown(InputKeys::MoveBackward)) {
				newTranslation -= (float)(mKeyPanSpeedZ * e.mElapsedTime) * viewVec;
			}

			if (input.IsKeyDown(InputKeys::MoveLeft)) {
				newTranslation -= (float)(mKeyPanSpeedX * e.mElapsedTime) * sideways;
			}

			if (input.IsKeyDown(InputKeys::MoveRight)) {
				newTranslation += (float)(mKeyPanSpeedX * e.mElapsedTime) * sideways;
			}

			// Update the transform
			mCameraNode.Patch<Transform>([&](Transform& transform) {
				transform.SetTranslation(newTranslation);
				transform.SetRotation(newRotation);
			});
		}
	}

	EditorCameraController::EditorCameraController(EntityNode cameraNode, Scene* scene) :
		mCameraNode(cameraNode),
		mScene(scene) {
		scene->GetDispatcher()->sink<UpdateEvent>().
			connect<&EditorCameraController::OnUpdate>(this);
	}

	EditorCameraController::EditorCameraController(const EditorCameraController& other) :
		mCameraNode(other.mCameraNode),
		mElevation(other.mElevation),
		mAzimuth(other.mAzimuth),
		mScene(other.mScene) {
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