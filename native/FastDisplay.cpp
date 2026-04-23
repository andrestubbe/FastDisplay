#include <windows.h>
#include <shellscalingapi.h>
#include <dxgi1_6.h>
#include <icm.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <jni.h>
#include <mutex>
#include <atomic>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

#define MAX_DEVICE_ID_LEN 200

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

struct MonitorInfo {
    HMONITOR handle;
    int index;
    int width;
    int height;
    int dpi;
    int orientation;
    int refreshRate;
    bool hdrEnabled;
    char colorProfile[MAX_PATH];
    char manufacturer[4];
    char modelName[128];
    char serialNumber[128];
    RECT rect;
};

static std::mutex monitorMutex;
static int monitorCount = 0;
static MonitorInfo monitors[16];  // Support up to 16 monitors

// DXGI for refresh rate change detection
static IDXGIFactory5* g_dxgiFactory = nullptr;
static DWORD g_dxgiCookie = 0;

// Initialize DXGI for refresh rate monitoring
static bool initDXGI(HWND hwnd) {
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory5), (void**)&g_dxgiFactory))) {
        return false;
    }

    // Register window for DXGI mode change events
    if (FAILED(g_dxgiFactory->RegisterOcclusionStatusWindow(hwnd, WM_USER + 100, &g_dxgiCookie))) {
        g_dxgiFactory->Release();
        g_dxgiFactory = nullptr;
        return false;
    }

    return true;
}

// Cleanup DXGI resources
static void cleanupDXGI() {
    if (g_dxgiFactory && g_dxgiCookie) {
        g_dxgiFactory->UnregisterOcclusionStatus(g_dxgiCookie);
    }
    if (g_dxgiFactory) {
        g_dxgiFactory->Release();
        g_dxgiFactory = nullptr;
    }
}

// Helper function to detect HDR support for a monitor
static bool isMonitorHDR(HMONITOR hMonitor) {
    IDXGIOutput* pOutput = nullptr;
    IDXGIAdapter* pAdapter = nullptr;
    IDXGIFactory1* pFactory = nullptr;
    bool hdrEnabled = false;

    // Create DXGI Factory
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory))) {
        return false;
    }

    // Enumerate adapters
    for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters(adapterIndex, &pAdapter)); ++adapterIndex) {
        // Enumerate outputs
        for (UINT outputIndex = 0; SUCCEEDED(pAdapter->EnumOutputs(outputIndex, &pOutput)); ++outputIndex) {
            DXGI_OUTPUT_DESC desc;
            if (SUCCEEDED(pOutput->GetDesc(&desc))) {
                if (desc.Monitor == hMonitor) {
                    // Found the matching output, check HDR support
                    IDXGIOutput6* pOutput6 = nullptr;
                    if (SUCCEEDED(pOutput->QueryInterface(__uuidof(IDXGIOutput6), (void**)&pOutput6))) {
                        DXGI_OUTPUT_DESC1 desc1;
                        if (SUCCEEDED(pOutput6->GetDesc1(&desc1))) {
                            // Check if HDR is supported by checking color space capabilities
                            // Use a simpler check - if it's not standard sRGB, assume HDR capability
                            hdrEnabled = (desc1.ColorSpace != DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
                        }
                        pOutput6->Release();
                    }
                    pOutput->Release();
                    pAdapter->Release();
                    pFactory->Release();
                    return hdrEnabled;
                }
            }
            pOutput->Release();
        }
        pAdapter->Release();
    }

    pFactory->Release();
    return false;
}

// Helper function to read color profile for a monitor
static void getMonitorColorProfile(HMONITOR hMonitor, char* colorProfile, int bufferSize) {
    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hMonitor, &mi)) {
        strcpy_s(colorProfile, bufferSize, "Unknown");
        return;
    }

    HDC hdc = CreateDCW(mi.szDevice, nullptr, nullptr, nullptr);
    if (!hdc) {
        strcpy_s(colorProfile, bufferSize, "Unknown");
        return;
    }

    DWORD profileSize = bufferSize;
    if (GetICMProfile(hdc, &profileSize, colorProfile)) {
        // Success
    } else {
        strcpy_s(colorProfile, bufferSize, "Unknown");
    }

    DeleteDC(hdc);
}

