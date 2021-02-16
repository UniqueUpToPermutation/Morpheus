#pragma once

#include <functional>
namespace Morpheus {
	class Engine;
	class EngineApp;

	class PlatformLinux;
	class PlatformWin32;

	typedef std::function<void(double, double)> update_callback_t;

	struct EngineParams;

	class IPlatform {
	public:
		virtual ~IPlatform() {
		}

		virtual int Initialize(Engine* engine, 
			const EngineParams& params) = 0;
		virtual void Shutdown() = 0;
		virtual bool IsValid() const = 0;
		virtual void MessageLoop(const update_callback_t& callback) = 0;
		virtual void Flush() = 0;

		virtual PlatformLinux* ToLinux() = 0;
		virtual PlatformWin32* ToWindows() = 0;
	};

	IPlatform* CreatePlatform();
}