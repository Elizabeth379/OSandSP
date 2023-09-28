#include <Windows.h>
#include <string>
#include "SoftwareDefinitions.h"
#include "resource.h"



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {

	WNDCLASS SoftwareMainClass = NewWindowClass((HBRUSH)COLOR_WINDOW, LoadCursor(NULL, IDC_HAND), hInst,
		LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"MainWndClass", SoftwareMainProcedure);

	if (!RegisterClassW(&SoftwareMainClass)) { return -1; }
	MSG SoftwareMainMessage = { 0 };

	CreateWindow(L"MainWndClass", L"Текстовый редактор", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 250, NULL, NULL, NULL, NULL);
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
		case OnMenuAction1:
			MessageBoxA(hWnd, "Menu 1 was clicked!", "Menu worked", MB_OK);
			break;
		case OnClearField:
			SetWindowTextA(hEditControl, "");
			break;
		case OnReadColor:

			brushRectangle = CreateSolidBrush(
			RGB(GetDlgItemInt(hWnd, DigIndexColorR, FALSE, false),
				GetDlgItemInt(hWnd, DigIndexColorG, FALSE, false),
				GetDlgItemInt(hWnd, DigIndexColorB, FALSE, false)));

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

		FillRect(ps.hdc, &windowRectangle, brushRectangle);

		EndPaint(hWnd, &ps);
		break;
	case WM_CREATE:
		MainWndAddMenus(hWnd);
		MainWndAddWidgets(hWnd);
		SetOpenFileParams(hWnd);
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
	AppendMenu(RootMenu, MF_STRING, (UINT_PTR)SubMenu, L"Help");

	SetMenu(hWnd, RootMenu);
}

void MainWndAddWidgets(HWND hWnd) {

	hStaticControl = CreateWindowA("static", "STATUS: Hello Wind!", WS_VISIBLE | WS_CHILD | ES_CENTER, 250, 5, 100, 30, hWnd, NULL, NULL, NULL);
	//hEditControl = CreateWindowA("edit", "This is EDIT control!", WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL, 5, 40, 480, 100, hWnd, NULL, NULL, NULL);
	windowRectangle = { 5 + 480, 40, 5, 40 + 100 };

	CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 5, 170, 120, 30, hWnd, (HMENU)DigIndexColorR, NULL, NULL);
	CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 110, 170, 120, 30, hWnd, (HMENU)DigIndexColorG, NULL, NULL);
	CreateWindowA("edit", "0", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 215, 170, 120, 30, hWnd, (HMENU)DigIndexColorB, NULL, NULL);


	CreateWindowA("button", "Clear", WS_VISIBLE | WS_CHILD | ES_CENTER, 5, 5, 120, 30, hWnd, (HMENU)OnClearField, NULL, NULL);
	CreateWindowA("button", "Set Color", WS_VISIBLE | WS_CHILD | ES_CENTER, 130, 5, 100, 30, hWnd, (HMENU)OnReadColor, NULL, NULL);

}

void SaveData(LPCSTR path) {
	HANDLE FileToSave = CreateFileA(
		path,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	int saveLenth = GetWindowTextLength(hEditControl) + 1;
	char* data = new char[saveLenth];

	saveLenth = GetWindowTextA(hEditControl, data, saveLenth);

	DWORD bytesIterated;
	WriteFile(FileToSave, data, saveLenth, &bytesIterated, NULL);

	CloseHandle(FileToSave);
	delete[] data;
}

void LoadData(LPCSTR path) {
	HANDLE FileToLoad = CreateFileA(
		path,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	DWORD bytesIterated;
	ReadFile(FileToLoad, Buffer, TextBufferSize, &bytesIterated, NULL);

	SetWindowTextA(hEditControl, Buffer);

	CloseHandle(FileToLoad);
}

void SetOpenFileParams(HWND hWnd) {
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrFilter = "*.txt";
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = "C:/Users/Acer/Desktop/films/osisp-assets/files";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}