// Helper function to read EDID data for a monitor
static void getMonitorEDID(HMONITOR hMonitor, char* manufacturer, char* modelName, char* serialNumber) {
    // Initialize output strings
    manufacturer[0] = '\0';
    modelName[0] = '\0';
    serialNumber[0] = '\0';

    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hMonitor, &mi)) {
        return;
    }

    // Get device name from monitor info
    DISPLAY_DEVICEW dd = {};
    dd.cb = sizeof(dd);
    DWORD deviceIndex = 0;
    bool found = false;

    while (EnumDisplayDevicesW(nullptr, deviceIndex, &dd, 0)) {
        if (wcscmp(dd.DeviceName, mi.szDevice) == 0) {
            found = true;
            break;
        }
        deviceIndex++;
    }

    if (!found) {
        return;
    }

    // Use display device description as model name
    WideCharToMultiByte(CP_UTF8, 0, dd.DeviceString, -1, modelName, 128, nullptr, nullptr);

    // Extract manufacturer from device name (e.g., "\\.\DISPLAY1" -> "DIS")
    // This is a simplified approach; real EDID parsing would require SetupAPI
    const wchar_t* deviceName = dd.DeviceName;
    if (deviceName && wcslen(deviceName) > 4) {
        // Skip "\\.\\" and take first 3 characters
        const wchar_t* nameStart = deviceName + 4;
        manufacturer[0] = (char)nameStart[0];
        manufacturer[1] = (char)nameStart[1];
        manufacturer[2] = (char)nameStart[2];
        manufacturer[3] = '\0';
    }
}

static BOOL CALLBACK EnumMonitorsCallback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hMonitor, &mi);

    int width = lprcMonitor->right - lprcMonitor->left;
    int height = lprcMonitor->bottom - lprcMonitor->top;

    // Get DPI for this monitor
    UINT dpiX = 96, dpiY = 96;
    GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

    // Get refresh rate and orientation
    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);
    int refreshRate = 60;
    int orientation = 0;
    if (EnumDisplaySettingsW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm)) {
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

    // Check HDR support
    bool hdrEnabled = isMonitorHDR(hMonitor);

    // Get color profile
    getMonitorColorProfile(hMonitor, monitors[monitorCount].colorProfile, MAX_PATH);

    // Get EDID data
    getMonitorEDID(hMonitor, monitors[monitorCount].manufacturer, monitors[monitorCount].modelName, monitors[monitorCount].serialNumber);

    if (monitorCount < 16) {
        monitors[monitorCount].handle = hMonitor;
        monitors[monitorCount].index = monitorCount;
        monitors[monitorCount].width = width;
        monitors[monitorCount].height = height;
        monitors[monitorCount].dpi = (int)dpiX;
        monitors[monitorCount].orientation = orientation;
        monitors[monitorCount].refreshRate = refreshRate;
        monitors[monitorCount].hdrEnabled = hdrEnabled;
        monitors[monitorCount].rect = *lprcMonitor;
        monitorCount++;
    }

    return TRUE;
}

// ============================================================================
// FASTDISPLAY DISPLAY MONITORING API
// ============================================================================

static JavaVM* g_jvm = nullptr;
static jobject g_displayObj = nullptr;
static jmethodID g_notifyMethodId = nullptr;
static jmethodID g_notifyDPIMethodId = nullptr;
static jmethodID g_notifyInitialStateMethodId = nullptr;
static jmethodID g_notifyOrientationChangedMethodId = nullptr;
static jmethodID g_notifyWindowMonitorChangedMethodId = nullptr;
static jmethodID g_notifyColorProfileChangedMethodId = nullptr;
static std::atomic<HWND> g_hwnd = nullptr;  // FIX: atomic
static DWORD g_threadId = 0;
static HWND g_trackedWindow = nullptr;
static int g_lastMonitorIndex = -1;

static int lastWidth = 0;
static int lastHeight = 0;
static int lastDpi = 0;
static int lastOrientation = 0;

