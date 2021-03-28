#pragma once

#include <Engine/Scene.hpp>
#include <Engine/Entity.hpp>
#include <Engine/Components/ScriptComponent.hpp>

#include "BasicMath.hpp"

namespace DG = Diligent;

namespace Morpheus {
	using entt::operator""_hs;
	using entt::operator""_hws;

	struct EditorCameraControllerFirstPerson {
		struct Data {
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
		};

		static void OnUpdate(const ScriptUpdateEvent& args);
		static void OnBegin(const ScriptBeginEvent& args);
		static void OnDestroy(const ScriptDestroyEvent& args);

		static inline entt::hashed_string Name() {
			return "EditorCameraControllerFirstPerson"_hs;
		}
	};

	struct EditorCameraController2D {
		static void OnUpdate(const ScriptUpdateEvent& args);
		static void OnBegin(const ScriptBeginEvent& args);
		static void OnDestroy(const ScriptDestroyEvent& args);

		static inline entt::hashed_string Name() {
			return "EditorCameraController2D"_hs;
		}
	};
}