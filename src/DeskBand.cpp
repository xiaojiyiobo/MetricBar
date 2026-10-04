#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <oleidl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <strsafe.h>
#include <uxtheme.h>
#include <winhttp.h>

#include <algorithm>
#include <atomic>
#include <new>
#include <string>
#include <vector>

#include "NetParsing.h"

namespace
{
    const CLSID CLSID_VpsTraySpeed =
        {0x8c5934e4, 0xa2b7, 0x4f58, {0x9d, 0x7e, 0x3c, 0x24, 0xf6, 0xa1, 0xb0, 0x52}};
    const wchar_t* const kClsidText = L"{8C5934E4-A2B7-4F58-9D7E-3C24F6A1B052}";
    const wchar_t* const kDeskBandCategory = L"{00021492-0000-0000-C000-000000000046}";
    const wchar_t* const kWindowClass = L"MetricBarDeskBandWindow";
    const UINT WM_METRICS_UPDATE = WM_APP + 0x4A1;

    HINSTANCE g_module = nullptr;
    std::atomic<long> g_objectCount(0);
    std::atomic<long> g_serverLocks(0);
    volatile LONG g_activeSite = 0;

    struct BandConfig
    {
        std::wstring dataUrl = L"http://23.94.171.107:18099/homepage.json";
        std::wstring homepageUrl = L"http://23.94.171.107:3002/";
        int refreshSeconds = 10;
        DisplayMode mode = DisplayMode::Both;
        COLORREF textColor = RGB(255, 255, 255);
        COLORREF errorColor = RGB(255, 72, 72);
        int fontPoints = 10;
        int width = 220;
        bool metricsEnabled = false;
        std::wstring separator = L" ";
        std::vector<MetricDefinition> metrics;
    };

    struct SharedState
    {
        bool valid = false;
        bool healthy = false;
        bool hasLegacyPayload = false;
        FieldTable fields;
        PayloadData payload = {};
    };

    std::wstring ModuleDirectory()
    {
        wchar_t path[MAX_PATH] = {};
        DWORD length = GetModuleFileNameW(g_module, path, MAX_PATH);
        if (length == 0 || length >= MAX_PATH) return L".";
        wchar_t* slash = wcsrchr(path, L'\\');
        if (slash) *slash = L'\0';
        return path;
    }

    bool HasIniSection(const std::wstring& ini, const wchar_t* section)
    {
        wchar_t buffer[4096] = {};
        return GetPrivateProfileSectionW(section, buffer, ARRAYSIZE(buffer), ini.c_str()) > 0;
    }

    std::wstring ReadIniText(const std::wstring& ini, const wchar_t* section,
        const wchar_t* key, const wchar_t* fallback)
    {
        wchar_t value[1024] = {};
        GetPrivateProfileStringW(section, key, fallback, value, 1024, ini.c_str());
        return value;
    }

    std::vector<MetricDefinition> DefaultMetrics()
    {
        return {
            {L"net.down", L"\x2193{value}", MetricFormat::Raw},
            {L"net.up", L"\x2191{value}", MetricFormat::Raw},
            {L"vps_cpu", L"CPU {value}%", MetricFormat::Raw}
        };
    }

