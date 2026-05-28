#include <windows.h>
#include <objbase.h>
#include <shobjidl_core.h>
#include <vector>
#include <string>
#include <jni.h>
#include <iostream>

// Forward declarations
struct IApplicationView;

// VirtualDesktopManagerInternal
static const CLSID CLSID_VirtualDesktopManagerInternal =
{ 0xc5e0cdca, 0x7b6e, 0x41b2, { 0x9f, 0xc4, 0xd9, 0x3f, 0x3e, 0x4f, 0x67, 0x0f } };

// VirtualDesktopNotificationService
static const CLSID CLSID_VirtualDesktopNotificationService =
{ 0xa501fdec, 0x4a09, 0x464c, { 0xae, 0x4e, 0x1b, 0x9c, 0x21, 0xb0, 0x5e, 0x4a } };

struct __declspec(uuid("A5CD92FF-29BE-454C-8D04-D82879FB3F1B"))
Fast_IVirtualDesktopManager : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE IsWindowOnCurrentVirtualDesktop(HWND topLevelWindow, BOOL* onCurrentDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWindowDesktopId(HWND topLevelWindow, GUID* desktopId) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveWindowToDesktop(HWND topLevelWindow, REFGUID desktopId) = 0;
};

struct __declspec(uuid("FF72FFDD-BE7E-43FC-9C03-AD81681E88E4"))
Fast_IVirtualDesktop : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE IsViewVisible(IApplicationView* pView, int* pfVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetID(GUID* pGuid) = 0;
};

