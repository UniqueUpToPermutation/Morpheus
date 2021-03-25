#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Components/Transform.hpp>

namespace Morpheus {
	void EditorCameraController::OnUpdate(const ScriptUpdateEvent& e) {

		auto& input = e.mEngine->GetInputController();
		const auto& mouseState = input.GetMouseState();
		const auto& lastState = input.GetLastMouseState();

		EntityNode entity = e.mEntity;
		auto oldTransform = entity.TryGet<Transform>();
		auto& data = entity.Get<EditorCameraController::Data>();

		if (oldTransform) {
			auto viewVec = data.GetViewVector();

			auto up = DG::float3(0.0f, 1.0f, 0.0f);
			auto sideways = DG::cross(viewVec, up);
			sideways = DG::normalize(sideways);
			auto viewUp = DG::cross(viewVec, sideways);
			viewUp = DG::normalize(viewUp);

			DG::Quaternion newRotation = oldTransform->GetRotation();
			DG::float3 newTranslation = oldTransform->GetTranslation();

			if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_LEFT) {
				data.mAzimuth -= data.mMouseRotationSpeedX * (float)(mouseState.PosX - lastState.PosX);
				data.mElevation += data.mMouseRotationSpeedY * (float)(mouseState.PosY - lastState.PosY);

				newRotation = data.GetViewQuat();
			}

			if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT) {
				newTranslation -= data.mMousePanSpeedX * (float)(mouseState.PosX - lastState.PosX) * sideways;
				newTranslation -= data.mMousePanSpeedY * (float)(mouseState.PosY - lastState.PosY) * viewUp;
			}

			if (input.IsKeyDown(InputKeys::MoveForward)) {
				newTranslation += (float)(data.mKeyPanSpeedZ * e.mElapsedTime) * viewVec;
			}

			if (input.IsKeyDown(InputKeys::MoveBackward)) {
				newTranslation -= (float)(data.mKeyPanSpeedZ * e.mElapsedTime) * viewVec;
			}

			if (input.IsKeyDown(InputKeys::MoveLeft)) {
				newTranslation -= (float)(data.mKeyPanSpeedX * e.mElapsedTime) * sideways;
			}

			if (input.IsKeyDown(InputKeys::MoveRight)) {
				newTranslation += (float)(data.mKeyPanSpeedX * e.mElapsedTime) * sideways;
			}

			// Update the transform
			entity.Patch<Transform>([&](Transform& transform) {
				transform.SetTranslation(newTranslation);
				transform.SetRotation(newRotation);
			});
		}
	}

	void EditorCameraController::OnBegin(const ScriptBeginEvent& args) {
		auto entity = args.mEntity;
		entity.AddOrReplace<EditorCameraController::Data>();
	}

	void EditorCameraController::OnDestroy(const ScriptDestroyEvent& args) {
		auto entity = args.mEntity;
		entity.Remove<EditorCameraController::Data>();
	}

	DG::Quaternion EditorCameraController::Data::GetViewQuat() const {
		auto rotate_azimuth =  DG::Quaternion::RotationFromAxisAngle(
			DG::float3(0.0f, 1.0f, 0.0f), mAzimuth);
		auto rotate_elevation = DG::Quaternion::RotationFromAxisAngle(
			DG::float3(1.0f, 0.0f, 0.0f), mElevation);
		return rotate_azimuth * rotate_elevation;
	}

	DG::float3 EditorCameraController::Data::GetViewVector() const {
		return GetViewQuat().RotateVector(DG::float3(0.0f, 0.0f, 1.0f));
	}
}