    COLORREF ParseColor(const std::wstring& value, COLORREF fallback)
    {
        if (_wcsicmp(value.c_str(), L"auto") == 0) return fallback;
        if (value.size() != 7 || value[0] != L'#') return fallback;
        wchar_t* end = nullptr;
        unsigned long rgb = wcstoul(value.c_str() + 1, &end, 16);
        if (!end || *end != L'\0') return fallback;
        return RGB((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
    }

    BandConfig LoadConfig() noexcept
    {
        BandConfig config;
        try
        {
            const std::wstring ini = ModuleDirectory() + L"\\config.ini";
            const wchar_t* baseSection = HasIniSection(ini, L"MetricBar")
                ? L"MetricBar"
                : (HasIniSection(ini, L"TaskbarJsonMonitor") ? L"TaskbarJsonMonitor" : L"VpsTraySpeed");
            config.dataUrl = ReadIniText(ini, baseSection, L"DataUrl", config.dataUrl.c_str());
            config.homepageUrl = ReadIniText(ini, baseSection, L"HomepageUrl", config.homepageUrl.c_str());
            config.refreshSeconds = std::max(2, std::min(3600,
                static_cast<int>(GetPrivateProfileIntW(baseSection, L"RefreshSeconds", 10, ini.c_str()))));
            config.fontPoints = std::max(8, std::min(20,
                static_cast<int>(GetPrivateProfileIntW(baseSection, L"FontSize", 10, ini.c_str()))));
            config.width = std::max(60, std::min(400,
                static_cast<int>(GetPrivateProfileIntW(baseSection, L"Width", 220, ini.c_str()))));

            const std::wstring mode = ReadIniText(ini, baseSection, L"DisplayMode", L"both");
            if (_wcsicmp(mode.c_str(), L"download") == 0) config.mode = DisplayMode::Download;
            else if (_wcsicmp(mode.c_str(), L"upload") == 0) config.mode = DisplayMode::Upload;
            else config.mode = DisplayMode::Both;

            config.textColor = ParseColor(ReadIniText(ini, baseSection, L"TextColor", L"auto"), RGB(255, 255, 255));
            config.errorColor = ParseColor(ReadIniText(ini, baseSection, L"ErrorColor", L"#FF4848"), RGB(255, 72, 72));

            if (HasIniSection(ini, L"Metrics"))
            {
                config.metricsEnabled = true;
                const std::vector<MetricDefinition> defaults = DefaultMetrics();
                int count = static_cast<int>(GetPrivateProfileIntW(L"Metrics", L"Count", 3, ini.c_str()));
                if (count < 1 || count > 16) count = 3;
                config.separator = ReadIniText(ini, L"Metrics", L"Separator", L" ");
                if (config.separator.empty()) config.separator = L" ";
                for (int i = 1; i <= count; ++i)
                {
                    wchar_t section[16] = {};
                    std::swprintf(section, ARRAYSIZE(section), L"M%d", i);
                    const MetricDefinition fallback = i <= static_cast<int>(defaults.size())
                        ? defaults[static_cast<size_t>(i - 1)]
                        : MetricDefinition{L"", L"{value}", MetricFormat::Raw};
                    MetricDefinition metric;
                    metric.path = ReadIniText(ini, section, L"Path", fallback.path.c_str());
                    metric.templateText = ReadIniText(ini, section, L"Template", fallback.templateText.c_str());
                    if (metric.path.empty()) metric.path = fallback.path;
                    if (metric.templateText.find(L"{value}") == std::wstring::npos)
                        metric.templateText = fallback.templateText;
                    const std::wstring format = ReadIniText(ini, section, L"Format", L"raw");
                    metric.format = _wcsicmp(format.c_str(), L"bytes") == 0
                        ? MetricFormat::Bytes : MetricFormat::Raw;
                    config.metrics.push_back(metric);
                }
            }
        }
        catch (...)
        {
        }
        return config;
    }

    bool Utf8ToWide(const std::vector<char>& bytes, std::wstring& result)
    {
        if (bytes.empty()) return false;
        int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(),
            static_cast<int>(bytes.size()), nullptr, 0);
        if (count <= 0) return false;
        result.resize(static_cast<size_t>(count));
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(),
            static_cast<int>(bytes.size()), &result[0], count) != count)
            return false;
        if (!result.empty() && result[0] == 0xfeff) result.erase(0, 1);
        return true;
    }

    bool DownloadJson(const std::wstring& url, std::wstring& json) noexcept
    {
        HINTERNET session = nullptr;
        HINTERNET connection = nullptr;
        HINTERNET request = nullptr;
        bool success = false;
        try
        {
            URL_COMPONENTSW parts = {};
            parts.dwStructSize = sizeof(parts);
            parts.dwSchemeLength = static_cast<DWORD>(-1);
            parts.dwHostNameLength = static_cast<DWORD>(-1);
            parts.dwUrlPathLength = static_cast<DWORD>(-1);
            parts.dwExtraInfoLength = static_cast<DWORD>(-1);
            if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts)) return false;

            std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
            std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
            if (parts.dwExtraInfoLength) path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
            if (path.empty()) path = L"/";

            session = WinHttpOpen(L"MetricBar/3.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
            if (!session) throw 1;
            if (!WinHttpSetTimeouts(session, 8000, 8000, 8000, 8000)) throw 2;
            connection = WinHttpConnect(session, host.c_str(), parts.nPort, 0);
            if (!connection) throw 3;
            const DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
            request = WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr,
                WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
            if (!request) throw 4;
            if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request, nullptr)) throw 5;

            DWORD status = 0;
            DWORD statusSize = sizeof(status);
            if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX) || status != 200)
                throw 6;

            std::vector<char> body;
            char buffer[4096];
            for (;;)
            {
                DWORD read = 0;
                if (!WinHttpReadData(request, buffer, sizeof(buffer), &read)) throw 7;
                if (read == 0) break;
                if (body.size() + read > 1024 * 1024) throw 8;
                body.insert(body.end(), buffer, buffer + read);
            }
            success = Utf8ToWide(body, json);
        }
        catch (...)
        {
            success = false;
        }
        if (request) WinHttpCloseHandle(request);
        if (connection) WinHttpCloseHandle(connection);
        if (session) WinHttpCloseHandle(session);
        return success;
    }

    HRESULT SetRegistryString(HKEY root, const std::wstring& key, const wchar_t* valueName,
        const std::wstring& value)
    {
        HKEY handle = nullptr;
        LONG error = RegCreateKeyExW(root, key.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
            KEY_WRITE | KEY_WOW64_64KEY, nullptr, &handle, nullptr);
        if (error != ERROR_SUCCESS) return HRESULT_FROM_WIN32(error);
        error = RegSetValueExW(handle, valueName, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(handle);
        return HRESULT_FROM_WIN32(error);
    }

    void DeleteRegistryValue(HKEY root, const wchar_t* key, const wchar_t* valueName)
    {
        HKEY handle = nullptr;
        if (RegOpenKeyExW(root, key, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &handle) == ERROR_SUCCESS)
        {
            RegDeleteValueW(handle, valueName);
            RegCloseKey(handle);
        }
    }
}

