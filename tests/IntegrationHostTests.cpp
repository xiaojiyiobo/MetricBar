#include <windows.h>
#include <objbase.h>
#include <psapi.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

typedef HRESULT (WINAPI* DllGetClassObjectFn)(REFCLSID, REFIID, void**);
typedef HRESULT (WINAPI* DllCanUnloadNowFn)();

static const CLSID CLSID_VpsTraySpeed =
    {0x8c5934e4, 0xa2b7, 0x4f58, {0x9d, 0x7e, 0x3c, 0x24, 0xf6, 0xa1, 0xb0, 0x52}};

class TestSite final : public IOleWindow
{
public:
    explicit TestSite(HWND window) : refs_(1), window_(window) {}
    STDMETHODIMP QueryInterface(REFIID riid, void** output) override
    {
        if (!output) return E_POINTER;
        *output = nullptr;
        if (riid != IID_IUnknown && riid != IID_IOleWindow) return E_NOINTERFACE;
        *output = static_cast<IOleWindow*>(this);
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&refs_)); }
    STDMETHODIMP_(ULONG) Release() override
    {
        LONG count = InterlockedDecrement(&refs_);
        if (!count) delete this;
        return static_cast<ULONG>(count);
    }
    STDMETHODIMP GetWindow(HWND* output) override
    {
        if (!output) return E_POINTER;
        *output = window_;
        return S_OK;
    }
    STDMETHODIMP ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }
private:
    volatile LONG refs_;
    HWND window_;
};

static int Fail(const char* message, HRESULT hr = E_FAIL)
{
    std::cerr << "FAIL " << message << " (0x" << std::hex << static_cast<unsigned long>(hr) << ")\n";
    return 1;
}

static void PumpMessages(DWORD milliseconds)
{
    const ULONGLONG end = GetTickCount64() + milliseconds;
    MSG message;
    while (GetTickCount64() < end)
    {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        Sleep(10);
    }
}

static LRESULT CALLBACK TestParentProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_ERASEBKGND || message == WM_PRINTCLIENT)
    {
        HDC dc = reinterpret_cast<HDC>(wparam);
        RECT rect = {};
        GetClientRect(window, &rect);
        HBRUSH brush = CreateSolidBrush(RGB(32, 32, 32));
        FillRect(dc, &rect, brush);
        DeleteObject(brush);
        return 1;
    }
    if (message == WM_PAINT)
    {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(window, &paint);
        RECT rect = {};
        GetClientRect(window, &rect);
        HBRUSH brush = CreateSolidBrush(RGB(32, 32, 32));
        FillRect(dc, &rect, brush);
        DeleteObject(brush);
        EndPaint(window, &paint);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

static std::string ToUtf8(const wchar_t* text)
{
    int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) return std::string();
    std::string output(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, &output[0], length, nullptr, nullptr);
    output.resize(static_cast<size_t>(length - 1));
    return output;
}

static bool SavePreview(HWND window, const char* path)
{
    RECT rect = {};
    GetClientRect(window, &rect);
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    HDC source = GetDC(window);
    HDC memory = CreateCompatibleDC(source);
    HBITMAP bitmap = CreateCompatibleBitmap(source, width, height);
    HGDIOBJ old = SelectObject(memory, bitmap);
    SendMessageW(window, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(memory), PRF_CLIENT);

    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    std::vector<unsigned char> pixels(static_cast<size_t>(width * height * 4));
    const bool copied = GetDIBits(memory, bitmap, 0, static_cast<UINT>(height), pixels.data(), &info, DIB_RGB_COLORS) != 0;

    BITMAPFILEHEADER file = {};
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    file.bfSize = file.bfOffBits + static_cast<DWORD>(pixels.size());
    std::ofstream stream(path, std::ios::binary);
    if (copied && stream)
    {
        stream.write(reinterpret_cast<const char*>(&file), sizeof(file));
        stream.write(reinterpret_cast<const char*>(&info.bmiHeader), sizeof(info.bmiHeader));
        stream.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
    }
    SelectObject(memory, old);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(window, source);
    return copied && stream.good();
}

