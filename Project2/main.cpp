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

    //  Open input file for reading, existing file only.
    InputFile = CreateFile(
        sourcePath.c_str(),       //  Input file name
        GENERIC_READ,             //  Open for reading
        FILE_SHARE_READ,          //  Share for read
        NULL,                     //  Default security
        OPEN_EXISTING,            //  Existing file only
        FILE_ATTRIBUTE_NORMAL,    //  Normal file
        NULL);                    //  No attr. template

    if (InputFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"Cannot open source file.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    //  Get input file size.
    Success = GetFileSizeEx(InputFile, &FileSize);
    if ((!Success) || (FileSize.QuadPart > 0xFFFFFFFF)) {
        MessageBox(hwnd, L"Cannot get input file size or file is larger than 4GB.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }
    InputFileSize = FileSize.LowPart;

    //  Allocate memory for file content.
    InputBuffer = (PBYTE)malloc(InputFileSize);
    if (!InputBuffer) {
        MessageBox(hwnd, L"Cannot allocate memory for uncompressed buffer.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    //  Read input file.
    Success = ReadFile(InputFile, InputBuffer, InputFileSize, &ByteRead, NULL);
    if ((!Success) || (ByteRead != InputFileSize)) {
        MessageBox(hwnd, L"Cannot read from source file.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    //  Open an empty file for writing, if exist, overwrite it.
    CompressedFile = CreateFile(
        destPath.c_str(),         //  Compressed file name
        GENERIC_WRITE | DELETE,   //  Open for writing; delete if cannot compress
        0,                        //  Do not share
        NULL,                     //  Default security
        CREATE_ALWAYS,            //  Create a new file; if exist, overwrite it
        FILE_ATTRIBUTE_NORMAL,    //  Normal file
        NULL);                    //  No template

    if (CompressedFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"Cannot create compressed file.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    //  Create an XpressHuff compressor.
    Success = CreateCompressor(
        COMPRESS_ALGORITHM_XPRESS_HUFF, //  Compression Algorithm
        NULL,                           //  Optional allocation routine
        &Compressor);                   //  Handle

    if (!Success) {
        MessageBox(hwnd, L"Cannot create a compressor.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    //  Query compressed buffer size.
    Success = Compress(
        Compressor,                  //  Compressor Handle
        InputBuffer,                 //  Input buffer, Uncompressed data
        InputFileSize,               //  Uncompressed data size
        NULL,                        //  Compressed Buffer
        0,                           //  Compressed Buffer size
        &CompressedBufferSize);      //  Compressed Data size

    //  Allocate memory for compressed buffer.
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

    //  Call Compress() again to do real compression and output the compressed
    //  data to CompressedBuffer.
    Success = Compress(
        Compressor,               //  Compressor Handle
        InputBuffer,              //  Input buffer, Uncompressed data
        InputFileSize,            //  Uncompressed data size
        CompressedBuffer,         //  Compressed Buffer
        CompressedBufferSize,     //  Compressed Buffer size
        &CompressedDataSize);     //  Compressed Data size

    if (!Success) {
        MessageBox(hwnd, L"Cannot compress data.", L"Error", MB_ICONERROR | MB_OK);
        goto done;
    }

    EndTime = GetTickCount64();

    //  Get compression time.
    TimeDuration = (EndTime - StartTime) / 1000.0;

    //  Write compressed data to output file.
    Success = WriteFile(
        CompressedFile,     //  File handle
        CompressedBuffer,   //  Start of data to write
        CompressedDataSize, //  Number of byte to write
        &ByteWritten,       //  Number of byte written
        NULL);              //  No overlapping structure

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
        //  Compression fails, delete the compressed file.
        if (DeleteTargetFile) {
            FILE_DISPOSITION_INFO fdi;
            fdi.DeleteFile = TRUE;  //  Marking for deletion
            Success = SetFileInformationByHandle(
                CompressedFile,
                FileDispositionInfo,
                &fdi,
                sizeof(FILE_DISPOSITION_INFO));
            if (!Success) {
                MessageBox(hwnd, L"Cannot delete corrupted compressed file.", L"Error", MB_ICONERROR | MB_OK);
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


    //  Open input file for reading, existing file only.
    InputFile = CreateFile(
        sourcePath.c_str(),                  //  Input file name, compressed file
        GENERIC_READ,             //  Open for reading
        FILE_SHARE_READ,          //  Share for read
        NULL,                     //  Default security
        OPEN_EXISTING,            //  Existing file only
        FILE_ATTRIBUTE_NORMAL,    //  Normal file
        NULL);                    //  No template

    if (InputFile == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, L"Cannot open source file!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Get compressed file size.
    Success = GetFileSizeEx(InputFile, &FileSize);
    if ((!Success) || (FileSize.QuadPart > 0xFFFFFFFF))
    {
        MessageBox(hwnd, L"Cannot get file size!", L"Error", MB_ICONERROR);
        goto done;
    }
    InputFileSize = FileSize.LowPart;

    //  Allocation memory for compressed content.
    CompressedBuffer = (PBYTE)malloc(InputFileSize);
    if (!CompressedBuffer)
    {
        MessageBox(hwnd, L"Memory allocation error!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Read compressed content into buffer.
    Success = ReadFile(InputFile, CompressedBuffer, InputFileSize, &ByteRead, NULL);
    if ((!Success) || (ByteRead != InputFileSize))
    {
        MessageBox(hwnd, L"Cannot read from source file!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Open an empty file for writing, if exist, destroy it.
    DecompressedFile = CreateFile(
        destPath.c_str(),                  //  Decompressed file name
        GENERIC_WRITE | DELETE,     //  Open for writing
        0,                        //  Do not share
        NULL,                     //  Default security
        CREATE_ALWAYS,            //  Create a new file, if exists, overwrite it.
        FILE_ATTRIBUTE_NORMAL,    //  Normal file
        NULL);                    //  No template

    if (DecompressedFile == INVALID_HANDLE_VALUE)
    {
        MessageBox(hwnd, L"Cannot create destination file!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Create an XpressHuff decompressor.
    Success = CreateDecompressor(
        COMPRESS_ALGORITHM_XPRESS_HUFF, //  Compression Algorithm
        NULL,                           //  Optional allocation routine
        &Decompressor);                 //  Handle

    if (!Success)
    {
        MessageBox(hwnd, L"Cannot create decompressor!", L"Error", MB_ICONERROR);
        goto done;
    }

    //  Query decompressed buffer size.
    Success = Decompress(
        Decompressor,                //  Compressor Handle
        CompressedBuffer,            //  Compressed data
        InputFileSize,               //  Compressed data size
        NULL,                        //  Buffer set to NULL
        0,                           //  Buffer size set to 0
        &DecompressedBufferSize);    //  Decompressed Data size

    //  Allocate memory for decompressed buffer.
    if (!Success)
    {
        DWORD ErrorCode = GetLastError();

        // Note that the original size returned by the function is extracted 
        // from the buffer itself and should be treated as untrusted and tested
        // against reasonable limits.
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

    //  Decompress data and write data to DecompressedBuffer.
    Success = Decompress(
        Decompressor,               //  Decompressor handle
        CompressedBuffer,           //  Compressed data
        InputFileSize,              //  Compressed data size
        DecompressedBuffer,         //  Decompressed buffer
        DecompressedBufferSize,     //  Decompressed buffer size
        &DecompressedDataSize);     //  Decompressed data size

    if (!Success)
    {
        MessageBox(hwnd, L"Cannot decompress data 2", L"Error", MB_ICONERROR);
        goto done;
    }

    EndTime = GetTickCount64();

    //  Get decompression time.
    TimeDuration = (EndTime - StartTime) / 1000.0;

    //  Write decompressed data to output file.
    Success = WriteFile(
        DecompressedFile,       //  File handle
        DecompressedBuffer,     //  Start of data to write
        DecompressedDataSize,   //  Number of byte to write
        &ByteWritten,           //  Number of byte written
        NULL);                  //  No overlapping structure
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
        //  Compression fails, delete the compressed file.
        if (DeleteTargetFile)
        {
            FILE_DISPOSITION_INFO fdi;
            fdi.DeleteFile = TRUE;      //  Marking for deletion
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