class VpsDeskBand final : public IDeskBand, public IObjectWithSite, public IPersistStream, public IInputObject
{
public:
    VpsDeskBand() : refs_(1), site_(nullptr), hwnd_(nullptr), tooltip_(nullptr), font_(nullptr),
        stopEvent_(CreateEventW(nullptr, TRUE, FALSE, nullptr)), workerStarted_(false), claimed_(false)
    {
        ++g_objectCount;
        InitializeCriticalSection(&stateLock_);
        tooltipText_[0] = L'\0';
    }

    ~VpsDeskBand()
    {
        CloseDW(0);
        if (font_) DeleteObject(font_);
        if (stopEvent_) CloseHandle(stopEvent_);
        DeleteCriticalSection(&stateLock_);
        --g_objectCount;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** output) override
    {
        if (!output) return E_POINTER;
        *output = nullptr;
        if (riid == IID_IUnknown || riid == IID_IOleWindow || riid == IID_IDockingWindow || riid == IID_IDeskBand)
            *output = static_cast<IDeskBand*>(this);
        else if (riid == IID_IObjectWithSite)
            *output = static_cast<IObjectWithSite*>(this);
        else if (riid == IID_IPersist || riid == IID_IPersistStream)
            *output = static_cast<IPersistStream*>(this);
        else if (riid == IID_IInputObject)
            *output = static_cast<IInputObject*>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&refs_)); }

    STDMETHODIMP_(ULONG) Release() override
    {
        const LONG count = InterlockedDecrement(&refs_);
        if (count == 0) delete this;
        return static_cast<ULONG>(count);
    }

    STDMETHODIMP SetSite(IUnknown* site) override
    {
        try
        {
            if (!site)
            {
                CloseDW(0);
                return S_OK;
            }
            if (site_) return S_OK;
            if (InterlockedCompareExchange(&g_activeSite, 1, 0) != 0) return E_FAIL;
            claimed_ = true;

            IOleWindow* oleWindow = nullptr;
            HRESULT hr = site->QueryInterface(IID_PPV_ARGS(&oleWindow));
            if (FAILED(hr))
            {
                ReleaseClaim();
                return hr;
            }
            HWND parent = nullptr;
            hr = oleWindow->GetWindow(&parent);
            oleWindow->Release();
            if (FAILED(hr) || !parent)
            {
                ReleaseClaim();
                return FAILED(hr) ? hr : E_FAIL;
            }

            site_ = site;
            site_->AddRef();
            config_ = LoadConfig();
            if (!CreateBandWindow(parent))
            {
                site_->Release();
                site_ = nullptr;
                ReleaseClaim();
                return HRESULT_FROM_WIN32(GetLastError());
            }
            StartWorker();
            return S_OK;
        }
        catch (...)
        {
            CloseDW(0);
            return E_FAIL;
        }
    }

    STDMETHODIMP GetSite(REFIID riid, void** output) override
    {
        if (!output) return E_POINTER;
        *output = nullptr;
        return site_ ? site_->QueryInterface(riid, output) : E_FAIL;
    }

    STDMETHODIMP GetWindow(HWND* output) override
    {
        if (!output) return E_POINTER;
        *output = hwnd_;
        return hwnd_ ? S_OK : E_FAIL;
    }

    STDMETHODIMP ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }

    STDMETHODIMP ShowDW(BOOL show) override
    {
        if (hwnd_) ShowWindow(hwnd_, show ? SW_SHOW : SW_HIDE);
        return S_OK;
    }

    STDMETHODIMP CloseDW(DWORD) override
    {
        try
        {
            if (stopEvent_) SetEvent(stopEvent_);
            HWND oldWindow = hwnd_;
            hwnd_ = nullptr;
            if (tooltip_)
            {
                DestroyWindow(tooltip_);
                tooltip_ = nullptr;
            }
            if (oldWindow && IsWindow(oldWindow)) DestroyWindow(oldWindow);
            if (site_)
            {
                site_->Release();
                site_ = nullptr;
            }
            ReleaseClaim();
        }
        catch (...)
        {
        }
        return S_OK;
    }

    STDMETHODIMP ResizeBorderDW(const RECT*, IUnknown*, BOOL) override { return E_NOTIMPL; }

    STDMETHODIMP GetBandInfo(DWORD, DWORD viewMode, DESKBANDINFO* info) override
    {
        if (!info) return E_POINTER;
        if (info->dwMask & DBIM_MINSIZE)
        {
            info->ptMinSize.x = config_.mode == DisplayMode::Both ? std::min(config_.width, 110) : 55;
            info->ptMinSize.y = 22;
        }
        if (info->dwMask & DBIM_MAXSIZE)
        {
            info->ptMaxSize.x = -1;
            info->ptMaxSize.y = 40;
        }
        if (info->dwMask & DBIM_INTEGRAL)
        {
            info->ptIntegral.x = 1;
            info->ptIntegral.y = 1;
        }
        if (info->dwMask & DBIM_ACTUAL)
        {
            info->ptActual.x = config_.width;
            info->ptActual.y = 24;
        }
        if (info->dwMask & DBIM_TITLE)
            StringCchCopyW(info->wszTitle, ARRAYSIZE(info->wszTitle), L"MetricBar");
        if (info->dwMask & DBIM_MODEFLAGS)
            info->dwModeFlags = DBIMF_NORMAL | ((viewMode & DBIF_VIEWMODE_FLOATING) ? DBIMF_VARIABLEHEIGHT : 0);
        if (info->dwMask & DBIM_BKCOLOR)
            info->dwMask &= ~DBIM_BKCOLOR;
        return S_OK;
    }

    STDMETHODIMP GetClassID(CLSID* output) override
    {
        if (!output) return E_POINTER;
        *output = CLSID_VpsTraySpeed;
        return S_OK;
    }

    STDMETHODIMP IsDirty() override { return S_FALSE; }
    STDMETHODIMP Load(IStream*) override { return S_OK; }
    STDMETHODIMP Save(IStream*, BOOL) override { return S_OK; }

    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* size) override
    {
        if (!size) return E_POINTER;
        size->QuadPart = 0;
        return S_OK;
    }

    STDMETHODIMP UIActivateIO(BOOL activate, MSG*) override
    {
        if (activate && hwnd_) SetFocus(hwnd_);
        return S_OK;
    }

    STDMETHODIMP HasFocusIO() override { return hwnd_ && GetFocus() == hwnd_ ? S_OK : S_FALSE; }
    STDMETHODIMP TranslateAcceleratorIO(MSG*) override { return S_FALSE; }

