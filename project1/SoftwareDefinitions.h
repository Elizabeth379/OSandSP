#pragma once

#define OnMenuAction1  1
#define OnMenuAction2  2
#define OnMenuAction3  3
#define OnExitSoftware 4
#define OnClearField 5

HWND hEditControl;


LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure);

void MainWndAddMenus(HWND hWnd);
void MainWndAddWidgets(HWND hWnd);
