#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <iostream>
#include <cstring>

typedef HRESULT (WINAPI* DllGetClassObjectFn)(REFCLSID, REFIID, void**);
typedef HRESULT (WINAPI* DllCanUnloadNowFn)();

static const CLSID CLSID_VpsTraySpeed =
    {0x8c5934e4, 0xa2b7, 0x4f58, {0x9d, 0x7e, 0x3c, 0x24, 0xf6, 0xa1, 0xb0, 0x52}};

static int Fail(const char* message, HRESULT hr = E_FAIL)
{
    std::cerr << "FAIL " << message << " (0x" << std::hex << static_cast<unsigned long>(hr) << ")\n";
    return 1;
}

int main(int argc, char** argv)
{
    if (argc != 2) return Fail("DLL path argument missing");
    HMODULE module = LoadLibraryA(argv[1]);
    if (!module) return Fail("LoadLibrary", HRESULT_FROM_WIN32(GetLastError()));

    FARPROC getClassRaw = GetProcAddress(module, "DllGetClassObject");
    FARPROC canUnloadRaw = GetProcAddress(module, "DllCanUnloadNow");
    DllGetClassObjectFn getClass = nullptr;
    DllCanUnloadNowFn canUnload = nullptr;
    static_assert(sizeof(getClass) == sizeof(getClassRaw), "function pointer size mismatch");
    static_assert(sizeof(canUnload) == sizeof(canUnloadRaw), "function pointer size mismatch");
    std::memcpy(&getClass, &getClassRaw, sizeof(getClass));
    std::memcpy(&canUnload, &canUnloadRaw, sizeof(canUnload));
    if (!getClass || !canUnload) return Fail("required COM exports missing");

    IClassFactory* factory = nullptr;
    HRESULT hr = getClass(CLSID_VpsTraySpeed, IID_IClassFactory, reinterpret_cast<void**>(&factory));
    if (FAILED(hr) || !factory) return Fail("DllGetClassObject", hr);

    IDeskBand* band = nullptr;
    hr = factory->CreateInstance(nullptr, IID_IDeskBand, reinterpret_cast<void**>(&band));
    factory->Release();
    if (FAILED(hr) || !band) return Fail("CreateInstance IDeskBand", hr);

    IObjectWithSite* site = nullptr;
    IPersistStream* persist = nullptr;
    IInputObject* input = nullptr;
    if (FAILED(band->QueryInterface(IID_IObjectWithSite, reinterpret_cast<void**>(&site))))
        return Fail("IObjectWithSite missing");
    if (FAILED(band->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&persist))))
        return Fail("IPersistStream missing");
    if (FAILED(band->QueryInterface(IID_IInputObject, reinterpret_cast<void**>(&input))))
        return Fail("IInputObject missing");

    CLSID classId = {};
    if (FAILED(persist->GetClassID(&classId)) || classId != CLSID_VpsTraySpeed)
        return Fail("IPersistStream GetClassID mismatch");

    DESKBANDINFO info = {};
    info.dwMask = DBIM_MINSIZE | DBIM_ACTUAL | DBIM_TITLE | DBIM_MODEFLAGS;
    if (FAILED(band->GetBandInfo(1, DBIF_VIEWMODE_NORMAL, &info)))
        return Fail("IDeskBand GetBandInfo");
    if (info.ptMinSize.x <= 0 || info.ptActual.x <= 0 || wcscmp(info.wszTitle, L"MetricBar") != 0)
        return Fail("IDeskBand metadata invalid");

    input->Release();
    persist->Release();
    site->Release();
    band->Release();
    if (canUnload() != S_OK) return Fail("DllCanUnloadNow did not return S_OK");

    FreeLibrary(module);
    std::cout << "PASS COM exports and required interfaces\n";
    std::cout << "PASS IDeskBand metadata\n";
    std::cout << "PASS COM lifetime and unload contract\n";
    return 0;
}
