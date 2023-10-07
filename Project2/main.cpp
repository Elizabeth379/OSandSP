#include <windows.h>
#include <iostream>
#include <string>

const wchar_t g_szClassName[] = L"FileCompressionAppClass";
const int IDC_COMPRESS_BUTTON = 101;
const int IDC_DECOMPRESS_BUTTON = 102;

std::wstring GetOpenFileName(HWND hwnd) {
    OPENFILENAME ofn;
    wchar_t szFileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName) / sizeof(*szFileName);
    ofn.lpstrTitle = L"Select a file";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        return szFileName;
    }

    return L"";
}

std::wstring GetSaveFileName(HWND hwnd) {
    OPENFILENAME ofn;
    wchar_t szFileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = sizeof(szFileName) / sizeof(*szFileName);
    ofn.lpstrTitle = L"Save As";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn)) {
        return szFileName;
    }

    return L"";
}

void CompressFile(HWND hwnd) {
    std::wstring sourcePath = GetOpenFileName(hwnd);
    if (sourcePath.empty()) {
        return; // Пользователь отменил выбор файла
    }

    std::wstring destPath = GetSaveFileName(hwnd);
    if (destPath.empty()) {
        return; // Пользователь отменил выбор места сохранения файла
    }

    HANDLE hSourceFile = CreateFile(sourcePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hSourceFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"Failed to open source file.", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    HANDLE hDestFile = CreateFile(destPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hDestFile == INVALID_HANDLE_VALUE) {
        CloseHandle(hSourceFile);
        MessageBox(hwnd, L"Failed to open destination file.", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    const DWORD bufferSize = 4096;
    BYTE buffer[bufferSize];

    DWORD bytesRead, bytesWritten;
    while (ReadFile(hSourceFile, buffer, bufferSize, &bytesRead, nullptr) && bytesRead > 0) {
        if (!WriteFile(hDestFile, buffer, bytesRead, &bytesWritten, nullptr) || bytesWritten != bytesRead) {
            MessageBox(hwnd, L"Error writing to destination file.", L"Error", MB_ICONERROR | MB_OK);
            break;
        }
    }

    CloseHandle(hSourceFile);
    CloseHandle(hDestFile);

    MessageBox(hwnd, L"Compression completed successfully.", L"Info", MB_OK | MB_ICONINFORMATION);
}

void DecompressFile(HWND hwnd) {
    std::wstring sourcePath = GetOpenFileName(hwnd);
    if (sourcePath.empty()) {
        return; // Пользователь отменил выбор файла
    }

    std::wstring destPath = GetSaveFileName(hwnd);
    if (destPath.empty()) {
        return; // Пользователь отменил выбор места сохранения файла
    }

    // Добавьте код распаковки файла с использованием Windows API
    HANDLE hSourceFile = CreateFile(sourcePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hSourceFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"Failed to open source file.", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    HANDLE hDestFile = CreateFile(destPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hDestFile == INVALID_HANDLE_VALUE) {
        CloseHandle(hSourceFile);
        MessageBox(hwnd, L"Failed to open destination file.", L"Error", MB_ICONERROR | MB_OK);
        return;
    }

    const DWORD bufferSize = 4096;
    BYTE buffer[bufferSize];

    DWORD bytesRead, bytesWritten;
    while (ReadFile(hSourceFile, buffer, bufferSize, &bytesRead, nullptr) && bytesRead > 0) {
        if (!WriteFile(hDestFile, buffer, bytesRead, &bytesWritten, nullptr) || bytesWritten != bytesRead) {
            MessageBox(hwnd, L"Error writing to destination file.", L"Error", MB_ICONERROR | MB_OK);
            break;
        }
    }

    CloseHandle(hSourceFile);
    CloseHandle(hDestFile);

    MessageBox(hwnd, L"Decompression completed successfully.", L"Info", MB_OK | MB_ICONINFORMATION);

}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowEx(0, L"BUTTON", L"Compress", WS_CHILD | WS_VISIBLE, 10, 10, 100, 30, hwnd, (HMENU)IDC_COMPRESS_BUTTON, GetModuleHandle(nullptr), nullptr);
        CreateWindowEx(0, L"BUTTON", L"Decompress", WS_CHILD | WS_VISIBLE, 120, 10, 100, 30, hwnd, (HMENU)IDC_DECOMPRESS_BUTTON, GetModuleHandle(nullptr), nullptr);
        break;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case IDC_COMPRESS_BUTTON: {
            CompressFile(hwnd);
            break;
        }

        case IDC_DECOMPRESS_BUTTON: {
            DecompressFile(hwnd);
            break;
        }
        }
        break;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }

    default: {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    // Регистрация класса окна
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(nullptr, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Создание окна
    hwnd = CreateWindowEx(0, g_szClassName, L"File Compression App", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 300, 100, nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr) {
        MessageBox(nullptr, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Отображение окна
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Цикл обработки сообщений
    while (GetMessage(&Msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}
