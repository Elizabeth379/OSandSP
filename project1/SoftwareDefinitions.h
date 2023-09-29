#pragma once

#define OnExitSoftware 1
#define OnClearField 2
#define OnReadColor 3
#define OnSaveFile 4
#define OnLoadFile 5

#define DigIndexColorR 200
#define DigIndexColorG 201
#define DigIndexColorB 202


#define TextBufferSize 256

char Buffer[TextBufferSize];
unsigned num;

HWND hStaticControl;
HWND hEditControl;
HWND hNumberControl;

char filename[260];
OPENFILENAMEA ofn;

int colorR, colorG, colorB;
HFONT fontRectangle, fontStatic;
COLORREF fontColor;

HBRUSH brushRectangle;
RECT windowRectangle;
PAINTSTRUCT ps;
HDC hdcStatic;

LRESULT CALLBACK SoftwareMainProcedure(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
WNDCLASS NewWindowClass(HBRUSH BGColor, HCURSOR Cursor, HINSTANCE hInst, HICON Icon, LPCWSTR Name, WNDPROC Procedure);

void MainWndAddMenus(HWND hWnd);
void MainWndAddWidgets(HWND hWnd);
void SetOpenFileParams(HWND hWnd);
void SaveData(LPCSTR path);
void LoadData(LPCSTR path);
