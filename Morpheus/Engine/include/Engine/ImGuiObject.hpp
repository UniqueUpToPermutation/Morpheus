#pragma once 

namespace Morpheus {
	class Scene;

	class IImGuiObject {
	public:
		virtual ~IImGuiObject() {
		}

		virtual void OnCreate(Scene* scene) = 0;
		virtual void OnDestroy(Scene* scene) = 0;
		virtual void OnUpdate(Scene* scene, double currTime, double elapsedTime) = 0;
		virtual bool IsEnabled() = 0;
		virtual void SetEnabled(bool value) = 0;
	};
}