static int getMonitorIndexFromWindow(HWND hwnd) {
    if (hwnd == nullptr) return -1;

    RECT rect;
    GetWindowRect(hwnd, &rect);
    POINT center = { (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 };

    HMONITOR hMonitor = MonitorFromPoint(center, MONITOR_DEFAULTTONEAREST);
    if (hMonitor == nullptr) return -1;

    // FIX: Compare HMONITOR handles directly
    std::lock_guard<std::mutex> lock(monitorMutex);
    for (int i = 0; i < monitorCount; i++) {
        if (monitors[i].handle == hMonitor) {
            return i;
        }
    }

    return -1;
}

static int findMonitorIndex(HMONITOR hMonitor) {
    if (hMonitor == nullptr) return -1;

    std::lock_guard<std::mutex> lock(monitorMutex);
    for (int i = 0; i < monitorCount; i++) {
        if (monitors[i].handle == hMonitor) {
            return i;
        }
    }

    return -1;
}

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
    int width = 0, height = 0;
    DEVMODE dm = {};
    dm.dmSize = sizeof(dm);
    int refreshRate = 60;
    int orientation = 0;
    int dpi = 96;

    if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        width = dm.dmPelsWidth;
        height = dm.dmPelsHeight;
        if (dm.dmFields & DM_DISPLAYFREQUENCY) refreshRate = dm.dmDisplayFrequency;
        if (dm.dmFields & DM_DISPLAYORIENTATION) {
            switch (dm.dmDisplayOrientation) {
                case DMDO_DEFAULT: orientation = 0; break;
                case DMDO_90: orientation = 1; break;
                case DMDO_180: orientation = 2; break;
                case DMDO_270: orientation = 3; break;
            }
        }
    } else {
        // Fallback to GetSystemMetrics
        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
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
    lastOrientation = orientation;

    if (didAttach) {
        g_jvm->DetachCurrentThread();
    }
}