struct __declspec(uuid("F31574D6-B682-4CDC-BD56-1827860ABEC6"))
Fast_IVirtualDesktopManagerInternal : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE GetCount(HMONITOR hMonitor, UINT* pCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(IApplicationView* pView, Fast_IVirtualDesktop* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(IApplicationView* pView, int* pfCanViewMoveDesktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(HMONITOR hMonitor, Fast_IVirtualDesktop** ppDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktops(HMONITOR hMonitor, IObjectArray** ppDesktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdjacentDesktop(Fast_IVirtualDesktop* pDesktop, int direction, Fast_IVirtualDesktop** ppAdjacentDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(HMONITOR hMonitor, Fast_IVirtualDesktop* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDesktopW(HMONITOR hMonitor, Fast_IVirtualDesktop** ppNewDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveDesktop(Fast_IVirtualDesktop* pRemove, Fast_IVirtualDesktop* pFallback) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindDesktop(GUID* desktopId, Fast_IVirtualDesktop** ppDesktop) = 0;
};

struct __declspec(uuid("C179334C-4295-40D3-BEA1-C654D965605A"))
Fast_IVirtualDesktopNotification : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopCreated(Fast_IVirtualDesktop* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyBegin(Fast_IVirtualDesktop* pDesktopDestroyed, Fast_IVirtualDesktop* pDesktopFallback) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyFailed(Fast_IVirtualDesktop* pDesktopDestroyed, Fast_IVirtualDesktop* pDesktopFallback) = 0;
    virtual HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyed(Fast_IVirtualDesktop* pDesktopDestroyed, Fast_IVirtualDesktop* pDesktopFallback) = 0;
    virtual HRESULT STDMETHODCALLTYPE ViewVirtualDesktopChanged(IApplicationView* pView) = 0;
    virtual HRESULT STDMETHODCALLTYPE CurrentVirtualDesktopChanged(Fast_IVirtualDesktop* pOldDesktop, Fast_IVirtualDesktop* pNewDesktop) = 0;
};

struct __declspec(uuid("0CD45E71-D927-4F15-8B0A-8FEF525337BF"))
Fast_IVirtualDesktopNotificationService : public IUnknown {
public:
    virtual HRESULT STDMETHODCALLTYPE Register(Fast_IVirtualDesktopNotification* pNotification, DWORD* pdwCookie) = 0;
    virtual HRESULT STDMETHODCALLTYPE Unregister(DWORD dwCookie) = 0;
};

// Fast_IVirtualDesktopManager is already defined in shobjidl_core.h

// Utilities for GUID <-> String
static std::string GuidToString(const GUID& guid) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1],
             guid.Data4[2], guid.Data4[3], guid.Data4[4],
             guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::string(buf);
}

static GUID StringToGuid(const std::string& str) {
    GUID guid = {};
    sscanf_s(str.c_str(), "%08lx-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
             &guid.Data1, &guid.Data2, &guid.Data3,
             &guid.Data4[0], &guid.Data4[1],
             &guid.Data4[2], &guid.Data4[3], &guid.Data4[4],
             &guid.Data4[5], &guid.Data4[6], &guid.Data4[7]);
    return guid;
}

// Global to track if we are using the Win11 COM layout
static bool g_isWin11COM = false;

static HRESULT GetInternalManager(Fast_IVirtualDesktopManagerInternal** ppManager) {
    *ppManager = nullptr;
    IServiceProvider* pServiceProvider = nullptr;
    const CLSID CLSID_ImmersiveShell = {0xc2f03a33, 0x21f5, 0x47fa, {0xb4, 0xbb, 0x15, 0x63, 0x62, 0xa2, 0xf2, 0x39}};
    
    HRESULT hr = CoCreateInstance(CLSID_ImmersiveShell, nullptr, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (void**)&pServiceProvider);
    if (FAILED(hr)) {
        return hr;
    }

    IUnknown* pUnk = nullptr;
    hr = pServiceProvider->QueryService(CLSID_VirtualDesktopManagerInternal, __uuidof(IUnknown), (void**)&pUnk);
    pServiceProvider->Release();
    if (FAILED(hr)) {
        return hr;
    }

    const GUID iids[] = {
        {0x536D3495, 0xB208, 0x4CC9, {0xAE, 0x26, 0xDE, 0x81, 0x11, 0x27, 0x5B, 0xF8}}, // 23H2
        {0xA3175F2D, 0x239C, 0x4BD2, {0x8A, 0xA0, 0xEE, 0xBA, 0x8B, 0x0B, 0x13, 0x8E}}, // 22H2
        {0xB2F925B9, 0x5A0F, 0x4D2E, {0x9F, 0x4D, 0x2B, 0x15, 0x07, 0x59, 0x3C, 0x10}}, // 21H2
        {0x3F07F4BE, 0xB107, 0x441A, {0xAF, 0x0E, 0x39, 0x5C, 0x28, 0x68, 0xA4, 0x84}}, // 24H2
        {0xF31574D6, 0xB682, 0x4CDC, {0xBD, 0x56, 0x18, 0x27, 0x86, 0x0A, 0xBE, 0xC6}}  // Win10
    };

    HRESULT qihr = E_NOINTERFACE;
    for (int i = 0; i < 5; ++i) {
        qihr = pUnk->QueryInterface(iids[i], (void**)ppManager);
        if (SUCCEEDED(qihr)) {
            g_isWin11COM = (i < 4); // If not the last one (Win10), it's Win11
            break;
        }
    }
    pUnk->Release();
    return *ppManager ? S_OK : qihr;
}

static HRESULT GetDesktopID(Fast_IVirtualDesktop* pDesktop, GUID* pGuid) {
    if (!pDesktop) return E_POINTER;
    typedef HRESULT(__stdcall* GetIDFn)(void*, GUID*);
    void*** vtable = (void***)pDesktop;
    // Win11: GetID is index 3. Win10: GetID is index 4.
    int index = g_isWin11COM ? 3 : 4;
    GetIDFn fn = (GetIDFn)(*vtable)[index];
    return fn(pDesktop, pGuid);
}

static std::wstring GetDesktopName(const GUID& id) {
    HKEY k;
    std::wstring name = L"";
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops\\Desktops", 0, KEY_READ, &k) != ERROR_SUCCESS) {
        return L"";
    }

    DWORD i = 0;
    WCHAR val[256];
    BYTE data[256];
    DWORD valSize, dataSize, type;
    while (true) {
        valSize = 256;
        dataSize = 256;
        if (RegEnumValueW(k, i, val, &valSize, nullptr, &type, data, &dataSize) != ERROR_SUCCESS) break;

        char buf[64];
        snprintf(buf, sizeof(buf), "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                 id.Data1, id.Data2, id.Data3,
                 id.Data4[0], id.Data4[1], id.Data4[2], id.Data4[3],
                 id.Data4[4], id.Data4[5], id.Data4[6], id.Data4[7]);
        
        std::wstring target;
        for (char c : std::string(buf)) target.push_back((WCHAR)c);

        if (std::wstring(val) == target) {
            name = std::wstring((WCHAR*)data, dataSize / 2);
            // remove null terminator if present
            if (!name.empty() && name.back() == L'\0') {
                name.pop_back();
            }
            break;
        }
        i++;
    }
    RegCloseKey(k);
    return name;
}

// ----------------------------------------------------------------------------
// JNI Implementations
// ----------------------------------------------------------------------------

extern "C" JNIEXPORT jobjectArray JNICALL
Java_fastdisplay_FastDesktop_enumerateDesktops(JNIEnv* env, jobject obj) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    Fast_IVirtualDesktopManagerInternal* pManager = nullptr;
    HRESULT hrMgr = GetInternalManager(&pManager);
    if (FAILED(hrMgr) || !pManager) return nullptr;

    IObjectArray* pDesktops = nullptr;
    if (FAILED(pManager->GetDesktops(nullptr, &pDesktops))) {
        pManager->Release();
        return nullptr;
    }

    UINT count = 0;
    pDesktops->GetCount(&count);

    HMONITOR primary = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    Fast_IVirtualDesktop* currentDesktop = nullptr;
    pManager->GetCurrentDesktop(primary, &currentDesktop);
    GUID currentGuid = {};
    if (currentDesktop) {
        GetDesktopID(currentDesktop, &currentGuid);
    }

    jclass infoClass = env->FindClass("fastdisplay/FastDesktop$DesktopInfo");
    jmethodID ctor = env->GetMethodID(infoClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Z)V");

    jobjectArray result = env->NewObjectArray(count, infoClass, nullptr);

    for (UINT i = 0; i < count; ++i) {
        Fast_IVirtualDesktop* pDesktop = nullptr;
        if (SUCCEEDED(pDesktops->GetAt(i, __uuidof(Fast_IVirtualDesktop), (void**)&pDesktop))) {
            GUID id = {};
            GetDesktopID(pDesktop, &id);
            
            bool isCurrent = (currentDesktop && IsEqualGUID(id, currentGuid));
            std::string idStr = GuidToString(id);
            
            std::wstring wname = GetDesktopName(id);
            std::string nameStr;
            if (wname.empty()) {
                nameStr = "Desktop " + std::to_string(i + 1);
            } else {
                nameStr = std::string(wname.begin(), wname.end());
            }

            jstring jId = env->NewStringUTF(idStr.c_str());
            jstring jName = env->NewStringUTF(nameStr.c_str());
            
            jobject infoObj = env->NewObject(infoClass, ctor, jId, jName, isCurrent);
            env->SetObjectArrayElement(result, i, infoObj);

            pDesktop->Release();
        }
    }

    if (currentDesktop) currentDesktop->Release();
    pDesktops->Release();
    pManager->Release();
    
    return result;
}

extern "C" JNIEXPORT jstring JNICALL
Java_fastdisplay_FastDesktop_getCurrentDesktopId(JNIEnv* env, jobject obj) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    Fast_IVirtualDesktopManager* pManager = nullptr;
    const CLSID CLSID_VirtualDesktopManager = { 0xaa509086, 0x5ca9, 0x4c25, { 0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a } };
    HRESULT hr = CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL, __uuidof(Fast_IVirtualDesktopManager), (void**)&pManager);
    if (FAILED(hr) || !pManager) {
        char buf[128];
        snprintf(buf, sizeof(buf), "ERROR: CoCreateInstance VirtualDesktopManager failed with HR 0x%08X", hr);
        return env->NewStringUTF(buf);
    }

    HWND hwnd = GetForegroundWindow();
    bool created = false;
    if (!hwnd) {
        hwnd = CreateWindowW(L"STATIC", L"", 0, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
        ShowWindow(hwnd, SW_SHOWNA);
        created = true;
    }

    GUID id = {};
    hr = pManager->GetWindowDesktopId(hwnd, &id);
    
    if (created) {
        DestroyWindow(hwnd);
    }
    pManager->Release();

    if (FAILED(hr)) {
        char buf[128];
        snprintf(buf, sizeof(buf), "ERROR: GetWindowDesktopId failed with HR 0x%08X", hr);
        return env->NewStringUTF(buf);
    }

    std::string idStr = GuidToString(id);
    return env->NewStringUTF(idStr.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_fastdisplay_FastDesktop_switchDesktop(JNIEnv* env, jobject obj, jstring desktopId) {
    if (!desktopId) return JNI_FALSE;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    Fast_IVirtualDesktopManagerInternal* pManager = nullptr;
    if (FAILED(GetInternalManager(&pManager)) || !pManager) return JNI_FALSE;

    const char* cId = env->GetStringUTFChars(desktopId, nullptr);
    GUID guid = StringToGuid(cId);
    env->ReleaseStringUTFChars(desktopId, cId);

    Fast_IVirtualDesktop* pDesktop = nullptr;
    if (FAILED(pManager->FindDesktop(&guid, &pDesktop))) {
        pManager->Release();
        return JNI_FALSE;
    }

    HRESULT hr = pManager->SwitchDesktop(nullptr, pDesktop);
    pDesktop->Release();
    pManager->Release();

    return SUCCEEDED(hr) ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_fastdisplay_FastDesktop_moveWindowToDesktop(JNIEnv* env, jobject obj, jlong hwnd, jstring desktopId) {
    if (!desktopId) return JNI_FALSE;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    Fast_IVirtualDesktopManager* pManager = nullptr;
    if (FAILED(CoCreateInstance({0xaa509086, 0x5ca9, 0x4c25, {0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a}}, nullptr, CLSCTX_ALL, __uuidof(Fast_IVirtualDesktopManager), (void**)&pManager))) {
        return JNI_FALSE;
    }

    const char* cId = env->GetStringUTFChars(desktopId, nullptr);
    GUID guid = StringToGuid(cId);
    env->ReleaseStringUTFChars(desktopId, cId);

    HRESULT hr = pManager->MoveWindowToDesktop((HWND)hwnd, guid);
    pManager->Release();

    return SUCCEEDED(hr) ? JNI_TRUE : JNI_FALSE;
}

// ----------------------------------------------------------------------------
// Listener Service implementation (stub for now, requires global JNIEnv + JVM attach)
// ----------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_fastdisplay_FastDesktop_setListener(JNIEnv* env, jobject obj, jobject listener) {
    // Advanced feature: Setting up a background COM thread to listen for notifications
    // Requires registering Fast_IVirtualDesktopNotification with Fast_IVirtualDesktopNotificationService
    // and attaching JVM thread. Omitted for simplicity unless required.
}




