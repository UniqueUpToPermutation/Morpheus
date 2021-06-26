#include <Engine/Systems/SimpleFPSCameraSystem.hpp>

namespace Morpheus {
	void SimpleFPSCameraController::SetFromTransform(const Transform& transform, const Camera& camera) {

		DG::float4x4 transformMat = transform.ToMatrix();
		DG::float4x4 transformInv = transformMat.Inverse();

		DG::float4x4 view = camera.GetView();

		DG::float4x4 m = transformInv * view;

		float thetaX = atan2(m.m21, m.m22);
		float thetaY = atan2(-m.m02, sqrt(m.m12 * m.m12 + m.m22 * m.m22));
		float thetaZ = atan2(m.m01, m.m00);

		mAzimuth = thetaY;
		mElevation = thetaX;
	}

	DG::Quaternion SimpleFPSCameraController::GetViewQuat() const {
		auto rotate_azimuth =  DG::Quaternion::RotationFromAxisAngle(
			DG::float3(0.0f, 1.0f, 0.0f), mAzimuth);
		auto rotate_elevation = DG::Quaternion::RotationFromAxisAngle(
			DG::float3(1.0f, 0.0f, 0.0f), mElevation);
		return rotate_azimuth * rotate_elevation;
	}

	DG::float3 SimpleFPSCameraController::GetViewVector() const {
		return GetViewQuat().RotateVector(DG::float3(0.0f, 0.0f, 1.0f));
	}

	void SimpleFPSCameraSystem::Inject(Frame* frame) {
		// Actually update the transform on this injection step
		if (mData.mUpdateTarget != entt::null) {
			frame->Replace<Transform>(mData.mUpdateTarget, mData.mUpdatedTransform);
		}
	}

	void SimpleFPSCameraSystem::Update(Frame* frame, const FrameTime& time) {
		auto cameraEnt = frame->mCamera;

		if (cameraEnt == entt::null) {
			mData.mUpdateTarget = entt::null;
			return;
		}

		auto& camera = frame->Get<Camera>(cameraEnt);
		auto* transform = frame->TryGet<Transform>(cameraEnt);
		auto* fpsController = frame->TryGet<SimpleFPSCameraController>(cameraEnt);

		if (fpsController == nullptr)
			return;

		if (transform == nullptr) {
			std::cout << "WARNING: Entity has SimpleFPSCameraController but no Transform!" << std::endl;
			return;
		}

		if (mData.mUpdateTarget != cameraEnt)
			fpsController->SetFromTransform(*transform, camera);

		mData.mUpdateTarget = cameraEnt;

		auto viewVec = fpsController->GetViewVector();

		auto up = DG::float3(0.0f, 1.0f, 0.0f);
		auto sideways = DG::cross(viewVec, up);
		sideways = DG::normalize(sideways);
		auto viewUp = DG::cross(viewVec, sideways);
		viewUp = DG::normalize(viewUp);

		DG::Quaternion newRotation = transform->GetRotation();
		DG::float3 newTranslation = transform->GetTranslation();

		const auto& input = *mInput;
		const auto& mouseState = input.GetMouseState();
		const auto& lastState = input.GetLastMouseState();

		if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_LEFT) {
			fpsController->mAzimuth += fpsController->mMouseRotationSpeedX * (float)(mouseState.PosX - lastState.PosX);
			fpsController->mElevation += fpsController->mMouseRotationSpeedY * (float)(mouseState.PosY - lastState.PosY);

			newRotation = fpsController->GetViewQuat();
		}

		if (mouseState.ButtonFlags & MouseState::BUTTON_FLAG_RIGHT) {
			newTranslation += fpsController->mMousePanSpeedX * (float)(mouseState.PosX - lastState.PosX) * sideways;
			newTranslation -= fpsController->mMousePanSpeedY * (float)(mouseState.PosY - lastState.PosY) * viewUp;
		}

		if (input.IsKeyDown(InputKeys::MoveForward)) {
			newTranslation += (float)(fpsController->mKeyPanSpeedZ * time.mElapsedTime) * viewVec;
		}

		if (input.IsKeyDown(InputKeys::MoveBackward)) {
			newTranslation -= (float)(fpsController->mKeyPanSpeedZ * time.mElapsedTime) * viewVec;
		}

		if (input.IsKeyDown(InputKeys::MoveLeft)) {
			newTranslation += (float)(fpsController->mKeyPanSpeedX * time.mElapsedTime) * sideways;
		}

		if (input.IsKeyDown(InputKeys::MoveRight)) {
			newTranslation -= (float)(fpsController->mKeyPanSpeedX * time.mElapsedTime) * sideways;
		}

		mData.mUpdatedTransform = Transform()
			.SetTranslation(newTranslation)
			.SetRotation(newRotation);
	}

	Task SimpleFPSCameraSystem::Startup(SystemCollection& systems) {

		auto& processor = systems.GetFrameProcessor();

		// This task updates simple fps camera controllers
		processor.AddUpdateTask(ParameterizedTask<UpdateParams>(
			[this](const TaskParams& e, const UpdateParams& params) {
				Update(params.mFrame, params.mTime);
			}, "Update FPS Camera", TaskType::UPDATE
		));

		// This injects the changes to camera transform into the frame
		InjectProc injectCameraTransform;
		injectCameraTransform.mTarget = entt::type_id<Transform>();
		injectCameraTransform.mProc = [this](Frame* frame) {
			Inject(frame);
		};
		processor.AddInjector(injectCameraTransform);

		return Task();
	}

	bool SimpleFPSCameraSystem::IsInitialized() const {
		return true;
	}

	void SimpleFPSCameraSystem::Shutdown() {

	}

	void SimpleFPSCameraSystem::NewFrame(Frame* frame) {
		mData.mUpdateTarget = entt::null;
	}

	void SimpleFPSCameraSystem::OnAddedTo(SystemCollection& collection) {

	}
}