static LRESULT CALLBACK MonitorWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DISPLAYCHANGE: {
            // Check if monitor count changed (multi-monitor connect/disconnect)
            int oldMonitorCount = monitorCount;
            int newMonitorCount = 0;
            EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR, HDC, LPRECT, LPARAM count) -> BOOL {
                (*(int*)count)++;
                return TRUE;
            }, (LPARAM)&newMonitorCount);

            if (oldMonitorCount != newMonitorCount) {
                // Monitor count changed - could notify Java if needed
            }

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

            // Get actual resolution from EnumDisplaySettings (not GetSystemMetrics)
            int width = 0, height = 0;
            DEVMODE dm = {};
            dm.dmSize = sizeof(dm);
            int refreshRate = 60;
            int orientation = 0;
            if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
                width = dm.dmPelsWidth;
                height = dm.dmPelsHeight;
                if (dm.dmFields & DM_DISPLAYFREQUENCY) refreshRate = dm.dmDisplayFrequency;
                if (dm.dmFields & DM_DISPLAYORIENTATION) {
                    switch (dm.dmDisplayOrientation) {
                        case DMDO_DEFAULT: orientation = 0; break;
                        case DMDO_90: orientation = 1; break;
                        case DMDO_180: orientation = 2; break;
                        case DMDO_270: orientation = 3; break;
                    }
                }
            } else {
                // Fallback to GetSystemMetrics
                width = GetSystemMetrics(SM_CXSCREEN);
                height = GetSystemMetrics(SM_CYSCREEN);
            }

            // Get DPI - always query current DPI from monitor
            int dpi = 96;
            HMONITOR hMonitor = MonitorFromWindow(hwnd ? hwnd : GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
            int monitorIndex = 0;
            if (hMonitor) {
                UINT dpiX = 96, dpiY = 96;
                if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
                    dpi = (int)dpiX;
                }
                // Find monitor index
                monitorIndex = findMonitorIndex(hMonitor);
            }

            // Separate resolution and orientation events
            bool resolutionChanged = (width != lastWidth || height != lastHeight || dpi != lastDpi);
            bool orientationChanged = (orientation != lastOrientation);

            if (resolutionChanged && g_notifyMethodId) {
                env->CallVoidMethod(g_displayObj, g_notifyMethodId, monitorIndex, width, height, dpi, refreshRate);
                lastWidth = width;
                lastHeight = height;
                lastDpi = dpi;
            }

            if (orientationChanged && g_notifyOrientationChangedMethodId) {
                env->CallVoidMethod(g_displayObj, g_notifyOrientationChangedMethodId, monitorIndex, orientation);
                lastOrientation = orientation;
            }

            if (didAttach) {
                g_jvm->DetachCurrentThread();
            }
            return 0;
        }

        case WM_SETTINGCHANGE: {
            // Check if this is a color profile change
            if (lParam) {
                LPCWSTR lParamStr = (LPCWSTR)lParam;
                if (wcsstr(lParamStr, L"ICM") != nullptr ||
                    wcsstr(lParamStr, L"ColorProfile") != nullptr ||
                    wcsstr(lParamStr, L"Color") != nullptr ||
                    wcsstr(lParamStr, L"Display") != nullptr) {
                    // Color profile change detected
                }
            }
            // Check if this is a DPI change by querying current DPI
            HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            if (hMonitor) {
                UINT dpiX = 96, dpiY = 96;
                if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
                    int scalePercent = (dpiX * 100) / 96;
                    // DPI changed - could notify Java if needed
                }
            }
            return 0;
        }

        case WM_DPICHANGED: {
            int newDpi = HIWORD(wParam);
            int scalePercent = (newDpi * 100) / 96;
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

                // Get monitor index for this DPI change
                HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                int monitorIndex = 0;
                if (hMonitor) {
                    monitorIndex = findMonitorIndex(hMonitor);
                }

                env->CallVoidMethod(g_displayObj, g_notifyDPIMethodId, monitorIndex, dpi, scalePercent);

                if (didAttach) {
                    g_jvm->DetachCurrentThread();
                }
            } else {
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_USER + 100: {
            // DXGI event - refresh rate or display mode may have changed
            // Re-enumerate monitors first
            {
                std::lock_guard<std::mutex> lock(monitorMutex);
                monitorCount = 0;
                EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);
            }

            // Check all monitors for refresh rate changes
            if (g_jvm && g_displayObj) {
                JNIEnv* env;
                jint attachResult = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
                bool didAttach = false;

                if (attachResult == JNI_EDETACHED) {
                    if (g_jvm->AttachCurrentThread((void**)&env, nullptr) == 0) {
                        didAttach = true;
                    }
                } else if (attachResult != JNI_OK) {
                    return 0;
                }

                // Check each monitor for refresh rate changes using correct device name
                std::lock_guard<std::mutex> lock(monitorMutex);
                for (int i = 0; i < monitorCount; i++) {
                    MONITORINFOEXW mi = {};
                    mi.cbSize = sizeof(mi);
                    if (GetMonitorInfoW(monitors[i].handle, (LPMONITORINFO)&mi)) {
                        DEVMODEW dm = {};
                        dm.dmSize = sizeof(dm);
                        if (EnumDisplaySettingsExW(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm, 0)) {
                            if (dm.dmFields & DM_DISPLAYFREQUENCY) {
                                int newRefreshRate = dm.dmDisplayFrequency;
                                if (newRefreshRate != monitors[i].refreshRate) {
                                    // Refresh rate changed
                                    monitors[i].refreshRate = newRefreshRate;
                                    // Notify Java
                                    if (g_notifyMethodId) {
                                        env->CallVoidMethod(g_displayObj, g_notifyMethodId, i,
                                            monitors[i].width, monitors[i].height, monitors[i].dpi, newRefreshRate);
                                    }
                                }
                            }
                        }
                    }
                }

                if (didAttach) {
                    g_jvm->DetachCurrentThread();
                }
            }
            return 0;
        }
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// ============================================================================
// Window Monitor Tracking via WinEventHook (FIX: replaced WM_MOVE)
// ============================================================================

static HWINEVENTHOOK g_hook = nullptr;
static HANDLE g_registryEvent = nullptr;
static HANDLE g_registryThread = nullptr;

