#include <Windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <iostream>
#include <string>

#pragma comment(lib, "Pdh.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define IDC_CPU_INDICATOR 1001

PDH_HQUERY cpuQuery;
PDH_HCOUNTER cpuTotal;

double GetCpuUsage() {
    PDH_FMT_COUNTERVALUE counterValue;
    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, NULL, &counterValue);
    return counterValue.doubleValue;
}

void UpdateWWindow(HWND hwnd) {
    double cpuUsage = GetCpuUsage();
    std::wstring text = L"CPU Usage: " + std::to_wstring(cpuUsage) + L"%";

    // Отображение информации о загрузке CPU в окне
    SetWindowText(hwnd, text.c_str());

    // Отображение текстового индикатора
    int indicatorWidth = static_cast<int>(cpuUsage * 4);  // Множитель для ширины индикатора
    std::wstring indicator(indicatorWidth, L'|');
    SetWindowText(GetDlgItem(hwnd, IDC_CPU_INDICATOR), indicator.c_str());
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    PdhOpenQuery(NULL, 0, &cpuQuery);
    PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", 0, &cpuTotal);
    PdhCollectQueryData(cpuQuery);

    const wchar_t CLASS_NAME[] = L"PerformanceMonitorWindow";

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Performance Monitor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 200,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Добавление текстового индикатора в окно
    CreateWindow(
        L"STATIC",
        NULL,
        WS_CHILD | WS_VISIBLE,
        10, 60, 400, 20,
        hwnd,
        reinterpret_cast<HMENU>(IDC_CPU_INDICATOR),
        hInstance,
        NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    PdhCloseQuery(cpuQuery);

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        UpdateWWindow(hwnd);
        SetTimer(hwnd, 1, 1000, NULL);
        break;
    case WM_TIMER:
        UpdateWWindow(hwnd);
        break;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
