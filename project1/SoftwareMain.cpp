#include <Windows.h>
#include "SoftwareDefinitions.h"



int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {

	WNDCLASS SoftwareMainClass = NewWindowClass((HBRUSH)COLOR_WINDOW, LoadCursor(NULL, IDC_HAND), hInst, LoadIcon(NULL, IDI_QUESTION),
		L"MainWndClass", SoftwareMainProcedure);

	if (!RegisterClassW(&SoftwareMainClass)) { return -1; }
	MSG SoftwareMainMessage = { 0 };

	CreateWindow(L"MainWndClass", L"First c++ window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 500, 250, NULL, NULL, NULL, NULL);
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
		case OnMenuAction2:
			MessageBoxA(hWnd, "Menu 2 was clicked!", "Menu worked", MB_OK);
			break;
		case OnMenuAction3:
			MessageBoxA(hWnd, "Menu 3 was clicked!", "Menu worked", MB_OK);
			break;
		case OnClearField:
			SetWindowTextA(hEditControl, "");
			break;
		case OnExitSoftware:
			PostQuitMessage(0);
			break;
		default:
			break;

		}
		break;

	case WM_CREATE:
		MainWndAddMenus(hWnd);
		MainWndAddWidgets(hWnd);
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
	AppendMenu(SubMenu, MF_STRING, OnExitSoftware, L"Exit");

	AppendMenu(RootMenu, MF_POPUP, (UINT_PTR)SubMenu, L"File");
	AppendMenu(RootMenu, MF_STRING, (UINT_PTR)SubMenu, L"Help");

	SetMenu(hWnd, RootMenu);
}

void MainWndAddWidgets(HWND hWnd) {
	CreateWindowA("static", "STATUS: Hello Wind!", WS_VISIBLE | WS_CHILD | ES_CENTER, 140, 5, 350, 30, hWnd, NULL, NULL, NULL);

	hEditControl = CreateWindowA("edit", "This is EDIT control!", WS_VISIBLE | WS_CHILD | ES_MULTILINE | WS_VSCROLL, 5, 40, 480, 130, hWnd, NULL, NULL, NULL);

	CreateWindowA("button", "Clear", WS_VISIBLE | WS_CHILD | ES_CENTER, 5, 5, 120, 30, hWnd, (HMENU)OnClearField, NULL, NULL);

}