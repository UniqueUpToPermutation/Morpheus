#pragma once

namespace Morpheus {
	class Engine;
	class EngineApp;
	class PlatformLinux;
	class PlatformWindows;

	class IPlatform {
	public:
		virtual ~IPlatform() {
		}

		virtual int Initialize(Engine* engine, int argc, char** argv) = 0;
		virtual void Shutdown() = 0;
		virtual bool IsValid() const = 0;
		virtual void MessageLoop() = 0;
		virtual void Flush() = 0;

		virtual PlatformLinux* ToLinux() = 0;
		virtual PlatformWindows* ToWindows() = 0;
	};

	IPlatform* CreatePlatform();
}