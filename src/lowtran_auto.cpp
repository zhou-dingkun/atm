#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Options {
    fs::path lowtran_dir;
    fs::path tape5_path;
    int click_x = 0;
    int click_y = 0;
    int timeout_sec = 20;
    bool has_coords = false;
};

static bool parse_args(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lowtran-dir" && i + 1 < argc) {
            opt.lowtran_dir = fs::path(argv[++i]);
        } else if (arg == "--tape5" && i + 1 < argc) {
            opt.tape5_path = fs::path(argv[++i]);
        } else if (arg == "--coords" && i + 2 < argc) {
            opt.click_x = std::stoi(argv[++i]);
            opt.click_y = std::stoi(argv[++i]);
            opt.has_coords = true;
        } else if (arg == "--timeout" && i + 1 < argc) {
            opt.timeout_sec = std::stoi(argv[++i]);
        } else {
            std::cout << "Unknown arg: " << arg << "\n";
            return false;
        }
    }
    return true;
}

static bool read_coords(const fs::path& file, int& x, int& y) {
    std::ifstream in(file);
    if (!in.is_open()) return false;
    in >> x >> y;
    return !in.fail();
}

static std::vector<fs::path> find_tape5_candidates(const fs::path& dir) {
    std::vector<fs::path> candidates;
    if (!fs::exists(dir)) return candidates;
    for (auto it = fs::recursive_directory_iterator(dir); it != fs::recursive_directory_iterator(); ++it) {
        if (!it->is_regular_file()) continue;
        auto name = it->path().filename().string();
        if (name == "Tape5" || name == "Tape5.dat") {
            candidates.push_back(it->path());
        }
    }
    return candidates;
}

static void safe_remove(const fs::path& p) {
    std::error_code ec;
    fs::remove(p, ec);
}

static bool parse_average_transmittance(const fs::path& tape6, double& value) {
    std::ifstream in(tape6);
    if (!in.is_open()) return false;
    std::string line;
    const std::string key = "AVERAGE TRANSMITTANCE";
    while (std::getline(in, line)) {
        if (line.find(key) != std::string::npos) {
            auto pos = line.find('=');
            if (pos == std::string::npos) return false;
            std::string num = line.substr(pos + 1);
            try {
                value = std::stod(num);
                return true;
            } catch (...) {
                return false;
            }
        }
    }
    return false;
}

static bool wait_for_outputs(const fs::path& dir, int timeout_sec) {
    auto t6 = dir / "TAPE6";
    auto t7 = dir / "TAPE7";
    auto t8 = dir / "TAPE8";

    const int step_ms = 500;
    int remain = timeout_sec * 1000;
    while (remain > 0) {
        if (fs::exists(t6) && fs::exists(t7) && fs::exists(t8)) {
            return true;
        }
        Sleep(step_ms);
        remain -= step_ms;
    }
    return false;
}

struct WindowSearch {
    DWORD pid;
    HWND hwnd;
};

static BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lparam) {
    auto* data = reinterpret_cast<WindowSearch*>(lparam);
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == data->pid && IsWindowVisible(hwnd)) {
        data->hwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}

static HWND find_main_window(DWORD pid) {
    WindowSearch data{ pid, nullptr };
    EnumWindows(enum_windows_proc, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

static void focus_window(HWND hwnd) {
    if (!hwnd) return;
    ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
    SetActiveWindow(hwnd);
    BringWindowToTop(hwnd);
    SetFocus(hwnd);
}

static void post_enter(HWND hwnd) {
    if (!hwnd) return;
    PostMessage(hwnd, WM_KEYDOWN, VK_RETURN, 0);
    PostMessage(hwnd, WM_KEYUP, VK_RETURN, 0);
}

static void send_enter_key() {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_RETURN;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_RETURN;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

int main(int argc, char** argv) {
    Options opt;
    opt.lowtran_dir = fs::path("c:/Users/zdk/Desktop/project/atm/lowtran7");
    opt.tape5_path = opt.lowtran_dir / "Tape5";

    if (!parse_args(argc, argv, opt)) {
        std::cout << "Usage: lowtran_auto --lowtran-dir <dir> --tape5 <Tape5> --coords <x y> --timeout <sec>\n";
        return 1;
    }

    if (!fs::exists(opt.lowtran_dir)) {
        std::cout << "Lowtran dir not found: " << opt.lowtran_dir.string() << "\n";
        return 1;
    }

    fs::path lowtran_exe = opt.lowtran_dir / "Lowtran.exe";
    if (!fs::exists(lowtran_exe)) {
        std::cout << "Lowtran.exe not found: " << lowtran_exe.string() << "\n";
        return 1;
    }

    if (!fs::exists(opt.tape5_path)) {
        auto candidates = find_tape5_candidates(opt.lowtran_dir);
        if (candidates.size() == 1) {
            opt.tape5_path = candidates.front();
        } else if (!candidates.empty()) {
            std::cout << "Multiple Tape5 candidates found. Please specify --tape5\n";
            for (const auto& p : candidates) {
                std::cout << "  " << p.string() << "\n";
            }
            return 1;
        } else {
            std::cout << "Tape5 not found: " << opt.tape5_path.string() << "\n";
            std::cout << "Use --tape5 to provide the template path.\n";
            return 1;
        }
    }

    if (!opt.has_coords) {
        fs::path coord_file = opt.lowtran_dir / "run_lowtran";
        if (!read_coords(coord_file, opt.click_x, opt.click_y)) {
            std::cout << "Run button coords not provided and file missing: " << coord_file.string() << "\n";
            return 1;
        }
    }

    fs::path tape5_dest = opt.lowtran_dir / "Tape5";
    if (opt.tape5_path != tape5_dest) {
        safe_remove(tape5_dest);
    }
    safe_remove(opt.lowtran_dir / "TAPE6");
    safe_remove(opt.lowtran_dir / "TAPE7");
    safe_remove(opt.lowtran_dir / "TAPE8");

    if (opt.tape5_path != tape5_dest) {
        try {
            fs::copy_file(opt.tape5_path, tape5_dest, fs::copy_options::overwrite_existing);
        } catch (const std::exception& ex) {
            std::cout << "Failed to copy Tape5: " << ex.what() << "\n";
            return 1;
        }
    }

    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);

    std::wstring cmd = L"\"" + lowtran_exe.wstring() + L"\"";
    std::wstring workdir = opt.lowtran_dir.wstring();

    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, workdir.c_str(), &si, &pi)) {
        std::cout << "Failed to start Lowtran.exe\n";
        return 1;
    }

    WaitForInputIdle(pi.hProcess, 5000);
    HWND main_hwnd = find_main_window(pi.dwProcessId);
    focus_window(main_hwnd);

    Sleep(1000);
    SetCursorPos(opt.click_x, opt.click_y);
    mouse_event(MOUSEEVENTF_LEFTDOWN, opt.click_x, opt.click_y, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, opt.click_x, opt.click_y, 0, 0);
    Sleep(300);
    focus_window(main_hwnd);
    post_enter(main_hwnd);
    send_enter_key();

    bool ok = wait_for_outputs(opt.lowtran_dir, opt.timeout_sec);

    TerminateProcess(pi.hProcess, 0);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (!ok) {
        std::cout << "Timeout waiting for TAPE6/7/8\n";
        return 1;
    }

    double avg = 0.0;
    if (parse_average_transmittance(opt.lowtran_dir / "TAPE6", avg)) {
        std::cout << "AVERAGE TRANSMITTANCE = " << avg << "\n";
    } else {
        std::cout << "Lowtran finished. Outputs in: " << opt.lowtran_dir.string() << "\n";
    }
    return 0;
}
