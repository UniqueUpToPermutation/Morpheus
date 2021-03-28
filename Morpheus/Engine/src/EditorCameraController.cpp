#include <Engine/EditorCameraController.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Components/Transform.hpp>

namespace Morpheus {
	void EditorCameraControllerFirstPerson::OnUpdate(const ScriptUpdateEvent& e) {

		auto& input = e.mEngine->GetInputController();
		const auto& mouseState = input.GetMouseState();
		const auto& lastState = input.GetLastMouseState();

		EntityNode entity = e.mEntity;
		auto oldTransform = entity.TryGet<Transform>();
		auto& data = entity.Get<EditorCameraControllerFirstPerson::Data>();

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

	void EditorCameraControllerFirstPerson::OnBegin(const ScriptBeginEvent& args) {
		auto entity = args.mEntity;
		entity.AddOrReplace<EditorCameraControllerFirstPerson::Data>();
	}

	void EditorCameraControllerFirstPerson::OnDestroy(const ScriptDestroyEvent& args) {
		auto entity = args.mEntity;
		entity.Remove<EditorCameraControllerFirstPerson::Data>();
	}

	DG::Quaternion EditorCameraControllerFirstPerson::Data::GetViewQuat() const {
		auto rotate_azimuth =  DG::Quaternion::RotationFromAxisAngle(
			DG::float3(0.0f, 1.0f, 0.0f), mAzimuth);
		auto rotate_elevation = DG::Quaternion::RotationFromAxisAngle(
			DG::float3(1.0f, 0.0f, 0.0f), mElevation);
		return rotate_azimuth * rotate_elevation;
	}

	DG::float3 EditorCameraControllerFirstPerson::Data::GetViewVector() const {
		return GetViewQuat().RotateVector(DG::float3(0.0f, 0.0f, 1.0f));
	}


	void EditorCameraController2D::OnUpdate(const ScriptUpdateEvent& args) {
		auto& input = args.mEngine->GetInputController();
		const auto& mouseState = input.GetMouseState();
		const auto& lastState = input.GetLastMouseState();

		EntityNode entity = args.mEntity;
		auto camera = entity.TryGet<Camera>();

		if (camera && entity.Has<Transform>()) {
			if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT) {
				DG::float2 diff(mouseState.PosX - lastState.PosX, mouseState.PosY - lastState.PosY);
		
				entity.Patch<Transform>([diff](Transform& transform) {
					auto translation = transform.GetTranslation();
					translation.x -= diff.x;
					translation.y += diff.y;
					transform.SetTranslation(translation);
				});
			}
		}
	}

	void EditorCameraController2D::OnBegin(const ScriptBeginEvent& args) {
	}

	void EditorCameraController2D::OnDestroy(const ScriptDestroyEvent& args) {
	}
}