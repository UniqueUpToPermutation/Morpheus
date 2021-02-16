#pragma once

#include <Engine/InputController.hpp>

namespace Morpheus {
    
    class InputControllerWin32 : public InputControllerBase
    {
    public:
        InputControllerWin32();

        bool HandleNativeMessage(const void* MsgData);

        const MouseState& GetMouseState();

    private:
        void UpdateMousePos();
    };

}