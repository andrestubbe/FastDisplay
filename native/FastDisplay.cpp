#include "FastDisplay.h"
#include <windows.h>
#include <shellscalingapi.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "advapi32.lib")

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// ============================================================================
// FASTDISPLAY DISPLAY MONITORING API
// ============================================================================

static JavaVM* g_jvm = nullptr;
static jobject g_displayObj = nullptr;
static jmethodID g_notifyMethodId = nullptr;
static jmethodID g_notifyDPIMethodId = nullptr;
static jmethodID g_notifyInitialStateMethodId = nullptr;
static HWND g_hwnd = nullptr;
static DWORD g_threadId = 0;

static int lastWidth = 0;
static int lastHeight = 0;
static int lastDpi = 0;

static void sendInitialState() {
    if (g_jvm == nullptr || g_displayObj == nullptr) return;
    
    JNIEnv* env;
    jint attachResult = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    bool didAttach = false;
    
    if (attachResult == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread((void**)&env, nullptr) != 0) {
            return;
        }
        didAttach = true;
    } else if (attachResult != JNI_OK) {
        return;
    }
    
    // Get current display settings
    DEVMODE dm = {};
    dm.dmSize = sizeof(dm);
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    int refreshRate = 60;
    int orientation = 0;
    int dpi = 96;
    
    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        if (dm.dmFields & DM_PELSWIDTH) width = dm.dmPelsWidth;
        if (dm.dmFields & DM_PELSHEIGHT) height = dm.dmPelsHeight;
        if (dm.dmFields & DM_DISPLAYFREQUENCY) refreshRate = dm.dmDisplayFrequency;
        if (dm.dmFields & DM_DISPLAYORIENTATION) {
            switch (dm.dmDisplayOrientation) {
                case DMDO_DEFAULT: orientation = 0; break;
                case DMDO_90: orientation = 1; break;
                case DMDO_180: orientation = 2; break;
                case DMDO_270: orientation = 3; break;
            }
        }
    }
    
    // Get DPI
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    if (hMonitor) {
        UINT dpiX = 96, dpiY = 96;
        if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
            dpi = (int)dpiX;
        }
    }
    
    env->CallVoidMethod(g_displayObj, g_notifyInitialStateMethodId, 
        width, height, dpi, orientation, refreshRate);

    // Initialize tracking variables
    lastWidth = width;
    lastHeight = height;
    lastDpi = dpi;

    if (didAttach) {
        g_jvm->DetachCurrentThread();
    }
}

