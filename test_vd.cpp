#include <windows.h>
#include <shobjidl_core.h>
#include <iostream>

const CLSID CLSID_VirtualDesktopManager = { 0xaa509086, 0x5ca9, 0x4c25, { 0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a } };

int main() {
    CoInitialize(NULL);
    IVirtualDesktopManager* pManager = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_VirtualDesktopManager, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pManager));
    if (SUCCEEDED(hr)) {
        GUID id;
        if (SUCCEEDED(pManager->GetWindowDesktopId(GetDesktopWindow(), &id))) {
            printf("GetDesktopWindow ID: %08x\n", id.Data1);
        } else {
            printf("Failed GetDesktopWindow\n");
        }
        if (SUCCEEDED(pManager->GetWindowDesktopId(GetShellWindow(), &id))) {
            printf("GetShellWindow ID: %08x\n", id.Data1);
        } else {
            printf("Failed GetShellWindow\n");
        }
    }
    return 0;
}
