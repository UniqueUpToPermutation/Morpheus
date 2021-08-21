#pragma once

#include <Engine/Systems/System.hpp>
#include <Engine/Components/Transform.hpp>
#include <Engine/Camera.hpp>
#include <Engine/Platform.hpp>

namespace Morpheus {

	struct SimpleFPSCameraController {
		float mElevation = 0.0f;
		float mAzimuth = 0.0f;

		float mMouseRotationSpeedX = 0.004f;
		float mMouseRotationSpeedY = 0.004f;

		float mMousePanSpeedX = 0.01f;
		float mMousePanSpeedY = 0.01f;

		float mKeyPanSpeedZ = 10.0f;
		float mKeyPanSpeedX = 10.0f;

		DG::Quaternion GetViewQuat() const;
		DG::float3 GetViewVector() const;

		void SetFromTransform(const Transform& transform, const Camera& camera);
	};

	class SimpleFPSCameraSystem : public ISystem {
	private:
		struct Data {
			Transform mUpdatedTransform;
			entt::entity mUpdateTarget = entt::null;
		} mData;

		const InputController* mInput = nullptr;

	public:
		void Update(Frame* frame, const FrameTime& time);
		void Inject(Frame* frame);

		inline SimpleFPSCameraSystem(const InputController* input) :
			mInput(input) {
		}

		inline SimpleFPSCameraSystem(const InputController& input) :
		 	SimpleFPSCameraSystem(&input) {
		}

		std::unique_ptr<Task> Startup(SystemCollection& systems) override;
		bool IsInitialized() const override;
		void Shutdown() override;
		void NewFrame(Frame* frame) override;
		void OnAddedTo(SystemCollection& collection) override;
	};
}