int main(int argc, char** argv)
{
    if (argc < 3 || argc > 4) return Fail("usage: IntegrationHostTests DLL success|failure [preview.bmp]");
    const bool expectSuccess = std::strcmp(argv[2], "success") == 0;
    HMODULE module = LoadLibraryA(argv[1]);
    if (!module) return Fail("LoadLibrary", HRESULT_FROM_WIN32(GetLastError()));

    FARPROC getClassRaw = GetProcAddress(module, "DllGetClassObject");
    FARPROC canUnloadRaw = GetProcAddress(module, "DllCanUnloadNow");
    DllGetClassObjectFn getClass = nullptr;
    DllCanUnloadNowFn canUnload = nullptr;
    std::memcpy(&getClass, &getClassRaw, sizeof(getClass));
    std::memcpy(&canUnload, &canUnloadRaw, sizeof(canUnload));
    if (!getClass || !canUnload) return Fail("required COM exports missing");

    WNDCLASSW parentClass = {};
    parentClass.lpfnWndProc = TestParentProc;
    parentClass.hInstance = GetModuleHandleW(nullptr);
    parentClass.lpszClassName = L"VpsDeskBandTestParent";
    RegisterClassW(&parentClass);
    HWND parent = CreateWindowExW(0, parentClass.lpszClassName, L"", WS_OVERLAPPED,
        0, 0, 300, 80, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!parent) return Fail("test parent window");

    IClassFactory* factory = nullptr;
    HRESULT hr = getClass(CLSID_VpsTraySpeed, IID_IClassFactory, reinterpret_cast<void**>(&factory));
    if (FAILED(hr)) return Fail("class factory", hr);
    IDeskBand* band = nullptr;
    hr = factory->CreateInstance(nullptr, IID_IDeskBand, reinterpret_cast<void**>(&band));
    factory->Release();
    if (FAILED(hr)) return Fail("band instance", hr);
    IObjectWithSite* objectWithSite = nullptr;
    hr = band->QueryInterface(IID_IObjectWithSite, reinterpret_cast<void**>(&objectWithSite));
    if (FAILED(hr)) return Fail("IObjectWithSite", hr);

    TestSite* site = new TestSite(parent);
    hr = objectWithSite->SetSite(site);
    if (FAILED(hr)) return Fail("SetSite", hr);
    HWND bandWindow = nullptr;
    if (FAILED(band->GetWindow(&bandWindow)) || !bandWindow) return Fail("band window");
    MoveWindow(bandWindow, 0, 0, 180, 28, TRUE);

    wchar_t text[128] = {};
    const ULONGLONG deadline = GetTickCount64() + (expectSuccess ? 12000 : 4000);
    do
    {
        PumpMessages(100);
        GetWindowTextW(bandWindow, text, 128);
        if (expectSuccess && wcscmp(text, L"?") != 0) break;
    } while (GetTickCount64() < deadline);

    const bool gotSuccessText = text[0] != L'\0' && wcscmp(text, L"?") != 0;
    if (expectSuccess != gotSuccessText)
    {
        std::wcerr << L"FAIL unexpected displayed text: [" << text << L"]\n";
        return 1;
    }

    PROCESS_MEMORY_COUNTERS memory = {};
    memory.cb = sizeof(memory);
    GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory));
    std::cout << "PASS displayed text [" << ToUtf8(text) << "]\n";
    std::cout << "PASS integration host working set " << (memory.WorkingSetSize / (1024 * 1024)) << " MB\n";
    if (argc >= 4 && !SavePreview(bandWindow, argv[3])) return Fail("preview capture");

    objectWithSite->SetSite(nullptr);
    site->Release();
    objectWithSite->Release();
    band->Release();
    DestroyWindow(parent);
    for (int i = 0; i < 100 && canUnload() != S_OK; ++i) Sleep(100);
    if (canUnload() != S_OK) return Fail("DLL remained busy after CloseDW");
    FreeLibrary(module);
    std::cout << "PASS background worker stopped without blocking UI thread\n";
    return 0;
}