static LRESULT CALLBACK MonitorWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            return 0;
            
        case WM_DISPLAYCHANGE: {
            if (!g_jvm || !g_displayObj) return 0;

            JNIEnv* env;
            jint attachResult = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            bool didAttach = false;

            if (attachResult == JNI_EDETACHED) {
                if (g_jvm->AttachCurrentThread((void**)&env, nullptr) == 0) {
                    didAttach = true;
                } else {
                    return 0;
                }
            } else if (attachResult != JNI_OK) {
                return 0;
            }

            int width = GetSystemMetrics(SM_CXSCREEN);
            int height = GetSystemMetrics(SM_CYSCREEN);

            DEVMODE dm = {};
            dm.dmSize = sizeof(dm);
            int refreshRate = 60;
            int orientation = 0;
            if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
                if (dm.dmFields & DM_PELSWIDTH) width = dm.dmPelsWidth;
                if (dm.dmFields & DM_PELSHEIGHT) height = dm.dmPelsHeight;
                if (dm.dmFields & DM_DISPLAYFREQUENCY) refreshRate = dm.dmDisplayFrequency;
                if (dm.dmFields & DM_DISPLAYORIENTATION) {
                    switch (dm.dmDisplayOrientation) {
                        case DMDO_DEFAULT: orientation = 0; break;
                        case DMDO_90: orientation = 1; break;
                        case DMDO_180: orientation = 2; break;
                        case DMDO_270: orientation = 3; break;
                    }
                }
            }

            int dpi = 96;
            HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
            if (hMonitor) {
                UINT dpiX = 96, dpiY = 96;
                if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
                    dpi = (int)dpiX;
                }
            }

            if (g_notifyMethodId) {
                env->CallVoidMethod(g_displayObj, g_notifyMethodId, width, height, dpi, orientation, refreshRate);
            }

            if (g_notifyDPIMethodId && lastDpi != 0 && lastDpi != dpi) {
                int scalePercent = dpi * 100 / 96;
                env->CallVoidMethod(g_displayObj, g_notifyDPIMethodId, dpi, scalePercent);
            }
            lastDpi = dpi;

            if (didAttach) {
                g_jvm->DetachCurrentThread();
            }
            return 0;
        }

        case WM_DPICHANGED: {
            RECT* const prcNewWindow = (RECT*)lParam;
            if (prcNewWindow) {
                SetWindowPos(hwnd, NULL,
                    prcNewWindow->left, prcNewWindow->top,
                    prcNewWindow->right - prcNewWindow->left,
                    prcNewWindow->bottom - prcNewWindow->top,
                    SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            }

            if (g_jvm && g_displayObj && g_notifyDPIMethodId) {
                JNIEnv* env;
                jint attachResult = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
                bool didAttach = false;
                
                if (attachResult == JNI_EDETACHED) {
                    if (g_jvm->AttachCurrentThread((void**)&env, nullptr) == 0) {
                        didAttach = true;
                    } else {
                        return 0;
                    }
                } else if (attachResult != JNI_OK) {
                    return 0;
                }
                
                int dpi = (int)HIWORD(wParam);
                int scalePercent = dpi * 100 / 96;

                env->CallVoidMethod(g_displayObj, g_notifyDPIMethodId, dpi, scalePercent);

                if (didAttach) {
                    g_jvm->DetachCurrentThread();
                }
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

static DWORD WINAPI MonitorThread(LPVOID lpParam) {
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = MonitorWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "FastDisplayMonitor";
    
    if (!RegisterClassA(&wc)) {
        return 1;
    }
    
    g_hwnd = CreateWindowExA(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED,
        "FastDisplayMonitor",
        "FastDisplay Monitor",
        WS_VISIBLE | WS_POPUP,  // Visible popup window (no border)
        -10, -10, 1, 1,         // Off-screen, 1x1 pixel
        NULL, NULL, GetModuleHandleA(NULL), NULL
    );

    if (g_hwnd == nullptr) {
        return 0;
    }

    // Make it fully transparent and topmost so it doesn't interfere
    SetLayeredWindowAttributes(g_hwnd, RGB(0,0,0), 0, LWA_ALPHA);
    SetWindowPos(g_hwnd, HWND_TOPMOST, -10, -10, 1, 1, SWP_NOACTIVATE);

    MSG msg = { 0 };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}

// ============================================================================
// JNI EXPORTS - FASTDISPLAY MONITORING
// ============================================================================

extern "C" {

JNIEXPORT jboolean JNICALL Java_fastdisplay_FastDisplay_startMonitoring(JNIEnv* env, jobject obj) {
    if (g_hwnd != nullptr) {
        return JNI_TRUE;
    }

    env->GetJavaVM(&g_jvm);
    g_displayObj = env->NewGlobalRef(obj);
    jclass cls = env->GetObjectClass(obj);
    g_notifyMethodId = env->GetMethodID(cls, "notifyResolutionChanged", "(IIIII)V");
    g_notifyDPIMethodId = env->GetMethodID(cls, "notifyDPIChanged", "(II)V");
    g_notifyInitialStateMethodId = env->GetMethodID(cls, "notifyInitialState", "(IIIII)V");

    if (!g_notifyMethodId || !g_notifyDPIMethodId || !g_notifyInitialStateMethodId) {
         return JNI_FALSE;
    }

    HANDLE hThread = CreateThread(nullptr, 0, MonitorThread, nullptr, 0, &g_threadId);
    if (hThread) {
        CloseHandle(hThread);
        int attempts = 0;
        while (g_hwnd == nullptr && attempts < 50) {
            Sleep(10);
            attempts++;
        }
        if (g_hwnd != nullptr) {
            sendInitialState();
        }
        return JNI_TRUE;
    }

    return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_fastdisplay_FastDisplay_stopMonitoring(JNIEnv* env, jobject obj) {
    if (g_hwnd) {
        PostMessageA(g_hwnd, WM_CLOSE, 0, 0);
        g_hwnd = nullptr;
    }

    if (g_displayObj) {
        env->DeleteGlobalRef(g_displayObj);
        g_displayObj = nullptr;
    }
    g_jvm = nullptr;
}

} // extern "C"