private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
    {
        VpsDeskBand* self = reinterpret_cast<VpsDeskBand*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
            self = static_cast<VpsDeskBand*>(create->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(window, message, wparam, lparam);
        try
        {
            switch (message)
            {
            case WM_PAINT: return self->Paint();
            case WM_PRINTCLIENT:
            {
                RECT rect = {};
                GetClientRect(window, &rect);
                self->DrawToDc(reinterpret_cast<HDC>(wparam), rect);
                return 0;
            }
            case WM_ERASEBKGND: return 1;
            case WM_GETTEXT:
                if (lparam && wparam)
                    return StringCchCopyW(reinterpret_cast<wchar_t*>(lparam), static_cast<size_t>(wparam),
                        self->displayText_.c_str()) == S_OK ? static_cast<LRESULT>(self->displayText_.size()) : 0;
                return 0;
            case WM_GETTEXTLENGTH:
                return static_cast<LRESULT>(self->displayText_.size());
            case WM_LBUTTONUP:
                ShellExecuteW(window, L"open", self->config_.homepageUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            case WM_SETCURSOR:
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            case WM_NOTIFY:
            {
                NMHDR* header = reinterpret_cast<NMHDR*>(lparam);
                if (header && header->code == TTN_GETDISPINFOW)
                {
                    NMTTDISPINFOW* info = reinterpret_cast<NMTTDISPINFOW*>(lparam);
                    info->lpszText = self->tooltipText_;
                    return 0;
                }
                break;
            }
            case WM_METRICS_UPDATE:
                self->ApplyLatestState();
                return 0;
            }
        }
        catch (...)
        {
            return 0;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

    bool CreateBandWindow(HWND parent)
    {
        INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_WIN95_CLASSES};
        InitCommonControlsEx(&controls);
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = g_module;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = kWindowClass;
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

        hwnd_ = CreateWindowExW(WS_EX_TRANSPARENT, kWindowClass, L"MetricBar",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, config_.width, 24,
            parent, nullptr, g_module, this);
        if (!hwnd_) return false;

        displayText_ = L"?";

        HDC dc = GetDC(hwnd_);
        int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 96;
        if (dc) ReleaseDC(hwnd_, dc);
        font_ = CreateFontW(-MulDiv(config_.fontPoints, dpi, 72), 0, 0, 0, FW_SEMIBOLD,
            FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        tooltip_ = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, hwnd_, nullptr, g_module, nullptr);
        if (tooltip_)
        {
            TOOLINFOW tool = {};
            tool.cbSize = sizeof(tool);
            tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            tool.hwnd = hwnd_;
            tool.uId = reinterpret_cast<UINT_PTR>(hwnd_);
            tool.lpszText = LPSTR_TEXTCALLBACKW;
            SendMessageW(tooltip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
            SendMessageW(tooltip_, TTM_SETMAXTIPWIDTH, 0, 500);
        }
        StringCchCopyW(tooltipText_, ARRAYSIZE(tooltipText_), L"MetricBar: connecting...");
        return true;
    }

    LRESULT Paint()
    {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(hwnd_, &paint);
        if (!dc) return 0;
        RECT rect = {};
        GetClientRect(hwnd_, &rect);
        DrawToDc(dc, rect);
        EndPaint(hwnd_, &paint);
        return 0;
    }

    void DrawToDc(HDC dc, const RECT& rect)
    {
        RECT drawRect = rect;
        if (FAILED(DrawThemeParentBackground(hwnd_, dc, &drawRect)))
        {
            HBRUSH background = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
            FillRect(dc, &rect, background);
            DeleteObject(background);
        }
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, currentHealthy_ ? config_.textColor : config_.errorColor);
        HFONT oldFont = font_ ? static_cast<HFONT>(SelectObject(dc, font_)) : nullptr;
        DrawTextW(dc, displayText_.c_str(), -1, &drawRect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        if (oldFont) SelectObject(dc, oldFont);
    }

    static DWORD WINAPI WorkerEntry(void* context)
    {
        VpsDeskBand* self = static_cast<VpsDeskBand*>(context);
        self->WorkerLoop();
        self->Release();
        return 0;
    }

    void StartWorker()
    {
        if (workerStarted_ || !stopEvent_) return;
        workerStarted_ = true;
        AddRef();
        HANDLE thread = CreateThread(nullptr, 0, WorkerEntry, this, 0, nullptr);
        if (thread) CloseHandle(thread);
        else
        {
            workerStarted_ = false;
            Release();
        }
    }

    void WorkerLoop() noexcept
    {
        try
        {
            while (WaitForSingleObject(stopEvent_, 0) != WAIT_OBJECT_0)
            {
                SharedState next;
                std::wstring json;
                next.valid = DownloadJson(config_.dataUrl, json) && BuildFieldTable(json, next.fields);
                next.hasLegacyPayload = next.valid && TryGetLegacyPayload(next.fields, next.payload);
                next.healthy = next.valid;
                double online = 0.0;
                if (next.valid && TryGetNumericField(next.fields, L"online", online))
                    next.healthy = online >= 1.0;
                EnterCriticalSection(&stateLock_);
                state_ = next;
                LeaveCriticalSection(&stateLock_);
                HWND target = hwnd_;
                if (target && IsWindow(target)) PostMessageW(target, WM_METRICS_UPDATE, 0, 0);
                if (WaitForSingleObject(stopEvent_, static_cast<DWORD>(config_.refreshSeconds * 1000)) == WAIT_OBJECT_0)
                    break;
            }
        }
        catch (...)
        {
        }
    }

    void ApplyLatestState()
    {
        SharedState snapshot;
        EnterCriticalSection(&stateLock_);
        snapshot = state_;
        LeaveCriticalSection(&stateLock_);
        if (snapshot.valid)
        {
            if (config_.metricsEnabled)
                displayText_ = RenderMetrics(snapshot.fields, config_.metrics, config_.separator);
            else if (snapshot.hasLegacyPayload)
                displayText_ = FormatDisplay(snapshot.payload, config_.mode);
            else
                displayText_ = L"?";
            currentHealthy_ = snapshot.healthy;
            if (snapshot.hasLegacyPayload)
                StringCchCopyW(tooltipText_, ARRAYSIZE(tooltipText_), FormatTooltip(snapshot.payload).c_str());
            else
                StringCchPrintfW(tooltipText_, ARRAYSIZE(tooltipText_), L"MetricBar: %ls", displayText_.c_str());
        }
        else
        {
            displayText_ = L"?";
            currentHealthy_ = false;
            StringCchCopyW(tooltipText_, ARRAYSIZE(tooltipText_), L"MetricBar: data source unavailable");
        }
        if (hwnd_) InvalidateRect(hwnd_, nullptr, TRUE);
    }

    void ReleaseClaim()
    {
        if (claimed_)
        {
            claimed_ = false;
            InterlockedExchange(&g_activeSite, 0);
        }
    }

    volatile LONG refs_;
    IUnknown* site_;
    HWND hwnd_;
    HWND tooltip_;
    HFONT font_;
    HANDLE stopEvent_;
    bool workerStarted_;
    bool claimed_;
    bool currentHealthy_ = false;
    BandConfig config_;
    CRITICAL_SECTION stateLock_;
    SharedState state_;
    std::wstring displayText_;
    wchar_t tooltipText_[192];
};

class BandClassFactory final : public IClassFactory
{
public:
    BandClassFactory() : refs_(1) { ++g_objectCount; }
    ~BandClassFactory() { --g_objectCount; }

    STDMETHODIMP QueryInterface(REFIID riid, void** output) override
    {
        if (!output) return E_POINTER;
        *output = nullptr;
        if (riid != IID_IUnknown && riid != IID_IClassFactory) return E_NOINTERFACE;
        *output = static_cast<IClassFactory*>(this);
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
    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** output) override
    {
        if (outer) return CLASS_E_NOAGGREGATION;
        VpsDeskBand* band = new (std::nothrow) VpsDeskBand();
        if (!band) return E_OUTOFMEMORY;
        HRESULT hr = band->QueryInterface(riid, output);
        band->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL lock) override
    {
        if (lock) ++g_serverLocks;
        else --g_serverLocks;
        return S_OK;
    }
private:
    volatile LONG refs_;
};

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void*)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** output)
{
    if (clsid != CLSID_VpsTraySpeed) return CLASS_E_CLASSNOTAVAILABLE;
    BandClassFactory* factory = new (std::nothrow) BandClassFactory();
    if (!factory) return E_OUTOFMEMORY;
    HRESULT hr = factory->QueryInterface(riid, output);
    factory->Release();
    return hr;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DllCanUnloadNow()
{
    return g_objectCount.load() == 0 && g_serverLocks.load() == 0 ? S_OK : S_FALSE;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DllRegisterServer()
{
    wchar_t modulePath[MAX_PATH] = {};
    if (!GetModuleFileNameW(g_module, modulePath, MAX_PATH)) return HRESULT_FROM_WIN32(GetLastError());
    const std::wstring clsidKey = std::wstring(L"CLSID\\") + kClsidText;
    HRESULT hr = SetRegistryString(HKEY_CLASSES_ROOT, clsidKey, nullptr, L"MetricBar");
    if (SUCCEEDED(hr)) hr = SetRegistryString(HKEY_CLASSES_ROOT, clsidKey + L"\\InprocServer32", nullptr, modulePath);
    if (SUCCEEDED(hr)) hr = SetRegistryString(HKEY_CLASSES_ROOT, clsidKey + L"\\InprocServer32", L"ThreadingModel", L"Apartment");
    if (SUCCEEDED(hr)) hr = SetRegistryString(HKEY_CLASSES_ROOT,
        clsidKey + L"\\Implemented Categories\\" + kDeskBandCategory, nullptr, L"");
    if (SUCCEEDED(hr)) hr = SetRegistryString(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar", kClsidText, L"MetricBar");
    if (SUCCEEDED(hr)) hr = SetRegistryString(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", kClsidText, L"MetricBar");
    if (FAILED(hr))
    {
        RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidKey.c_str());
        DeleteRegistryValue(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar", kClsidText);
        DeleteRegistryValue(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", kClsidText);
    }
    return hr;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DllUnregisterServer()
{
    const std::wstring clsidKey = std::wstring(L"CLSID\\") + kClsidText;
    RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidKey.c_str());
    DeleteRegistryValue(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar", kClsidText);
    DeleteRegistryValue(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", kClsidText);
    return S_OK;
}
