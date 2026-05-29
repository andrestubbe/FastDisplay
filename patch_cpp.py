import re

def patch_file():
    with open('native/FastDesktop.cpp', 'r') as f:
        content = f.read()

    # 1. Add ComInit and CLSID declarations at the top
    top_insert = """#include <windows.h>
#include <objbase.h>
#include <shobjidl_core.h>
#include <vector>
#include <string>
#include <jni.h>
#include <iostream>

// RAII for COM
struct ComInit {
    HRESULT hr;
    ComInit(DWORD coinit = COINIT_MULTITHREADED) {
        hr = CoInitializeEx(nullptr, coinit);
    }
    ~ComInit() {
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
    }
    bool ok() const { return SUCCEEDED(hr); }
};

static const CLSID CLSID_ImmersiveShell_local = 
{0xc2f03a33, 0x21f5, 0x47fa, {0xb4, 0xbb, 0x15, 0x63, 0x62, 0xa2, 0xf2, 0x39}};

static const CLSID CLSID_VirtualDesktopManager_local =
{ 0xaa509086, 0x5ca9, 0x4c25, { 0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a } };
"""
    content = re.sub(r'#include <windows\.h>.*?#include <iostream>', top_insert, content, flags=re.DOTALL)

    # 2. Replace GetDesktopID with version-safe version
    old_get_desktop_id = """static HRESULT GetDesktopID(Fast_IVirtualDesktop* pDesktop, GUID* pGuid) {
    if (!pDesktop) return E_POINTER;
    typedef HRESULT(__stdcall* GetIDFn)(void*, GUID*);
    void\*\*\* vtable = (void\*\*\*)pDesktop;
    // Win11: GetID is index 3. Win10: GetID is index 4.
    int index = g_isWin11COM \? 3 : 4;
    GetIDFn fn = (GetIDFn)\(\*vtable\)\[index\];
    return fn\(pDesktop, pGuid\);
}"""

    new_get_desktop_id = """static HRESULT GetDesktopID(Fast_IVirtualDesktop* pDesktop, GUID* pGuid) {
    if (!pDesktop || !pGuid) return E_POINTER;

    typedef HRESULT(__stdcall* GetIDFn)(void*, GUID*);
    void*** vtable = (void***)pDesktop;

    for (int index : {3, 4}) { // Win11 first, then Win10
        GetIDFn fn = (GetIDFn)(*vtable)[index];
        if (!fn) continue;
        GUID tmp = {};
        HRESULT hr = fn(pDesktop, &tmp);
        if (SUCCEEDED(hr)) {
            *pGuid = tmp;
            g_isWin11COM = (index == 3);
            return S_OK;
        }
    }
    return E_FAIL;
}"""
    content = content.replace("static HRESULT GetDesktopID(Fast_IVirtualDesktop* pDesktop, GUID* pGuid) {\n    if (!pDesktop) return E_POINTER;\n    typedef HRESULT(__stdcall* GetIDFn)(void*, GUID*);\n    void*** vtable = (void***)pDesktop;\n    // Win11: GetID is index 3. Win10: GetID is index 4.\n    int index = g_isWin11COM ? 3 : 4;\n    GetIDFn fn = (GetIDFn)(*vtable)[index];\n    return fn(pDesktop, pGuid);\n}", new_get_desktop_id)

    # 3. Replace JNI functions to use ComInit and proper error handling
    # enumerateDesktops
    old_enum = """extern "C" JNIEXPORT jobjectArray JNICALL
Java_fastdisplay_FastDesktop_enumerateDesktops(JNIEnv* env, jobject obj) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);"""
    
    new_enum = """extern "C" JNIEXPORT jobjectArray JNICALL
Java_fastdisplay_FastDesktop_enumerateDesktops(JNIEnv* env, jobject obj) {
    ComInit com;
    if (!com.ok()) return nullptr;"""
    content = content.replace(old_enum, new_enum)

    content = content.replace("const CLSID CLSID_VirtualDesktopManager = { 0xaa509086, 0x5ca9, 0x4c25, { 0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a } };\n    if (SUCCEEDED(CoCreateInstance(CLSID_VirtualDesktopManager", "if (SUCCEEDED(CoCreateInstance(CLSID_VirtualDesktopManager_local")


    # getCurrentDesktopId
    old_get_curr = """extern "C" JNIEXPORT jstring JNICALL
Java_fastdisplay_FastDesktop_getCurrentDesktopId(JNIEnv* env, jobject obj) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    Fast_IVirtualDesktopManager* pManager = nullptr;
    const CLSID CLSID_VirtualDesktopManager = { 0xaa509086, 0x5ca9, 0x4c25, { 0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a } };
    HRESULT hr = CoCreateInstance(CLSID_VirtualDesktopManager, nullptr, CLSCTX_ALL, __uuidof(Fast_IVirtualDesktopManager), (void**)&pManager);
    if (FAILED(hr) || !pManager) {
        char buf[128];
        snprintf(buf, sizeof(buf), "ERROR: CoCreateInstance VirtualDesktopManager failed with HR 0x%08X", hr);
        return env->NewStringUTF(buf);
    }"""
    
    new_get_curr = """extern "C" JNIEXPORT jstring JNICALL
Java_fastdisplay_FastDesktop_getCurrentDesktopId(JNIEnv* env, jobject obj) {
    ComInit com;
    if (!com.ok()) return nullptr;
    
    Fast_IVirtualDesktopManager* pManager = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_VirtualDesktopManager_local, nullptr, CLSCTX_ALL, __uuidof(Fast_IVirtualDesktopManager), (void**)&pManager);
    if (FAILED(hr) || !pManager) {
        return nullptr;
    }"""
    content = content.replace(old_get_curr, new_get_curr)
    content = content.replace("        char buf[128];\n        snprintf(buf, sizeof(buf), \"ERROR: GetWindowDesktopId failed with HR 0x%08X\", hr);\n        return env->NewStringUTF(buf);", "        return nullptr;")

    # switchDesktop
    old_switch = """extern "C" JNIEXPORT jboolean JNICALL
Java_fastdisplay_FastDesktop_switchDesktop(JNIEnv* env, jobject obj, jstring desktopId) {
    if (!desktopId) return JNI_FALSE;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);"""
    
    new_switch = """extern "C" JNIEXPORT jboolean JNICALL
Java_fastdisplay_FastDesktop_switchDesktop(JNIEnv* env, jobject obj, jstring desktopId) {
    if (!desktopId) return JNI_FALSE;
    ComInit com;
    if (!com.ok()) return JNI_FALSE;"""
    content = content.replace(old_switch, new_switch)

    # moveWindowToDesktop
    old_move = """extern "C" JNIEXPORT jboolean JNICALL
Java_fastdisplay_FastDesktop_moveWindowToDesktop(JNIEnv* env, jobject obj, jlong hwnd, jstring desktopId) {
    if (!desktopId) return JNI_FALSE;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    Fast_IVirtualDesktopManager* pManager = nullptr;
    if (FAILED(CoCreateInstance({0xaa509086, 0x5ca9, 0x4c25, {0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a}}, nullptr, CLSCTX_ALL, __uuidof(Fast_IVirtualDesktopManager), (void**)&pManager))) {"""
    
    new_move = """extern "C" JNIEXPORT jboolean JNICALL
Java_fastdisplay_FastDesktop_moveWindowToDesktop(JNIEnv* env, jobject obj, jlong hwnd, jstring desktopId) {
    if (!desktopId) return JNI_FALSE;
    ComInit com;
    if (!com.ok()) return JNI_FALSE;
    Fast_IVirtualDesktopManager* pManager = nullptr;
    if (FAILED(CoCreateInstance(CLSID_VirtualDesktopManager_local, nullptr, CLSCTX_ALL, __uuidof(Fast_IVirtualDesktopManager), (void**)&pManager))) {"""
    content = content.replace(old_move, new_move)

    # 4. Global JNI_OnLoad and Listener Implementation
    old_listener = """extern "C" JNIEXPORT void JNICALL
Java_fastdisplay_FastDesktop_setListener(JNIEnv* env, jobject obj, jobject listener) {
    // Advanced feature: Setting up a background COM thread to listen for notifications
    // Requires registering Fast_IVirtualDesktopNotification with Fast_IVirtualDesktopNotificationService
    // and attaching JVM thread. Omitted for simplicity unless required.
}"""
    
    new_listener = """static JavaVM* g_vm = nullptr;
static jobject g_listenerGlobal = nullptr;
static DWORD g_notificationCookie = 0;
static Fast_IVirtualDesktopNotificationService* g_notificationService = nullptr;

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    g_vm = vm;
    return JNI_VERSION_1_8;
}

class VirtualDesktopNotification :
    public Fast_IVirtualDesktopNotification {

    LONG m_ref = 1;

public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(Fast_IVirtualDesktopNotification)) {
            *ppv = static_cast<Fast_IVirtualDesktopNotification*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_ref);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this;
        return r;
    }

    HRESULT STDMETHODCALLTYPE VirtualDesktopCreated(Fast_IVirtualDesktop*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyBegin(Fast_IVirtualDesktop*, Fast_IVirtualDesktop*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyFailed(Fast_IVirtualDesktop*, Fast_IVirtualDesktop*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE VirtualDesktopDestroyed(Fast_IVirtualDesktop*, Fast_IVirtualDesktop*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE ViewVirtualDesktopChanged(IApplicationView*) override { return S_OK; }

    HRESULT STDMETHODCALLTYPE CurrentVirtualDesktopChanged(Fast_IVirtualDesktop* pOldDesktop,
                                                           Fast_IVirtualDesktop* pNewDesktop) override {
        if (!g_vm || !g_listenerGlobal || !pNewDesktop) return S_OK;

        JNIEnv* env = nullptr;
        bool detach = false;
        if (g_vm->GetEnv((void**)&env, JNI_VERSION_1_8) != JNI_OK) {
            if (g_vm->AttachCurrentThread((void**)&env, nullptr) != JNI_OK) {
                return S_OK;
            }
            detach = true;
        }

        GUID id = {};
        if (FAILED(GetDesktopID(pNewDesktop, &id))) {
            if (detach) g_vm->DetachCurrentThread();
            return S_OK;
        }

        std::string idStr = GuidToString(id);
        jclass cls = env->GetObjectClass(g_listenerGlobal);
        if (!cls) {
            if (detach) g_vm->DetachCurrentThread();
            return S_OK;
        }

        jmethodID mid = env->GetMethodID(cls, "onDesktopChanged", "(Ljava/lang/String;)V");
        if (!mid) {
            if (detach) g_vm->DetachCurrentThread();
            return S_OK;
        }

        jstring jId = env->NewStringUTF(idStr.c_str());
        env->CallVoidMethod(g_listenerGlobal, mid, jId);
        env->DeleteLocalRef(jId);

        if (detach) g_vm->DetachCurrentThread();
        return S_OK;
    }
};

extern "C" JNIEXPORT void JNICALL
Java_fastdisplay_FastDesktop_setListener(JNIEnv* env, jobject obj, jobject listener) {
    ComInit com;
    if (!com.ok()) return;

    // Clear previous listener
    if (g_listenerGlobal) {
        env->DeleteGlobalRef(g_listenerGlobal);
        g_listenerGlobal = nullptr;
    }
    if (g_notificationService && g_notificationCookie) {
        g_notificationService->Unregister(g_notificationCookie);
        g_notificationCookie = 0;
    }

    if (!listener) {
        return;
    }

    g_listenerGlobal = env->NewGlobalRef(listener);

    IServiceProvider* pServiceProvider = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_ImmersiveShell_local, nullptr, CLSCTX_LOCAL_SERVER,
                                  __uuidof(IServiceProvider), (void**)&pServiceProvider);
    if (FAILED(hr)) return;

    IUnknown* pUnk = nullptr;
    hr = pServiceProvider->QueryService(CLSID_VirtualDesktopNotificationService,
                                        __uuidof(Fast_IVirtualDesktopNotificationService),
                                        (void**)&g_notificationService);
    pServiceProvider->Release();
    if (FAILED(hr) || !g_notificationService) return;

    VirtualDesktopNotification* notif = new VirtualDesktopNotification();
    g_notificationService->Register(notif, &g_notificationCookie);
    notif->Release(); // service holds a ref now
}"""
    content = content.replace(old_listener, new_listener)

    with open('native/FastDesktop.cpp', 'w') as f:
        f.write(content)

if __name__ == '__main__':
    patch_file()
