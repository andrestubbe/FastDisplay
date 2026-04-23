/**
 * @file FastDisplay.cpp
 * @brief Native Windows display monitoring implementation for Java via JNI
 * 
 * This file implements the FastDisplay native library that provides real-time
 * monitoring of Windows display changes (resolution, DPI, refresh rate, orientation).
 * 
 * Architecture:
 * - Background thread creates a hidden window to receive Windows messages
 * - WM_DISPLAYCHANGE: Resolution and refresh rate changes
 * - WM_DPICHANGED: DPI scaling changes (when supported)
 * - Polling timers: Backup mechanism for DPI and refresh rate detection
 * - JNI callbacks: Events are forwarded to Java listener
 * 
 * Thread Safety:
 * - Monitor data protected by monitorMutex
 * - Global handles are atomic
 * - JNI thread attachment/detachment handled per callback
 * 
 * @author Andre Stubbe
 * @version 1.0.0
 */

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
// DATA STRUCTURES
// ============================================================================

/**
 * @brief Monitor information structure
 * 
 * Stores all relevant display information for a single monitor.
 * Protected by monitorMutex for thread-safe access.
 */
struct MonitorInfo {
    HMONITOR handle;              ///< Windows monitor handle
    int index;                    ///< Monitor index (0-based)
    int width;                    ///< Display width in pixels
    int height;                   ///< Display height in pixels
    int dpi;                      ///< Dots per inch (96 = 100% scale)
    int orientation;              ///< Orientation (0=landscape, 1=portrait, etc.)
    int refreshRate;              ///< Refresh rate in Hz
    RECT rect;                    ///< Monitor screen rectangle
};

// ============================================================================
// GLOBAL STATE
// ============================================================================

/** Mutex protecting monitor data access */
static std::mutex monitorMutex;

/** Number of currently detected monitors */
static int monitorCount = 0;

/** Array of monitor information (max 16 monitors) */
static MonitorInfo monitors[16];

/** DPI polling timer handle */
static UINT_PTR g_dpiTimer = 0;

/** Flag indicating if DPI monitoring is active */
static std::atomic<bool> g_dpiMonitoring(false);

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static VOID CALLBACK DPICallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
static int findMonitorIndex(HMONITOR hMonitor);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================


/**
 * @brief Callback for enumerating all monitors
 * 
 * Called by EnumDisplayMonitors for each monitor in the system.
 * Collects display information including resolution, DPI, refresh rate, and orientation.
 * 
 * @param hMonitor Monitor handle
 * @param hdcMonitor Device context (unused)
 * @param lprcMonitor Monitor rectangle
 * @param dwData User data (unused)
 * @return TRUE to continue enumeration
 */
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


    if (monitorCount < 16) {
        monitors[monitorCount].handle = hMonitor;
        monitors[monitorCount].index = monitorCount;
        monitors[monitorCount].width = width;
        monitors[monitorCount].height = height;
        monitors[monitorCount].dpi = (int)dpiX;
        monitors[monitorCount].orientation = orientation;
        monitors[monitorCount].refreshRate = refreshRate;
        monitors[monitorCount].rect = *lprcMonitor;
        monitorCount++;
    }

    return TRUE;
}

// ============================================================================
// JNI GLOBAL STATE
// ============================================================================

/** Java VM instance (set during initialization) */
static JavaVM* g_jvm = nullptr;

/** Java FastDisplay object reference (global) */
static jobject g_displayObj = nullptr;

/** JNI method IDs for callbacks */
static jmethodID g_notifyMethodId = nullptr;
static jmethodID g_notifyDPIMethodId = nullptr;
static jmethodID g_notifyInitialStateMethodId = nullptr;
static jmethodID g_notifyOrientationChangedMethodId = nullptr;
static jmethodID g_notifyDebugMethodId = nullptr;

/** Hidden window handle for receiving Windows messages (atomic for thread safety) */
static std::atomic<HWND> g_hwnd = nullptr;

/** Monitor thread ID */
static DWORD g_threadId = 0;
static int lastWidth = 0;
static int lastHeight = 0;
static int lastDpi = 0;
static int lastOrientation = 0;

// Helper function to log debug messages to Java
static void logDebug(const char* message) {
    if (!g_jvm || !g_displayObj || !g_notifyDebugMethodId) return;

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

    jstring msg = env->NewStringUTF(message);
    env->CallVoidMethod(g_displayObj, g_notifyDebugMethodId, msg);
    env->DeleteLocalRef(msg);

    if (didAttach) {
        g_jvm->DetachCurrentThread();
    }
}

