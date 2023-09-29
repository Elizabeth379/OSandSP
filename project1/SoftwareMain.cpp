#include <Windows.h>
#include <string>
#include "SoftwareColors.h"
#include "SoftwareDefinitions.h"
#include "resource.h"



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {

	fontRectangle = CreateFontA(
		60, 20, 0, 0, FW_MEDIUM,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		FF_DECORATIVE, "SpecialFont"
	);

	fontStatic = CreateFontA(
		30, 10, 0, 0, FW_MEDIUM,
		FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		FF_DECORATIVE, "SpecialStaticFont"
	);

	// Создание класса окна WNDCLASS и задание его параметров
	WNDCLASS SoftwareMainClass = NewWindowClass((HBRUSH)COLOR_WINDOW, LoadCursor(NULL, IDC_HAND), hInst,
		LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"MainWndClass", SoftwareMainProcedure);

	// Регистрация класса окна
	if (!RegisterClassW(&SoftwareMainClass)) { return -1; }
	MSG SoftwareMainMessage = { 0 };

	// параметр WS_OVERLAPPEDWINDOW добавляет кнопки размер, свернуть, закрыть на окно + перемещение окна + стандартные бордюр и рамка + системное меню
	CreateWindow(L"MainWndClass", L"Text editor for kids", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 500, NULL, NULL, NULL, NULL);
	while (GetMessage(&SoftwareMainMessage, NULL, NULL, NULL))
	{
		TranslateMessage(&SoftwareMainMessage);
		DispatchMessage(&SoftwareMainMessage);
	}

	return 0;

}


WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure) {

	WNDCLASS NWC = { 0 };

	NWC.hCursor = Cursor;
	NWC.hIcon = Icon;
	NWC.hInstance = hInst;
	NWC.lpszClassName = Name;
	NWC.hbrBackground = BGColor;
	NWC.lpfnWndProc = Procedure;

	return NWC;

}

LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg)
	{
	case WM_COMMAND:
		switch (wp) {
		case OnClearField:
			SetWindowTextA(hEditControl, "");
			break;
		case OnReadColor:

			colorR = GetDlgItemInt(hWnd, DigIndexColorR, FALSE, false);
			colorG = GetDlgItemInt(hWnd, DigIndexColorG, FALSE, false);
			colorB = GetDlgItemInt(hWnd, DigIndexColorB, FALSE, false);


			//brushRectangle = CreateSolidBrush(RGB(colorR, colorG, colorB));

			fontColor = RGB(colorR, colorG, colorB);

			RedrawWindow(hWnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);

			break;
		case OnSaveFile:
			if (GetSaveFileNameA(&ofn)) { SaveData(filename); }
			break;
		case OnLoadFile:
			if (GetOpenFileNameA(&ofn)) { LoadData(filename); }
			break;
		case OnExitSoftware:
			PostQuitMessage(0);
			break;
		default:
			break;

		}
		break;

	case WM_PAINT:

		BeginPaint(hWnd, &ps);

		//FillRect(ps.hdc, &windowRectangle, brushRectangle);
		GradientRect(ps.hdc, &windowRectangle, Color(0,148,153), Color(0,184,74));

		SetBkMode(ps.hdc, TRANSPARENT);
		SetTextColor(ps.hdc, fontColor);
		SelectObject(ps.hdc, fontRectangle);
		// DT_NOCLIP - если текст не влезет в прямоугольник, то выйдет за его пределы и будет виден
		DrawTextA(ps.hdc, "Gradient text", 15, &windowRectangle, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOCLIP);

		EndPaint(hWnd, &ps);
		break;
	case WM_CREATE:
		MainWndAddMenus(hWnd);
		MainWndAddWidgets(hWnd);
		SetOpenFileParams(hWnd);

		SendMessageA(hStaticControl, WM_SETFONT, (WPARAM)fontStatic, TRUE);
		break;
	//case WM_CTLCOLORSTATIC:
	//{
	//	HDC hdcStatic = (HDC)wp;
	//	SetBKColor(hdcStatic, RGB(0, 255, 0));
	//	return (INT_PTR)CreateSolidBrush(RGB((0, 255, 0));
	//}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default: return DefWindowProc(hWnd, msg, wp, lp);
		
	}
}

