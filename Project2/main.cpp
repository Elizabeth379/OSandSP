#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <compressapi.h>

#pragma comment(lib, "WindowsApp.lib")


const wchar_t g_szClassName[] = L"FileCompressionAppClass";
const int IDC_COMPRESS_BUTTON = 101;
const int IDC_DECOMPRESS_BUTTON = 102;

std::wstring GetOpenFileName(HWND hwnd) {
    // Структура для хранения параметров диалога открытия файла
    OPENFILENAME ofn;
    wchar_t szFileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;      // Буфер для сохранения выбранного пути
    ofn.nMaxFile = sizeof(szFileName) / sizeof(*szFileName);    // Максимальная длина буфера
    ofn.lpstrTitle = L"Select a file";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;  // Файл не добавляется в список недавно открытых файлов (MRU list), должен существовать

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
    ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT; // -//-, Если пользователь выберет существующий, то система сначала спросит, хочет ли он его перезаписать

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

    COMPRESSOR_HANDLE Compressor = NULL;
    PBYTE CompressedBuffer = NULL;
    PBYTE InputBuffer = NULL;
    HANDLE InputFile = INVALID_HANDLE_VALUE;
    HANDLE CompressedFile = INVALID_HANDLE_VALUE;
    BOOL DeleteTargetFile = TRUE;
    BOOL Success;
    SIZE_T CompressedDataSize, CompressedBufferSize;
    DWORD InputFileSize, ByteRead, ByteWritten;
    LARGE_INTEGER FileSize;
    ULONGLONG StartTime, EndTime;
    double TimeDuration;

    //  Открыть входной файл для чтения, только существующий файл.
    InputFile = CreateFile(
        sourcePath.c_str(),       
        GENERIC_READ,             
        FILE_SHARE_READ,         // Разрешено совместное чтение
        NULL,                     //  Атрибуты безопасности, стандартная безопасность
        OPEN_EXISTING,            
        FILE_ATTRIBUTE_NORMAL,    
        NULL);                    //  Атрибуты шаблона не используются

    if (InputFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"Cannot open source file.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    // Получить размер входного файла.
    Success = GetFileSizeEx(InputFile, &FileSize);
    if ((!Success) || (FileSize.QuadPart > 0xFFFFFFFF)) {
        MessageBox(hwnd, L"Cannot get input file size or file is larger than 4GB.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }
    InputFileSize = FileSize.LowPart;

    //  Выделить память для содержимого файла.
    InputBuffer = (PBYTE)malloc(InputFileSize);
    if (!InputBuffer) {
        MessageBox(hwnd, L"Cannot allocate memory for uncompressed buffer.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    //  Прочесть входной файл.
    Success = ReadFile(InputFile, InputBuffer, InputFileSize, &ByteRead, NULL);
    if ((!Success) || (ByteRead != InputFileSize)) {
        MessageBox(hwnd, L"Cannot read from source file.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    // Открыть файл для записи, если существует - перезаписать
    CompressedFile = CreateFile(
        destPath.c_str(),         
        GENERIC_WRITE | DELETE,   //  Открыть для чтения; удалить, если нельзя сжать
        0,                        
        NULL,                    
        CREATE_ALWAYS,            
        FILE_ATTRIBUTE_NORMAL,    
        NULL);                    

    if (CompressedFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"Cannot create compressed file.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    //  Создать XpressHuff compressor.
    Success = CreateCompressor(
        COMPRESS_ALGORITHM_XPRESS_HUFF, //  алгоритм сжатия
        NULL,                           //  параметр для опционального аллокатора памяти
        &Compressor);                   

    if (!Success) {
        MessageBox(hwnd, L"Cannot create a compressor.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    //  Запросить размер сжатого буфера.
    Success = Compress(
        Compressor,                  //  Обработчик компрессора
        InputBuffer,                 //  Буфер ввода, несжатые данные
        InputFileSize,               //  Размер несжатых данных
        NULL,                        //  Буфер сжатых данных
        0,                           //  Размер буфера для сжатых данных
        &CompressedBufferSize);      //  Размер сжатых данных

    //  Выделить память для сжатого буфера.
    if (!Success) {
        DWORD ErrorCode = GetLastError();

        if (ErrorCode != ERROR_INSUFFICIENT_BUFFER) {
            MessageBox(hwnd, L"Cannot compress data.", L"Error", MB_ICONERROR | MB_OK);
            goto done;
        }

        CompressedBuffer = (PBYTE)malloc(CompressedBufferSize);
        if (!CompressedBuffer) {
            MessageBox(hwnd, L"Cannot allocate memory for compressed buffer.", L"Error", MB_ICONERROR | MB_OK);
            goto done;
        }
    }

    StartTime = GetTickCount64();

    //  Вызовите Compress() еще раз для выполнения реального сжатия и вывода сжатых 
    //  данных в CompressedBuffer.
    Success = Compress(
        Compressor,               //  Обработчик компрессора
        InputBuffer,              //  Буфер ввода, несжатые данные
        InputFileSize,            //  Размер несжатых данных
        CompressedBuffer,         //  Буфер сжатых данных
        CompressedBufferSize,     //  Размер буфера для сжатых данных
        &CompressedDataSize);     //  Размер сжатых данных

    if (!Success) {
        MessageBox(hwnd, L"Cannot compress data.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    EndTime = GetTickCount64();

    //  Получить время сжатия.
    TimeDuration = (EndTime - StartTime) / 1000.0;

    //  Записать сжатые данные в выходной файл.
    Success = WriteFile(
        CompressedFile,     //  Дескриптор файла
        CompressedBuffer,   //  Начало данных для записи
        CompressedDataSize, //  Количество байт для записи
        &ByteWritten,       //  Количество записанных байт
        NULL);              //  Нет структуры перекрытия

    if ((ByteWritten != CompressedDataSize) || (!Success)) {
        MessageBox(hwnd, L"Cannot write compressed data to file.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    wchar_t infoMessage[256];
    swprintf_s(infoMessage, L"Input file size: %d; Compressed Size: %d\n", InputFileSize, CompressedDataSize);
    MessageBox(hwnd, infoMessage, L"Compression Info", MB_OK | MB_ICONINFORMATION);
    swprintf_s(infoMessage, L"Compression Time (Exclude I/O): %.2f seconds\n", TimeDuration);
    MessageBox(hwnd, infoMessage, L"Compression Time", MB_OK | MB_ICONINFORMATION);
    MessageBox(hwnd, L"File Compressed.", L"Success", MB_OK | MB_ICONINFORMATION);

    DeleteTargetFile = FALSE;

done:
    if (Compressor != NULL) {
        CloseCompressor(Compressor);
    }

    if (CompressedBuffer) {
        free(CompressedBuffer);
    }

    if (InputBuffer) {
        free(InputBuffer);
    }

    if (InputFile != INVALID_HANDLE_VALUE) {
        CloseHandle(InputFile);
    }

    if (CompressedFile != INVALID_HANDLE_VALUE) {
        if (DeleteTargetFile) {         //  Сжатие не удалось, удалить сжатый файл.
            FILE_DISPOSITION_INFO fdi;
            fdi.DeleteFile = TRUE;  //  Пометка для удаления
            Success = SetFileInformationByHandle( //Устанавливаем информацию о том, что файл должен быть удален
                CompressedFile,
                FileDispositionInfo,
                &fdi,
                sizeof(FILE_DISPOSITION_INFO));
            if (!Success) {
                MessageBox(hwnd, L"Cannot delete corrupted compressed file.", L"Error", MB_ICONERROR | MB_OK); // Выводит сообщение об ошибке, если не удалось установить информацию о файле
            }
        }
        CloseHandle(CompressedFile);
    }
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

    DECOMPRESSOR_HANDLE Decompressor = NULL;
    PBYTE CompressedBuffer = NULL;
    PBYTE DecompressedBuffer = NULL;
    HANDLE InputFile = INVALID_HANDLE_VALUE;
    HANDLE DecompressedFile = INVALID_HANDLE_VALUE;
    BOOL DeleteTargetFile = TRUE;
    BOOL Success;
    SIZE_T DecompressedBufferSize, DecompressedDataSize;
    DWORD InputFileSize, ByteRead, ByteWritten;
    ULONGLONG StartTime, EndTime;
    LARGE_INTEGER FileSize;
    double TimeDuration;


    //  Открыть для чтения только существующий файл
    InputFile = CreateFile(
        sourcePath.c_str(),                  
        GENERIC_READ,            
        FILE_SHARE_READ,         
        NULL,                     
        OPEN_EXISTING,            
        FILE_ATTRIBUTE_NORMAL,    
        NULL);                   

    if (InputFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"Cannot open source file!", L"Error", MB_ICONERROR);
        goto done;
    }

    // Получить размер сжатого файла.
    Success = GetFileSizeEx(InputFile, &FileSize);
    if ((!Success) || (FileSize.QuadPart > 0xFFFFFFFF))
    {
        MessageBox(hwnd, L"Cannot get file size!", L"Error", MB_ICONERROR);
        goto done;
    }
    InputFileSize = FileSize.LowPart;

    //  Выделение памяти под сжатые данные.
    CompressedBuffer = (PBYTE)malloc(InputFileSize);
    if (!CompressedBuffer)
    {
        MessageBox(hwnd, L"Memory allocation error!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Прочитать сжатые данные в буфер.
    Success = ReadFile(InputFile, CompressedBuffer, InputFileSize, &ByteRead, NULL);
    if ((!Success) || (ByteRead != InputFileSize))
    {
        MessageBox(hwnd, L"Cannot read from source file!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Открыть пустой файл для записи, если существует - перезапис
    DecompressedFile = CreateFile(
        destPath.c_str(),                  
        GENERIC_WRITE | DELETE,     
        0,                        
        NULL,                     
        CREATE_ALWAYS,            
        FILE_ATTRIBUTE_NORMAL,    
        NULL);                   

    if (DecompressedFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(hwnd, L"Cannot create destination file!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Создать XpressHuff распаковщик.
    Success = CreateDecompressor(
        COMPRESS_ALGORITHM_XPRESS_HUFF, //  алгоритм сжатия
        NULL,                           //  параметр для опционального аллокатора памяти
        &Decompressor);                 

    if (!Success)
    {
        MessageBox(hwnd, L"Cannot create decompressor!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Запросить размер распаковочного буфера.
    Success = Decompress(
        Decompressor,                //  дескриптор компрессора
        CompressedBuffer,            
        InputFileSize,               
        NULL,                        //  Буфер установлен в NULL
        0,                           //  Buffer size в 0
        &DecompressedBufferSize);    //  Размер данных распаковки

    // Выделение памяти для распаковочного буфера.
    if (!Success)
    {
        DWORD ErrorCode = GetLastError();

        // Первоначальный размер, возвращаемый функцией,
        // извлекается из самого буфера и должен рассматриваться как ненадежный и проверяться
        // на разумные пределы.
        if (ErrorCode != ERROR_INSUFFICIENT_BUFFER)
        {
            MessageBox(hwnd, L"Cannot decompress data", L"Error", MB_ICONERROR);
            goto done;
        }

        DecompressedBuffer = (PBYTE)malloc(DecompressedBufferSize);
        if (!DecompressedBuffer)
        {
            MessageBox(hwnd, L"Cannot allocate memory for decompressed buffer.", L"Error", MB_ICONERROR);
            
            goto done;
        }
    }

    StartTime = GetTickCount64();

    //  Распаковка данных и их запись в DecompressedBuffer.
    Success = Decompress(
        Decompressor,               //  дескриптор распаковщика
        CompressedBuffer,           //  сжатые данные
        InputFileSize,              //  размер сжатых данных
        DecompressedBuffer,         //  буфер распаковки
        DecompressedBufferSize,     //  размер буфера распаковки
        &DecompressedDataSize);     //  размер распакованных данных

    if (!Success)
    {
        MessageBox(hwnd, L"Cannot decompress data 2", L"Error", MB_ICONERROR);
        goto done;
    }

    EndTime = GetTickCount64();

    //  Получение времени распаковки.
    TimeDuration = (EndTime - StartTime) / 1000.0;

    //  Записать распакованные данные в выходной файл.
    Success = WriteFile(
        DecompressedFile,       
        DecompressedBuffer,     //  Начало данных для записи
        DecompressedDataSize,   //  Количество байт для записи
        &ByteWritten,           //  КОличество записанных байт
        NULL);                  // Нет структуры перекрытия
    if ((ByteWritten != DecompressedDataSize) || (!Success))
    {
        MessageBox(hwnd, L"Cannot write to destination file!", L"Error", MB_ICONERROR);

    }

    wchar_t infoMessage[256];
    swprintf_s(infoMessage, L"Compressed size: %d; Decompressed Size: %d\n", InputFileSize, DecompressedDataSize);
    MessageBox(hwnd, infoMessage, L"Compression Info", MB_OK | MB_ICONINFORMATION);
    swprintf_s(infoMessage, L"Decompression Time(Exclude I/O): %.2f seconds\n", TimeDuration);
    MessageBox(hwnd, infoMessage, L"Compression Time", MB_OK | MB_ICONINFORMATION);
    MessageBox(hwnd, L"File decompressed.", L"Success", MB_OK | MB_ICONINFORMATION);


    DeleteTargetFile = FALSE;

done:
    if (Decompressor != NULL)
    {
        CloseDecompressor(Decompressor);
    }

    if (CompressedBuffer)
    {
        free(CompressedBuffer);
    }

    if (DecompressedBuffer)
    {
        free(DecompressedBuffer);
    }

    if (InputFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(InputFile);
    }

    if (DecompressedFile != INVALID_HANDLE_VALUE)
    {
        //  Распаковка не удалась, удалить файл
        if (DeleteTargetFile)
        {
            FILE_DISPOSITION_INFO fdi;
            fdi.DeleteFile = TRUE;      //  Пометка для удаления
            Success = SetFileInformationByHandle(
                DecompressedFile,
                FileDispositionInfo,
                &fdi,
                sizeof(FILE_DISPOSITION_INFO));
            if (!Success) {
                MessageBox(hwnd, L"Cannot delete corrupted decompressed file.", L"Error", MB_ICONERROR | MB_OK);

            }
        }
        CloseHandle(DecompressedFile);
    }

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