static void CALLBACK WinEventProc(
    HWINEVENTHOOK, DWORD, HWND hwnd, LONG, LONG, DWORD, DWORD)
{
    HWND tracked = g_trackedWindow;
    if (tracked == nullptr || hwnd != tracked)
        return;

    int newMonitorIndex = getMonitorIndexFromWindow(tracked);
    if (newMonitorIndex != -1 && newMonitorIndex != g_lastMonitorIndex) {
        int oldMonitorIndex = g_lastMonitorIndex;
        g_lastMonitorIndex = newMonitorIndex;

        int newDpi = monitors[newMonitorIndex].dpi;

        if (g_jvm && g_displayObj && g_notifyWindowMonitorChangedMethodId) {
            JNIEnv* env;
            jint attachResult = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
            bool didAttach = false;

            if (attachResult == JNI_EDETACHED) {
                if (g_jvm->AttachCurrentThread((void**)&env, nullptr) == 0) {
                    didAttach = true;
                } else {
                    return;
                }
            } else if (attachResult != JNI_OK) {
                return;
            }

            env->CallVoidMethod(g_displayObj, g_notifyWindowMonitorChangedMethodId, oldMonitorIndex, newMonitorIndex, newDpi);

            if (didAttach) {
                g_jvm->DetachCurrentThread();
            }
        }
    }
}

static DWORD WINAPI MonitorThread(LPVOID lpParam) {
    // Set DPI awareness BEFORE creating the window
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = MonitorWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "FastDisplayMonitor";

    if (!RegisterClassA(&wc)) {
        return 1;
    }

    // Find which monitor the console window is on
    HWND consoleHwnd = GetConsoleWindow();
    HMONITOR targetMonitor = nullptr;
    if (consoleHwnd) {
        RECT rect;
        GetWindowRect(consoleHwnd, &rect);
        POINT center = { (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 };
        targetMonitor = MonitorFromPoint(center, MONITOR_DEFAULTTONEAREST);
    }

    // If no console window, use primary monitor
    if (!targetMonitor) {
        targetMonitor = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    }

    // Get monitor info to position window on the correct monitor
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(targetMonitor, &mi);

    // Create a WS_POPUP window (not WS_OVERLAPPEDWINDOW) for DPI events
    // Position off-screen but keep it as a real top-level window
    HWND hwnd = CreateWindowExA(
        0,
        "FastDisplayMonitor",
        "FastDisplay Monitor",
        WS_POPUP,  // WS_POPUP receives DPI messages correctly
        mi.rcMonitor.left - 100, mi.rcMonitor.top - 100, 1, 1,  // Off-screen, minimal size
        NULL, NULL, GetModuleHandleA(NULL), NULL
    );

    if (!hwnd) {
        return 1;
    }

    g_hwnd = hwnd;

    // Initialize DXGI for refresh rate monitoring
    initDXGI(hwnd);

    MSG msg = { 0 };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup DXGI
    cleanupDXGI();

    return 0;
}

// ============================================================================
// Registry Monitoring for Color Profile Changes
// ============================================================================

static DWORD WINAPI RegistryMonitorThread(LPVOID lpParam) {
    // Monitor per-user profile associations (Microsoft-documented path)
    HKEY hKeyUser = nullptr;
    LONG resultUser = RegOpenKeyExA(HKEY_CURRENT_USER,
        "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ICM\\ProfileAssociations\\Display\\{4d36e96e-e325-11ce-bfc1-08002be10318}",
        0, KEY_NOTIFY, &hKeyUser);

    // Monitor system-wide profile associations (Microsoft-documented path)
    HKEY hKeySystem = nullptr;
    LONG resultSystem = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e96e-e325-11ce-bfc1-08002be10318}",
        0, KEY_NOTIFY, &hKeySystem);

    if (resultUser != ERROR_SUCCESS && resultSystem != ERROR_SUCCESS) {
        return 1;
    }

    // Debounce: wait for changes to settle
    DWORD lastEventTime = GetTickCount();
    while (g_registryEvent != nullptr) {
        BOOL changeDetected = FALSE;

        if (hKeyUser) {
            DWORD result = RegNotifyChangeKeyValue(hKeyUser, FALSE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, g_registryEvent, TRUE);
            if (result == ERROR_SUCCESS) {
                changeDetected = TRUE;
            }
        }

        if (hKeySystem) {
            DWORD result = RegNotifyChangeKeyValue(hKeySystem, FALSE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, g_registryEvent, TRUE);
            if (result == ERROR_SUCCESS) {
                changeDetected = TRUE;
            }
        }

        if (changeDetected) {
            DWORD currentTime = GetTickCount();
            // Debounce: only report if 1 second has passed since last event
            if (currentTime - lastEventTime > 1000) {
                lastEventTime = currentTime;

                // Re-enumerate monitors and notify Java of color profile changes
                if (g_jvm && g_displayObj && g_notifyColorProfileChangedMethodId) {
                    JNIEnv* env;
                    jint attachResult = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
                    bool didAttach = false;

                    if (attachResult == JNI_EDETACHED) {
                        if (g_jvm->AttachCurrentThread((void**)&env, nullptr) == 0) {
                            didAttach = true;
                        }
                    } else if (attachResult != JNI_OK) {
                        Sleep(1000);
                        continue;
                    }

                    // Re-enumerate monitors to get new color profiles
                    {
                        std::lock_guard<std::mutex> lock(monitorMutex);
                        monitorCount = 0;
                        EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);
                    }

                    // Notify Java for each monitor with color profile change
                    std::lock_guard<std::mutex> lock(monitorMutex);
                    for (int i = 0; i < monitorCount; i++) {
                        jstring profile = env->NewStringUTF(monitors[i].colorProfile);
                        env->CallVoidMethod(g_displayObj, g_notifyColorProfileChangedMethodId, i, profile);
                        env->DeleteLocalRef(profile);
                    }

                    if (didAttach) {
                        g_jvm->DetachCurrentThread();
                    }
                }
            }
            // Wait for changes to settle
            Sleep(1000);
        } else {
            break;
        }
    }

    if (hKeyUser) RegCloseKey(hKeyUser);
    if (hKeySystem) RegCloseKey(hKeySystem);
    return 0;
}

