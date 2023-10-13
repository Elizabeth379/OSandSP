#include <windows.h>
#include <tchar.h>
#include <cmath>
#include <vector>
#include <thread>
#include <string>
#include <atomic>

std::vector<std::thread> threads;
double result = 0.0;
double inputX = 0.0;  // Добавлено для хранения введенного значения x
HANDLE mutex;
std::atomic<bool> exitFlag(false);
std::vector<double> inputPoints;


// Переменная для отслеживания предыдущего результата
double previousResult = 0.0;

void CalculateExponential(double x, HWND hInputEdit, HWND hWnd) {
    while (true) {
        // Захватываем мьютекс
        WaitForSingleObject(mutex, INFINITE);

        // Проверяем флаг завершения
        if (exitFlag) {
            ReleaseMutex(mutex);
            break;
        }

        // Имитация долгой операции
        result = exp(x);

        // Сохраняем введенное значение x
        inputX = x;
        // Сохраняем введенное значение x
        inputPoints.push_back(x);

        // Отправляем сообщение для установки текста в поле ввода только при изменении результата
        if (result != previousResult) {
            previousResult = result;

            // Обновляем текст в поле ввода
            SetWindowText(hInputEdit, std::to_wstring(result).c_str());

            // Перерисовываем окно
            InvalidateRect(hWnd, NULL, TRUE);
        }

        // Освобождаем мьютекс
        ReleaseMutex(mutex);

        // Даем немного времени, чтобы обновление успело произойти перед следующей итерацией
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}





LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD(wParam) == 1) {
                // Получаем значение из поля для ввода
                TCHAR buffer[256];
                GetWindowText(GetDlgItem(hWnd, 0), buffer, sizeof(buffer) / sizeof(buffer[0]));
                double x = _ttof(buffer);

                // Запускаем потоки для вычисления
                threads.emplace_back(CalculateExponential, x, GetDlgItem(hWnd, 0), hWnd);
            }
        }
        break;

    case WM_DESTROY:
        // Устанавливаем флаг завершения
        exitFlag.store(true);

        // Ждем завершения потоков
        for (std::thread& t : threads) {
            t.join();
        }

        // Закрываем дескриптор мьютекса
        CloseHandle(mutex);

        // Завершаем программу
        PostQuitMessage(0);
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Получаем размеры клиентской области окна
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);

        // Создаем красное перо для рисования линии
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
        HGDIOBJ hOldPen = SelectObject(hdc, hPen);

        // Рисуем график экспоненты
        MoveToEx(hdc, 0, clientRect.bottom, NULL);
        for (int i = 0; i < clientRect.right; ++i) {
            double x = static_cast<double>(i);
            double y = exp(x / 50.0) * 50.0;  // Экспоненциальная функция, масштабированная для лучшей видимости
            LineTo(hdc, i, clientRect.bottom - static_cast<int>(y));
        }
        // Рисуем точки введенных значений
        for (double x : inputPoints) {
            Ellipse(hdc, static_cast<int>(x), clientRect.bottom - static_cast<int>(exp(x / 50.0) * 50.0),
                static_cast<int>(x) + 5, clientRect.bottom - static_cast<int>(exp(x / 50.0) * 50.0) + 5);
        }

        // Очищаем ресурсы
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        EndPaint(hWnd, &ps);
    }
    break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    const TCHAR szWindowClass[] = _T("Win32App");
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, _T("Call to RegisterClassEx failed!"), _T("Win32 Guided Tour"), NULL);
        return 1;
    }

    // Создание окна
    HWND hWnd = CreateWindow(szWindowClass, _T("Exponential function"), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        MessageBox(NULL, _T("Call to CreateWindow failed!"), _T("Win32 Guided Tour"), NULL);
        return 1;
    }

    // Создание мьютекса
    mutex = CreateMutex(NULL, FALSE, NULL);
    if (mutex == NULL) {
        MessageBox(NULL, _T("Mutex creation failed."), _T("Error"), NULL);
        return 1;
    }

    // Создаем поле для ввода
    HWND hEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
        10, 10, 100, 20, hWnd, NULL, hInstance, NULL);

    // Создаем кнопку
    HWND hButton = CreateWindow(_T("BUTTON"), _T("Calculate"), WS_CHILD | WS_VISIBLE,
        120, 10, 80, 20, hWnd, (HMENU)1, hInstance, NULL);

    // Отображение окна
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}