// ============================================================================
// JNI CALLBACK FUNCTIONS
// ============================================================================

/**
 * @brief DPI polling timer callback
 * 
 * Called every 500ms to check for DPI changes.
 * This is a backup mechanism for when WM_DPICHANGED doesn't fire.
 * Re-enumerates monitors and fires DPI events if DPI changes.
 * 
 * @param hwnd Window handle (unused)
 * @param uMsg Message ID (unused)
 * @param idEvent Timer ID (unused)
 * @param dwTime System time (unused)
 */
static VOID CALLBACK DPICallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    if (!g_jvm || !g_displayObj) return;

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

    // Save old DPI values before re-enumeration
    int oldDpiValues[16];
    {
        std::lock_guard<std::mutex> lock(monitorMutex);
        for (int i = 0; i < monitorCount; i++) {
            oldDpiValues[i] = monitors[i].dpi;
        }
    }

    // Re-enumerate monitors to get current state
    {
        std::lock_guard<std::mutex> lock(monitorMutex);
        monitorCount = 0;
        EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);
    }

    // Check each monitor for DPI changes
    std::lock_guard<std::mutex> lock(monitorMutex);
    for (int i = 0; i < monitorCount; i++) {
        UINT dpiX = 96, dpiY = 96;
        if (SUCCEEDED(GetDpiForMonitor(monitors[i].handle, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
            int newDpi = (int)dpiX;
            if (newDpi != oldDpiValues[i]) {
                // DPI changed - notify Java with DPI event
                int scalePercent = (newDpi * 100) / 96;
                if (g_notifyDPIMethodId) {
                    env->CallVoidMethod(g_displayObj, g_notifyDPIMethodId, i, newDpi, scalePercent);
                }
            }
        }
    }

    if (didAttach) {
        g_jvm->DetachCurrentThread();
    }
}


/**
 * @brief Find monitor index by HMONITOR handle
 * 
 * Searches the monitors array for a matching HMONITOR handle.
 * 
 * @param hMonitor Monitor handle to find
 * @return Monitor index, or -1 if not found
 */
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

/**
 * @brief Send initial display state to Java
 * 
 * Called when monitoring starts to provide the current display configuration.
 * Sends resolution, DPI, refresh rate, and orientation to the Java listener.
 */
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

/**
 * @brief Window procedure for the hidden monitoring window
 * 
 * Processes Windows messages related to display changes:
 * - WM_DISPLAYCHANGE: Resolution and refresh rate changes
 * - WM_DPICHANGED: DPI scaling changes
 * - WM_SETTINGCHANGE: System settings changes (color profiles)
 * 
 * @param hwnd Window handle
 * @param uMsg Message ID
 * @param wParam Message-specific parameter
 * @param lParam Message-specific parameter
 * @return Message result
 */
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

            // DPI change detection fallback (in case WM_DPICHANGED doesn't arrive) - FastTheme approach
            if (g_notifyDPIMethodId && lastDpi != 0 && lastDpi != dpi) {
                int scalePercent = dpi * 100 / 96;
                env->CallVoidMethod(g_displayObj, g_notifyDPIMethodId, dpi, scalePercent);
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

            // Fire resolution event with new DPI (combining DPI with resolution)
            if (g_jvm && g_displayObj && g_notifyMethodId) {
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

                // Get monitor index for this DPI change
                HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                int monitorIndex = 0;
                if (hMonitor) {
                    monitorIndex = findMonitorIndex(hMonitor);
                }

                // Re-enumerate monitors to get updated resolution (DPI changes often accompany resolution changes)
                {
                    std::lock_guard<std::mutex> lock(monitorMutex);
                    monitorCount = 0;
                    EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);
                    if (monitorIndex >= 0 && monitorIndex < monitorCount) {
                        int width = monitors[monitorIndex].width;
                        int height = monitors[monitorIndex].height;
                        int refreshRate = monitors[monitorIndex].refreshRate;
                        monitors[monitorIndex].dpi = newDpi;  // Ensure DPI is updated
                        // Fire resolution event with new DPI
                        env->CallVoidMethod(g_displayObj, g_notifyMethodId, monitorIndex, width, height, newDpi, refreshRate);
                    }
                }

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

/**
 * @brief Background monitoring thread
 * 
 * Creates a hidden window to receive Windows display change messages.
 * Starts DPI polling timer for DPI detection.
 * Runs the message loop until the window is destroyed.
 * 
 * @param lpParam Thread parameter (unused)
 * @return Thread exit code (0 on success, 1 on failure)
 */
static DWORD WINAPI MonitorThread(LPVOID lpParam) {
    // Set DPI awareness BEFORE creating the window (FastTheme approach)
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = MonitorWindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "FastDisplayMonitor";

    if (!RegisterClassA(&wc)) {
        return 1;
    }

    // Use normal window without WS_VISIBLE (like backup version) to receive WM_DPICHANGED
    HWND hwnd = CreateWindowExA(
        0,
        "FastDisplayMonitor",
        "FastDisplay Monitor",
        WS_OVERLAPPEDWINDOW,
        0, 0, 0, 0,
        NULL, NULL, GetModuleHandleA(NULL), NULL
    );

    if (!hwnd) {
        return 1;
    }

    g_hwnd = hwnd;

    // Start DPI polling timer (every 500ms)
    g_dpiMonitoring.store(true);
    g_dpiTimer = SetTimer(hwnd, 2, 500, DPICallback);

    MSG msg = { 0 };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup DPI timer
    if (g_dpiTimer != 0) {
        KillTimer(hwnd, g_dpiTimer);
        g_dpiTimer = 0;
    }
    g_dpiMonitoring.store(false);

    return 0;
}

// ============================================================================
// JNI EXPORTED FUNCTIONS
// ============================================================================

/**
 * @brief JNI: Start display monitoring
 * 
 * Initializes the monitoring thread and starts listening for display changes.
 * 
 * @param env JNI environment
 * @param obj FastDisplay object
 * @return true if monitoring started successfully
 */
JNIEXPORT jboolean JNICALL Java_fastdisplay_FastDisplay_startMonitoring(JNIEnv* env, jobject obj) {
    if (g_jvm == nullptr) {
        env->GetJavaVM(&g_jvm);
    }

    if (g_displayObj == nullptr) {
        g_displayObj = env->NewGlobalRef(obj);
    }

    // Get method IDs
    jclass clazz = env->GetObjectClass(obj);
    g_notifyMethodId = env->GetMethodID(clazz, "notifyResolutionChanged", "(IIIII)V");
    g_notifyDPIMethodId = env->GetMethodID(clazz, "notifyDPIChanged", "(III)V");
    g_notifyInitialStateMethodId = env->GetMethodID(clazz, "notifyInitialState", "(IIIII)V");
    g_notifyOrientationChangedMethodId = env->GetMethodID(clazz, "notifyOrientationChanged", "(II)V");
    g_notifyDebugMethodId = env->GetMethodID(clazz, "notifyDebug", "(Ljava/lang/String;)V");


    // Create monitoring thread
    HANDLE hThread = CreateThread(nullptr, 0, MonitorThread, nullptr, 0, &g_threadId);
    if (hThread == nullptr) {
        return JNI_FALSE;
    }
    CloseHandle(hThread);

    // Wait for window to be created
    Sleep(100);

    // Send initial state
    sendInitialState();

    return JNI_TRUE;
}

/**
 * @brief JNI: Stop display monitoring
 * 
 * Stops the monitoring thread and releases native resources.
 * 
 * @param env JNI environment
 * @param obj FastDisplay object
 */
JNIEXPORT void JNICALL Java_fastdisplay_FastDisplay_stopMonitoring(JNIEnv* env, jobject obj) {
    HWND hwnd = g_hwnd.load();
    if (hwnd != nullptr) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }

    if (g_displayObj != nullptr) {
        env->DeleteGlobalRef(g_displayObj);
        g_displayObj = nullptr;
    }
}

// ============================================================================
// JNI EXPORTS - UTILITY FUNCTIONS
// ============================================================================

extern "C" {

/**
 * @brief JNI: Library initialization
 * 
 * Called when the native library is loaded.
 * 
 * @param vm Java VM instance
 * @param reserved Reserved parameter
 * @return JNI version
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    return JNI_VERSION_1_6;
}

/**
 * @brief JNI: Enumerate all monitors
 * 
 * Returns an array of MonitorInfo objects containing information about all monitors.
 * 
 * @param env JNI environment
 * @param obj FastDisplay object
 * @return Array of MonitorInfo objects
 */
JNIEXPORT jobjectArray JNICALL Java_fastdisplay_FastDisplay_enumerateMonitors(JNIEnv* env, jobject obj) {
    std::lock_guard<std::mutex> lock(monitorMutex);
    monitorCount = 0;
    EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);

    jclass monitorInfoClass = env->FindClass("fastdisplay/FastDisplay$MonitorInfo");
    if (monitorInfoClass == nullptr) {
        return nullptr;
    }

    jclass orientationClass = env->FindClass("fastdisplay/FastDisplay$Orientation");
    if (orientationClass == nullptr) {
        return nullptr;
    }

    jmethodID constructor = env->GetMethodID(monitorInfoClass, "<init>", "(IIIILfastdisplay/FastDisplay$Orientation;I)V");
    if (constructor == nullptr) {
        return nullptr;
    }

    jobjectArray array = env->NewObjectArray(monitorCount, monitorInfoClass, nullptr);
    if (array == nullptr) {
        return nullptr;
    }

    for (int i = 0; i < monitorCount; i++) {
        jobject orientation;
        switch (monitors[i].orientation) {
            case 0: orientation = env->GetStaticObjectField(orientationClass, env->GetStaticFieldID(orientationClass, "LANDSCAPE", "Lfastdisplay/FastDisplay$Orientation;")); break;
            case 1: orientation = env->GetStaticObjectField(orientationClass, env->GetStaticFieldID(orientationClass, "PORTRAIT", "Lfastdisplay/FastDisplay$Orientation;")); break;
            case 2: orientation = env->GetStaticObjectField(orientationClass, env->GetStaticFieldID(orientationClass, "LANDSCAPE_FLIPPED", "Lfastdisplay/FastDisplay$Orientation;")); break;
            case 3: orientation = env->GetStaticObjectField(orientationClass, env->GetStaticFieldID(orientationClass, "PORTRAIT_FLIPPED", "Lfastdisplay/FastDisplay$Orientation;")); break;
            default: orientation = env->GetStaticObjectField(orientationClass, env->GetStaticFieldID(orientationClass, "LANDSCAPE", "Lfastdisplay/FastDisplay$Orientation;")); break;
        }

        jobject monitorInfo = env->NewObject(monitorInfoClass, constructor,
            monitors[i].index, monitors[i].width, monitors[i].height, monitors[i].dpi,
            orientation, monitors[i].refreshRate);

        env->SetObjectArrayElement(array, i, monitorInfo);
    }

    return array;
}

/**
 * @brief JNI: Get DPI for a specific monitor
 * 
 * @param env JNI environment
 * @param obj FastDisplay object
 * @param monitorIndex Monitor index
 * @return DPI value, or -1 if invalid
 */
JNIEXPORT jint JNICALL Java_fastdisplay_FastDisplay_getMonitorDPI(JNIEnv* env, jobject obj, jint monitorIndex) {
    std::lock_guard<std::mutex> lock(monitorMutex);
    if (monitorIndex < 0 || monitorIndex >= monitorCount) {
        return -1;
    }

    if (monitorCount == 0) {
        monitorCount = 0;
        EnumDisplayMonitors(nullptr, nullptr, EnumMonitorsCallback, 0);
    }

    if (monitorIndex >= monitorCount) {
        return -1;
    }

    return monitors[monitorIndex].dpi;
}

/**
 * @brief JNI: Get primary display resolution
 * 
 * @param env JNI environment
 * @param obj FastDisplay object
 * @return Array with [width, height]
 */
JNIEXPORT jintArray JNICALL Java_fastdisplay_FastDisplay_getResolution(JNIEnv* env, jobject obj) {
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);
    
    jintArray result = env->NewIntArray(2);
    jint values[2] = {width, height};
    env->SetIntArrayRegion(result, 0, 2, values);
    
    return result;
}

/**
 * @brief JNI: Get primary display scale factor
 * 
 * @param env JNI environment
 * @param obj FastDisplay object
 * @return Scale percentage (100, 125, 150, etc.)
 */
JNIEXPORT jint JNICALL Java_fastdisplay_FastDisplay_getScale(JNIEnv* env, jobject obj) {
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    if (hMonitor) {
        UINT dpiX = 96, dpiY = 96;
        if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
            return (int)((dpiX * 100) / 96);
        }
    }
    return 100;
}

/**
 * @brief JNI: Get primary display orientation
 * 
 * @param env JNI environment
 * @param obj FastDisplay object
 * @return Orientation ordinal (0=LANDSCAPE, 1=PORTRAIT, etc.)
 */
JNIEXPORT jint JNICALL Java_fastdisplay_FastDisplay_getOrientation(JNIEnv* env, jobject obj) {
    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        if (dm.dmFields & DM_DISPLAYORIENTATION) {
            switch (dm.dmDisplayOrientation) {
                case DMDO_DEFAULT: return 0;
                case DMDO_90: return 1;
                case DMDO_180: return 2;
                case DMDO_270: return 3;
            }
        }
    }
    return 0;
}

} // extern "C"