// ============================================================================
// JNI EXPORTS - FASTDISPLAY MONITORING
// ============================================================================

extern "C" {

// Set process DPI awareness when DLL loads
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    return JNI_VERSION_1_6;
}

JNIEXPORT jboolean JNICALL Java_fastdisplay_FastDisplay_startMonitoring(JNIEnv* env, jobject obj) {
    HWND currentHwnd = g_hwnd.load();
    if (currentHwnd != nullptr) {
        return JNI_TRUE;
    }

    env->GetJavaVM(&g_jvm);
    g_displayObj = env->NewGlobalRef(obj);
    jclass cls = env->GetObjectClass(obj);
    g_notifyMethodId = env->GetMethodID(cls, "notifyResolutionChanged", "(IIIII)V");
    g_notifyDPIMethodId = env->GetMethodID(cls, "notifyDPIChanged", "(III)V");
    g_notifyInitialStateMethodId = env->GetMethodID(cls, "notifyInitialState", "(IIIII)V");
    g_notifyOrientationChangedMethodId = env->GetMethodID(cls, "notifyOrientationChanged", "(II)V");
    g_notifyWindowMonitorChangedMethodId = env->GetMethodID(cls, "notifyWindowMonitorChanged", "(III)V");
    g_notifyColorProfileChangedMethodId = env->GetMethodID(cls, "notifyColorProfileChanged", "(ILjava/lang/String;)V");

    if (!g_notifyMethodId || !g_notifyDPIMethodId || !g_notifyInitialStateMethodId || !g_notifyOrientationChangedMethodId || !g_notifyColorProfileChangedMethodId) {
         return JNI_FALSE;
    }

    HANDLE hThread = CreateThread(nullptr, 0, MonitorThread, nullptr, 0, &g_threadId);
    if (hThread) {
        CloseHandle(hThread);
        int attempts = 0;
        while (g_hwnd.load() == nullptr && attempts < 50) {  // FIX: atomic load
            Sleep(10);
            attempts++;
        }
        if (g_hwnd.load() != nullptr) {  // FIX: atomic load
            // Set up WinEventHook for window move/location change detection
            g_hook = SetWinEventHook(
                EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE,
                nullptr, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);

            // Start registry monitoring for color profile changes
            g_registryEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            if (g_registryEvent) {
                g_registryThread = CreateThread(nullptr, 0, RegistryMonitorThread, nullptr, 0, nullptr);
            }

            sendInitialState();
        }
        return JNI_TRUE;
    }

    return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_fastdisplay_FastDisplay_stopMonitoring(JNIEnv* env, jobject obj) {
    HWND hwnd = g_hwnd.load();  // FIX: atomic load
    if (hwnd) {
        PostMessageA(hwnd, WM_CLOSE, 0, 0);
        g_hwnd.store(nullptr);  // FIX: atomic store
    }

    if (g_hook) {
        UnhookWinEvent(g_hook);
        g_hook = nullptr;
    }

    // Stop registry monitoring
    if (g_registryEvent) {
        SetEvent(g_registryEvent);
        g_registryEvent = nullptr;
    }
    if (g_registryThread) {
        WaitForSingleObject(g_registryThread, 1000);
        CloseHandle(g_registryThread);
        g_registryThread = nullptr;
    }

    if (g_displayObj) {
        env->DeleteGlobalRef(g_displayObj);
        g_displayObj = nullptr;
    }
    g_jvm = nullptr;
}

JNIEXPORT jobjectArray JNICALL Java_fastdisplay_FastDisplay_enumerateMonitors(JNIEnv* env, jobject obj) {
    std::lock_guard<std::mutex> lock(monitorMutex);
    monitorCount = 0;
    EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);

    // Find MonitorInfo class
    jclass monitorInfoClass = env->FindClass("fastdisplay/FastDisplay$MonitorInfo");
    if (monitorInfoClass == nullptr) {
        return nullptr;
    }

    // Find constructor: (IIFastDisplay$Orientation;I)V
    jclass orientationClass = env->FindClass("fastdisplay/FastDisplay$Orientation");
    if (orientationClass == nullptr) {
        return nullptr;
    }

    jmethodID constructor = env->GetMethodID(monitorInfoClass, "<init>", "(IIIILfastdisplay/FastDisplay$Orientation;IZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (constructor == nullptr) {
        return nullptr;
    }

    // Create array
    jobjectArray array = env->NewObjectArray(monitorCount, monitorInfoClass, nullptr);
    if (array == nullptr) {
        return nullptr;
    }

    // Fill array
    for (int i = 0; i < monitorCount; i++) {
        jobject orientation = env->GetStaticObjectField(orientationClass, 
            env->GetStaticFieldID(orientationClass, "LANDSCAPE", "Lfastdisplay/FastDisplay$Orientation;"));
        
        // Map orientation int to enum
        switch (monitors[i].orientation) {
            case 0: // LANDSCAPE
                orientation = env->GetStaticObjectField(orientationClass, 
                    env->GetStaticFieldID(orientationClass, "LANDSCAPE", "Lfastdisplay/FastDisplay$Orientation;"));
                break;
            case 1: // PORTRAIT
                orientation = env->GetStaticObjectField(orientationClass, 
                    env->GetStaticFieldID(orientationClass, "PORTRAIT", "Lfastdisplay/FastDisplay$Orientation;"));
                break;
            case 2: // LANDSCAPE_FLIPPED
                orientation = env->GetStaticObjectField(orientationClass, 
                    env->GetStaticFieldID(orientationClass, "LANDSCAPE_FLIPPED", "Lfastdisplay/FastDisplay$Orientation;"));
                break;
            case 3: // PORTRAIT_FLIPPED
                orientation = env->GetStaticObjectField(orientationClass, 
                    env->GetStaticFieldID(orientationClass, "PORTRAIT_FLIPPED", "Lfastdisplay/FastDisplay$Orientation;"));
                break;
        }

        jobject monitorInfo = env->NewObject(monitorInfoClass, constructor,
            monitors[i].index,
            monitors[i].width,
            monitors[i].height,
            monitors[i].dpi,
            orientation,
            monitors[i].refreshRate,
            monitors[i].hdrEnabled ? JNI_TRUE : JNI_FALSE,
            env->NewStringUTF(monitors[i].colorProfile),
            env->NewStringUTF(monitors[i].manufacturer),
            env->NewStringUTF(monitors[i].modelName),
            env->NewStringUTF(monitors[i].serialNumber));

        env->SetObjectArrayElement(array, i, monitorInfo);
    }

    return array;
}

