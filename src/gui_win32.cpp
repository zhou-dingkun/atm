#include <windows.h>
#include <string>
#include <vector>
#include <sstream>

struct Field {
    std::wstring label;
    std::wstring value;
    HWND edit = nullptr;
};

static std::vector<Field> g_fields;
static HWND g_output = nullptr;
static std::wstring g_exePath = L"c:\\Users\\zdk\\Desktop\\project\\atm\\src\\atm.exe";

static std::wstring GetText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    std::wstring text(len, L'\0');
    GetWindowTextW(hwnd, text.data(), len + 1);
    return text;
}

static void SetText(HWND hwnd, const std::wstring& text) {
    SetWindowTextW(hwnd, text.c_str());
}

static std::wstring BuildInput() {
    std::wstringstream ss;
    ss << GetText(g_fields[0].edit) << L" " << GetText(g_fields[1].edit) << L"\n";
    ss << GetText(g_fields[2].edit) << L"\n"; // path_type

    if (GetText(g_fields[2].edit) == L"0") {
        ss << GetText(g_fields[3].edit) << L" " << GetText(g_fields[4].edit) << L"\n";
    } else {
        ss << GetText(g_fields[5].edit) << L" " << GetText(g_fields[6].edit) << L" " << GetText(g_fields[7].edit) << L"\n";
    }

    ss << GetText(g_fields[8].edit) << L"\n";  // f0
    ss << GetText(g_fields[9].edit) << L"\n";  // co2 ppm
    ss << GetText(g_fields[10].edit) << L"\n"; // scale_by_mix
    ss << GetText(g_fields[11].edit) << L" " << GetText(g_fields[12].edit) << L" " << GetText(g_fields[13].edit) << L"\n"; // band
    ss << GetText(g_fields[14].edit) << L"\n"; // h2o_scale
    ss << GetText(g_fields[15].edit) << L"\n"; // co2_scale
    ss << GetText(g_fields[16].edit) << L"\n"; // h2o_alt_scale
    ss << GetText(g_fields[17].edit) << L"\n"; // h2o csv
    ss << GetText(g_fields[18].edit) << L"\n"; // co2 csv

    return ss.str();
}

static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, out.data(), size);
    return out;
}

static std::wstring RunProcess(const std::wstring& exePath, const std::wstring& input) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE inRead = nullptr, inWrite = nullptr;
    HANDLE outRead = nullptr, outWrite = nullptr;

    if (!CreatePipe(&inRead, &inWrite, &sa, 0)) return L"Failed to create stdin pipe.";
    if (!CreatePipe(&outRead, &outWrite, &sa, 0)) return L"Failed to create stdout pipe.";

    SetHandleInformation(inWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.hStdInput = inRead;
    si.hStdOutput = outWrite;
    si.hStdError = outWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"\"" + exePath + L"\"";

    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        CloseHandle(inRead);
        CloseHandle(inWrite);
        CloseHandle(outRead);
        CloseHandle(outWrite);
        return L"Failed to start process.";
    }

    CloseHandle(inRead);
    CloseHandle(outWrite);

    std::string inputUtf8 = WideToUtf8(input);
    DWORD written = 0;
    WriteFile(inWrite, inputUtf8.data(), static_cast<DWORD>(inputUtf8.size()), &written, nullptr);
    CloseHandle(inWrite);

    std::string output;
    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(outRead, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        output.append(buffer, buffer + read);
    }

    CloseHandle(outRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return Utf8ToWide(output);
}

static void OnRun(HWND hwnd) {
    std::wstring exePath = GetText(g_fields[19].edit);
    if (exePath.empty()) exePath = g_exePath;
    std::wstring input = BuildInput();
    std::wstring output = RunProcess(exePath, input);
    SetText(g_output, output);
}

static void CreateUI(HWND hwnd) {
    g_fields = {
        {L"RH (0-1)", L"0.46"},
        {L"Temperature (C)", L"15"},
        {L"路径类型 (0=水平,1=斜程)", L"0"},
        {L"Altitude_km (水平)", L"0.2"},
        {L"Range_km (水平)", L"1.0"},
        {L"H1_km (斜程)", L"0.0"},
        {L"H2_km (斜程)", L"1.0"},
        {L"GroundRange_km (斜程)", L"0.0"},
        {L"f0 (g/m^3)", L"6.8"},
        {L"CO2 ppm", L"400"},
        {L"按混合比缩放 (1/0)", L"1"},
        {L"start_um", L"3"},
        {L"end_um", L"5"},
        {L"step_um", L"0.05"},
        {L"H2O_scale (-1默认)", L"-1"},
        {L"CO2_scale (-1默认)", L"-1"},
        {L"H2O_alt_scale (-1默认)", L"-1"},
        {L"H2O CSV 路径", L"-"},
        {L"CO2 CSV 路径", L"-"},
        {L"atm.exe 路径", g_exePath},
    };

    int xLabel = 10;
    int xEdit = 260;
    int y = 10;
    int rowH = 26;
    int editW = 360;

    for (auto& field : g_fields) {
        CreateWindowW(L"STATIC", field.label.c_str(), WS_CHILD | WS_VISIBLE,
            xLabel, y + 4, 240, 20, hwnd, nullptr, nullptr, nullptr);
        field.edit = CreateWindowW(L"EDIT", field.value.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            xEdit, y, editW, 22, hwnd, nullptr, nullptr, nullptr);
        y += rowH;
    }

    CreateWindowW(L"BUTTON", L"运行", WS_CHILD | WS_VISIBLE,
        xLabel, y + 4, 80, 26, hwnd, reinterpret_cast<HMENU>(1), nullptr, nullptr);

    g_output = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
        xLabel, y + 40, xEdit + editW - xLabel, 220, hwnd, nullptr, nullptr, nullptr);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateUI(hwnd);
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {
                OnRun(hwnd);
                return 0;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"AtmGuiWin32";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Atmospheric Transmittance GUI",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 700, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
