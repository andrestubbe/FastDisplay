#include <windows.h>
#include <objbase.h>
#include <shobjidl_core.h>
#include <vector>
#include <string>
#include <jni.h>
#include <iostream>

static const CLSID CLSID_ImmersiveShell_local = 
{0xc2f03a33, 0x21f5, 0x47fa, {0xb4, 0xbb, 0x15, 0x63, 0x62, 0xa2, 0xf2, 0x39}};

static const CLSID CLSID_VirtualDesktopManager_local =
{ 0xaa509086, 0x5ca9, 0x4c25, { 0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a } };

static const CLSID CLSID_VirtualDesktopManagerInternal_local =
{0xc5e0cdca, 0x7b6e, 0x41b2, {0x9f, 0xc4, 0xd9, 0x39, 0x75, 0xcc, 0x46, 0x7b}};

struct IApplicationView;
struct Fast_IVirtualDesktop : public IUnknown {};

struct __declspec(uuid("a5cd92ff-29be-454c-8d04-d82879fb3f1b")) Fast_IVirtualDesktopManagerInternal : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetCount(UINT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(IApplicationView*, Fast_IVirtualDesktop*) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(IApplicationView*, int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(HWND, Fast_IVirtualDesktop**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktops(HWND, IObjectArray**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAdjacentDesktop(Fast_IVirtualDesktop*, int, Fast_IVirtualDesktop**) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(HWND, Fast_IVirtualDesktop*) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDesktopW(HWND, Fast_IVirtualDesktop**) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveDesktop(Fast_IVirtualDesktop*, HWND, int) = 0;
    virtual HRESULT STDMETHODCALLTYPE RemoveDesktop(Fast_IVirtualDesktop*, Fast_IVirtualDesktop*) = 0;
    virtual HRESULT STDMETHODCALLTYPE FindDesktop(GUID*, Fast_IVirtualDesktop**) = 0;
};

struct __declspec(uuid("a5cd92ff-29be-454c-8d04-d82879fb3f1b")) Fast_IVirtualDesktopManager : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE IsWindowOnCurrentVirtualDesktop(HWND, BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWindowDesktopId(HWND, GUID*) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveWindowToDesktop(HWND, REFGUID) = 0;
};

// Removed COM notification interfaces since they are unstable across Win11 builds.

extern JavaVM* g_jvm;
static jobject g_listenerGlobal = nullptr;
static bool g_isWin11COM = false;

static std::string GuidToString(const GUID& guid) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return std::string(buf);
}

static GUID StringToGuid(const std::string& str) {
    GUID guid = {};
    unsigned int d1, d2, d3, d4[8];
    if (sscanf_s(str.c_str(), "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                 &d1, &d2, &d3,
                 &d4[0], &d4[1], &d4[2], &d4[3],
                 &d4[4], &d4[5], &d4[6], &d4[7]) == 11) {
        guid.Data1 = d1; guid.Data2 = d2; guid.Data3 = d3;
        for (int i = 0; i < 8; ++i) guid.Data4[i] = (unsigned char)d4[i];
    }
    return guid;
}

static HRESULT GetInternalManager(Fast_IVirtualDesktopManagerInternal** ppManager) {
    *ppManager = nullptr;
    IServiceProvider* pServiceProvider = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ImmersiveShell_local, nullptr, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (void**)&pServiceProvider);
    if (FAILED(hr)) return hr;

    IUnknown* pUnk = nullptr;
    hr = pServiceProvider->QueryService(CLSID_VirtualDesktopManagerInternal_local, __uuidof(Fast_IVirtualDesktopManagerInternal), (void**)&pUnk);
    pServiceProvider->Release();
    if (FAILED(hr)) return hr;

    HRESULT qihr = pUnk->QueryInterface(__uuidof(Fast_IVirtualDesktopManagerInternal), (void**)ppManager);
    for (int i = 0; i < 5; i++) {
        if (SUCCEEDED(pUnk->QueryInterface(__uuidof(Fast_IVirtualDesktopManagerInternal), (void**)ppManager))) {
            g_isWin11COM = (i < 4);
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

extern "C" JNIEXPORT jobjectArray JNICALL
Java_fastdisplay_FastDesktop_enumerateDesktops(JNIEnv* env, jobject obj) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    
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

    Fast_IVirtualDesktop* currentDesktop = nullptr;
    pManager->GetCurrentDesktop(nullptr, &currentDesktop);
    GUID currentGuid = {};
    if (currentDesktop) {
        GetDesktopID(currentDesktop, &currentGuid);
    }

    jclass infoClass = env->FindClass("fastdisplay/FastDesktop$DesktopInfo");
    jmethodID ctor = env->GetMethodID(infoClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;Z)V");

    jobjectArray result = env->NewObjectArray(count, infoClass, nullptr);

    for (UINT i = 0; i < count; ++i) {
        Fast_IVirtualDesktop* pDesktop = nullptr;
        if (SUCCEEDED(pDesktops->GetAt(i, __uuidof(IUnknown), (void**)&pDesktop))) {
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
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    
    Fast_IVirtualDesktopManager* pManager = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_VirtualDesktopManager_local, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pManager));
    if (FAILED(hr) || !pManager) {
        return nullptr;
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
        return nullptr;
    }

    std::string idStr = GuidToString(id);
    return env->NewStringUTF(idStr.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_fastdisplay_FastDesktop_switchDesktop(JNIEnv* env, jobject obj, jstring desktopId) {
    if (!desktopId) return JNI_FALSE;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

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
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    Fast_IVirtualDesktopManager* pManager = nullptr;
    if (FAILED(CoCreateInstance(CLSID_VirtualDesktopManager_local, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pManager)))) {
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
// Polling Listener implementation
// ----------------------------------------------------------------------------
#include <thread>
#include <atomic>
#include <chrono>

static std::atomic<bool> g_polling(false);
static std::thread g_pollThread;

static void PollingThread() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    std::string lastDesktopId = "";

    while (g_polling) {
        Fast_IVirtualDesktopManagerInternal* pManager = nullptr;
        if (SUCCEEDED(GetInternalManager(&pManager)) && pManager) {
            Fast_IVirtualDesktop* currentDesktop = nullptr;
            pManager->GetCurrentDesktop(nullptr, &currentDesktop);
            if (currentDesktop) {
                GUID currentGuid = {};
                if (SUCCEEDED(GetDesktopID(currentDesktop, &currentGuid))) {
                    std::string newDesktopId = GuidToString(currentGuid);
                    
                    if (!lastDesktopId.empty() && lastDesktopId != newDesktopId) {
                        // Desktop changed!
                        if (g_jvm && g_listenerGlobal) {
                            JNIEnv* env = nullptr;
                            bool detach = false;
                            if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
                                if (g_jvm->AttachCurrentThread((void**)&env, nullptr) == JNI_OK) {
                                    detach = true;
                                }
                            }
                            if (env) {
                                jclass listenerClass = env->GetObjectClass(g_listenerGlobal);
                                jmethodID methodId = env->GetMethodID(listenerClass, "onDesktopChanged", "(Ljava/lang/String;)V");
                                if (methodId) {
                                    jstring jStr = env->NewStringUTF(newDesktopId.c_str());
                                    env->CallVoidMethod(g_listenerGlobal, methodId, jStr);
                                    env->DeleteLocalRef(jStr);
                                }
                                env->DeleteLocalRef(listenerClass);
                            }
                            if (detach) g_jvm->DetachCurrentThread();
                        }
                    }
                    lastDesktopId = newDesktopId;
                }
                currentDesktop->Release();
            }
            pManager->Release();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    CoUninitialize();
}

extern "C" JNIEXPORT void JNICALL
Java_fastdisplay_FastDesktop_setListener(JNIEnv* env, jobject obj, jobject listener) {
    if (g_listenerGlobal) {
        env->DeleteGlobalRef(g_listenerGlobal);
        g_listenerGlobal = nullptr;
    }

    if (!listener) {
        g_polling = false;
        if (g_pollThread.joinable()) {
            g_pollThread.join();
        }
        return;
    }

    if (!g_jvm) env->GetJavaVM(&g_jvm);
    g_listenerGlobal = env->NewGlobalRef(listener);

    if (!g_polling) {
        g_polling = true;
        if (g_pollThread.joinable()) {
            g_pollThread.join();
        }
        g_pollThread = std::thread(PollingThread);
    }
}