JNIEXPORT jint JNICALL Java_fastdisplay_FastDisplay_getMonitorDPI(JNIEnv* env, jobject obj, jint monitorIndex) {
    std::lock_guard<std::mutex> lock(monitorMutex);  // FIX: thread-safe
    // FIX: replaced MAX_MONITORS with monitorCount
    if (monitorIndex < 0 || monitorIndex >= monitorCount) {
        return -1;
    }

    // If monitors array is not populated, enumerate first
    if (monitorCount == 0) {
        monitorCount = 0;
        EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);
    }

    // Check if monitor index is within actual monitor count
    if (monitorIndex >= monitorCount) {
        return -1;
    }

    return monitors[monitorIndex].dpi;
}

JNIEXPORT void JNICALL Java_fastdisplay_FastDisplay_setWindowHandleNative(JNIEnv* env, jobject obj, jlong hwnd) {
    g_trackedWindow = (HWND)hwnd;
    if (g_trackedWindow != nullptr) {
        g_lastMonitorIndex = getMonitorIndexFromWindow(g_trackedWindow);
        
        // Install WinEventHook for window monitor tracking
        if (g_hook == nullptr) {
            g_hook = SetWinEventHook(
                EVENT_OBJECT_LOCATIONCHANGE,
                EVENT_OBJECT_LOCATIONCHANGE,
                nullptr,
                WinEventProc,
                0, 0,
                WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
            );
        }
    }
}