void MainWndAddMenus(HWND hWnd) {
	HMENU RootMenu = CreateMenu();

	HMENU SubMenu = CreateMenu();
	HMENU SubActionMenu = CreateMenu();

	
	AppendMenu(SubMenu, MF_STRING, OnClearField, L"Clear");
	AppendMenu(SubMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(SubMenu, MF_STRING, OnSaveFile, L"Save");
	AppendMenu(SubMenu, MF_STRING, OnLoadFile, L"Load");
	AppendMenu(SubMenu, MF_SEPARATOR, NULL, NULL);
	AppendMenu(SubMenu, MF_STRING, OnExitSoftware, L"Exit");

	AppendMenu(RootMenu, MF_POPUP, (UINT_PTR)SubMenu, L"File");

	SetMenu(hWnd, RootMenu);
}

void MainWndAddWidgets(HWND hWnd) {

	// WS_VISIBLE - элемент видим по дефолту, WS_CHILD - элемент является дочерним от базового окна(наодится внутри этого окна)
	hStaticControl = CreateWindowA("static", "Hello, Wind!", WS_VISIBLE | WS_CHILD | ES_CENTER, 275, 5, 100, 30, hWnd, NULL, NULL, NULL);
	windowRectangle = { 5 + 480, 70, 5, 110 };
	hEditControl = CreateWindowA("edit", "Write your text here", WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL, 5, 115, 480, 300, hWnd, NULL, NULL, NULL);


	CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 5, 40, 80, 20, hWnd, (HMENU)DigIndexColorR, NULL, NULL);
	CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 110, 40, 80, 20, hWnd, (HMENU)DigIndexColorG, NULL, NULL);
	CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 215, 40, 80, 20, hWnd, (HMENU)DigIndexColorB, NULL, NULL);


	CreateWindowA("button", "Clear", WS_VISIBLE | WS_CHILD | ES_CENTER, 5, 5, 80, 30, hWnd, (HMENU)OnClearField, NULL, NULL);
	CreateWindowA("button", "Set gradient text color", WS_VISIBLE | WS_CHILD | ES_CENTER, 95, 5, 170, 30, hWnd, (HMENU)OnReadColor, NULL, NULL);

}

void SaveData(LPCSTR path) {

	HANDLE FileToSave = CreateFileA(
		path,                                   // Путь к файлу
		GENERIC_WRITE,                          // Режим доступа (запись)
		0,                                      // Флаги и атрибуты файла
		NULL,                                   // Дескриптор безопасности (не используется)
		CREATE_ALWAYS,                          // Действие при создании: создать новый файл или перезаписать существующий
		FILE_ATTRIBUTE_NORMAL,                  // Атрибуты файла (обычный файл)
		NULL                                    // Шаблон файла (не используется)
	);

	int saveLenth = GetWindowTextLength(hEditControl) + 1;

	// буфер для хранения текста
	char* data = new char[saveLenth];

	// Получен текст из элемента управления и сохранен в буфере
	saveLenth = GetWindowTextA(hEditControl, data, saveLenth);

	// Переменная для отслеживания количества записанных байтов
	DWORD bytesIterated;

	// Записываем содержимое буфера data в файл
	WriteFile(FileToSave, data, saveLenth, &bytesIterated, NULL);

	// Закрываем дескриптор файла
	CloseHandle(FileToSave);

	delete[] data;
}


void LoadData(LPCSTR path) {
	HANDLE FileToLoad = CreateFileA(
		path,
		GENERIC_READ,			// Режим доступа (чтение)
		0,
		NULL,
		OPEN_EXISTING,            // Открыть существующий
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	DWORD bytesIterated;
	ReadFile(FileToLoad, Buffer, TextBufferSize, &bytesIterated, NULL);

	SetWindowTextA(hEditControl, Buffer);  // Считываем в буфер определенное кол-во символов и выводим их в поле edit

	CloseHandle(FileToLoad);
}

void SetOpenFileParams(HWND hWnd) {
	// Обнуляем структуру ofn (OPENFILENAME) перед использованием
	ZeroMemory(&ofn, sizeof(ofn));
	// Устанавливаем размер структуры OPENFILENAME
	ofn.lStructSize = sizeof(ofn);
	// Устанавливаем окно-владелец для диалогового окна выбора файла
	ofn.hwndOwner = hWnd;
	// Устанавливаем указатель на строку, в которой будет сохранено имя выбранного файла
	ofn.lpstrFile = filename;
	// Устанавливаем максимальную длину имени файла, которую может принять указатель lpstrFile
	ofn.nMaxFile = sizeof(filename);
	// Устанавливаем фильтр файлов для диалогового окна (только .txt файлы)
	ofn.lpstrFilter = "*.txt";
	// Устанавливаем указатель на строку, в которой будет сохранено имя выбранного файла без расширения
	ofn.lpstrFileTitle = NULL;
	// Устанавливаем максимальную длину имени файла без расширения
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = "C:/Users/Acer/Desktop/films/osisp-assets/files";
	// Устанавливаем флаги для диалогового окна выбора файла (директория должна существовать и файл должен существовать)
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}
