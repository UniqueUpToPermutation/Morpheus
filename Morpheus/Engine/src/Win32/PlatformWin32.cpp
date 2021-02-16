#include <Engine/Engine.hpp>
#include <Engine/Win32/PlatformWin32.hpp>

#include "resources/Win32AppResource.h"

namespace DG = Diligent;

namespace Morpheus {

    PlatformWin32* PlatformWin32::mGlobalInstance;
        
    DG::RENDER_DEVICE_TYPE g_DeviceType = DG::RENDER_DEVICE_TYPE_UNDEFINED;

    void SetButtonImage(HWND hwndDlg, int buttonId, int imageId, BOOL Enable)
    {
        HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(imageId));
        auto    hButton = GetDlgItem(hwndDlg, buttonId);
        SendMessage(hButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
        EnableWindow(hButton, Enable);
    }

    INT_PTR CALLBACK SelectDeviceTypeDialogProc(HWND   hwndDlg,
                                                UINT   message,
                                                WPARAM wParam,
                                                LPARAM lParam)
    {
        switch (message)
        {
            case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                    case ID_DIRECT3D11:
                        g_DeviceType = Diligent::RENDER_DEVICE_TYPE_D3D11;
                        EndDialog(hwndDlg, wParam);
                        return TRUE;

                    case ID_DIRECT3D12:
                        g_DeviceType = Diligent::RENDER_DEVICE_TYPE_D3D12;
                        EndDialog(hwndDlg, wParam);
                        return TRUE;

                    case ID_OPENGL:
                        g_DeviceType = Diligent::RENDER_DEVICE_TYPE_GL;
                        EndDialog(hwndDlg, wParam);
                        return TRUE;

                    case ID_VULKAN:
                        g_DeviceType = Diligent::RENDER_DEVICE_TYPE_VULKAN;
                        EndDialog(hwndDlg, wParam);
                        return TRUE;
                }
                break;

            case WM_INITDIALOG:
            {
    #if D3D11_SUPPORTED
                BOOL D3D11Supported = TRUE;
    #else
                BOOL D3D11Supported  = FALSE;
    #endif
                SetButtonImage(hwndDlg, ID_DIRECT3D11, IDB_DIRECTX11_LOGO, D3D11Supported);

    #if D3D12_SUPPORTED
                BOOL D3D12Supported = TRUE;
    #else
                BOOL D3D12Supported  = FALSE;
    #endif
                SetButtonImage(hwndDlg, ID_DIRECT3D12, IDB_DIRECTX12_LOGO, D3D12Supported);

    #if GL_SUPPORTED
                BOOL OpenGLSupported = TRUE;
    #else
                BOOL OpenGLSupported = FALSE;
    #endif
                SetButtonImage(hwndDlg, ID_OPENGL, IDB_OPENGL_LOGO, OpenGLSupported);

    #if VULKAN_SUPPORTED
                BOOL VulkanSupported = TRUE;
    #else
                BOOL VulkanSupported = FALSE;
    #endif
                SetButtonImage(hwndDlg, ID_VULKAN, IDB_VULKAN_LOGO, VulkanSupported);
            }
            break;
        }
        return FALSE;
    }

    // Called every time the NativeNativeAppBase receives a message
    LRESULT CALLBACK MessageProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto globalInstance = PlatformWin32::GetGlobalInstance();
        auto engine = globalInstance->GetEngine();

        auto res = engine->HandleWin32Message(wnd, message, wParam, lParam);
        if (res != 0)
            return res;

        switch (message)
        {
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                BeginPaint(wnd, &ps);
                EndPaint(wnd, &ps);
                return 0;
            }
            case WM_SIZE: // Window size has been changed

                engine->WindowResize(LOWORD(lParam), HIWORD(lParam));
                return 0;

            case WM_DESTROY:
                globalInstance->bQuit = true;
                return DefWindowProc(wnd, message, wParam, lParam);

            case WM_QUIT:
                globalInstance->bQuit = true;
                return DefWindowProc(wnd, message, wParam, lParam);

            case WM_GETMINMAXINFO:
            {
                LPMINMAXINFO lpMMI      = (LPMINMAXINFO)lParam;
                lpMMI->ptMinTrackSize.x = 320;
                lpMMI->ptMinTrackSize.y = 240;
                return 0;
            }

            default:
                return DefWindowProc(wnd, message, wParam, lParam);
        }
    }

    PlatformWin32::PlatformWin32() {
        mGlobalInstance = this;
    }

    int PlatformWin32::Initialize(Engine* engine, 
        const EngineParams& params) {
        mEngine = engine;

        HINSTANCE instance = GetModuleHandle(nullptr);

#ifdef UNICODE
    const auto* const WindowClassName = L"SampleApp";
#else
    const auto* const WindowClassName = "SampleApp";
#endif

        // Register our window class
        WNDCLASSEX wcex = {sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, MessageProc,
                        0L, 0L, instance, NULL, NULL, NULL, NULL, WindowClassName, NULL};
        RegisterClassEx(&wcex);

        int DesiredWidth  = 0;
        int DesiredHeight = 0;

        mEngine->GetDesiredInitialWindowSize(DesiredWidth, DesiredHeight);

        const auto* AppTitle = mEngine->GetAppTitle();
        
        // Create a window
        LONG WindowWidth  = DesiredWidth > 0 ? DesiredWidth : 1280;
        LONG WindowHeight = DesiredHeight > 0 ? DesiredHeight : 1024;
        RECT rc           = {0, 0, WindowWidth, WindowHeight};
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        HWND wnd = CreateWindowA("SampleApp", AppTitle,
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, instance, NULL);
        if (!wnd)
        {
            std::cerr << "Failed to create a window";
            return 1;
        }

        mWindow = wnd;

        mEngine->OnWindowCreated(wnd, WindowWidth, WindowHeight);

        ShowWindow(wnd, SW_SHOW);
        UpdateWindow(wnd);

        return 1;
    }
    void PlatformWin32::Shutdown() {
        if (mWindow) {
            DestroyWindow(mWindow);
        }
    }
    bool PlatformWin32::IsValid() const {
        return mWindow != 0 && !bQuit;
    }
    void PlatformWin32::MessageLoop(const update_callback_t& callback) {
        MSG msg;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
       
        auto CurrTime    = Timer.GetElapsedTime();
        auto ElapsedTime = CurrTime - mPrevTime;
        mPrevTime         = CurrTime;

        callback(CurrTime, ElapsedTime);
    }

    void PlatformWin32::Flush() {
    }

    PlatformLinux* PlatformWin32::ToLinux() {
        return nullptr;
    }
    PlatformWin32* PlatformWin32::ToWindows() {
        return this;
    }

    IPlatform* CreatePlatform() {
        return new PlatformWin32();
    }

} // namespace