JNIEXPORT jintArray JNICALL Java_fastdisplay_FastDisplay_getResolution(JNIEnv* env, jobject obj) {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    
    jintArray result = env->NewIntArray(2);
    jint values[2] = {width, height};
    env->SetIntArrayRegion(result, 0, 2, values);
    
    return result;
}

JNIEXPORT jint JNICALL Java_fastdisplay_FastDisplay_getScale(JNIEnv* env, jobject obj) {
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    if (hMonitor) {
        UINT dpiX = 96, dpiY = 96;
        if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
            return (int)((dpiX * 100) / 96);
        }
    }
    return 100; // Default 100%
}

JNIEXPORT jint JNICALL Java_fastdisplay_FastDisplay_getOrientation(JNIEnv* env, jobject obj) {
    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        if (dm.dmFields & DM_DISPLAYORIENTATION) {
            switch (dm.dmDisplayOrientation) {
                case DMDO_DEFAULT: return 0; // LANDSCAPE
                case DMDO_90: return 1; // PORTRAIT
                case DMDO_180: return 2; // LANDSCAPE_FLIPPED
                case DMDO_270: return 3; // PORTRAIT_FLIPPED
            }
        }
    }
    return 0; // Default LANDSCAPE
}

JNIEXPORT jint JNICALL Java_fastdisplay_FastDisplay_getCurrentMonitorIndex(JNIEnv* env, jobject obj) {
    // Ensure monitors are enumerated
    {
        std::lock_guard<std::mutex> lock(monitorMutex);
        monitorCount = 0;
        EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);
    }

    HWND consoleHwnd = GetConsoleWindow();
    if (consoleHwnd) {
        HMONITOR hMonitor = MonitorFromWindow(consoleHwnd, MONITOR_DEFAULTTONEAREST);
        if (hMonitor) {
            int monitorIndex = findMonitorIndex(hMonitor);
            if (monitorIndex >= 0) {
                return monitorIndex;
            }
        }
    }

    return 0;
}

} // extern "C"
