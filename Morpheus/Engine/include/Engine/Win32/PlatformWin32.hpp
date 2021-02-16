#pragma once

#include <Engine/Platform.hpp>

#include "Windows.h"

#include "Timer.hpp"

namespace DG = Diligent;

namespace Morpheus {
    
	class PlatformWin32 : public IPlatform {
	private:
        HWND mWindow = 0;
        bool bQuit = false;
        Engine* mEngine = nullptr;
		DG::Timer Timer;
		double mPrevTime;

        static PlatformWin32* mGlobalInstance;

	public:
		PlatformWin32();

		int Initialize(Engine* engine, 
			const EngineParams& params) override;
		void Shutdown() override;
		bool IsValid() const override;
		void MessageLoop(const update_callback_t& callback) override;
		void Flush() override;

		PlatformLinux* ToLinux() override;
		PlatformWin32* ToWindows() override;

        static inline PlatformWin32* GetGlobalInstance() {
            return mGlobalInstance;
        }

        inline Engine* GetEngine() {
            return mEngine;
        }

		friend LRESULT CALLBACK MessageProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
}