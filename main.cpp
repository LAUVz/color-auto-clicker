#include <windows.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <atomic>
#include <gdiplus.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

enum DetectionMode {
    MODE_TARGET_COLOR,
    MODE_COLOR_CHANGE
};

namespace Theme {
    const COLORREF BG_DARK = RGB(18, 18, 28);
    const COLORREF BG_CARD = RGB(28, 28, 38);
    const COLORREF BG_CARD_HOVER = RGB(35, 35, 48);
    const COLORREF ACCENT_START = RGB(124, 58, 237);
    const COLORREF ACCENT_END = RGB(59, 130, 246);
    const COLORREF ACCENT_GREEN = RGB(34, 197, 94);
    const COLORREF ACCENT_RED = RGB(239, 68, 68);
    const COLORREF TEXT_PRIMARY = RGB(248, 250, 252);
    const COLORREF TEXT_SECONDARY = RGB(148, 163, 184);
    const COLORREF BORDER = RGB(51, 65, 85);
}

struct Hotkeys {
    int pickColor = VK_NUMPAD8;
    int toggle = VK_NUMPAD2;
    int quit = VK_NUMPAD0;
};
COLORREF targetColor = RGB(0, 0, 0);
COLORREF lastColor = RGB(0, 0, 0);
bool hasTargetColor = false;
std::atomic<bool> detecting(false);
std::thread* detectionThread = nullptr;
DetectionMode currentMode = MODE_TARGET_COLOR;
GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
bool pickingColor = false;
Hotkeys hotkeys;
HWND hwndMain = NULL;
HWND hwndRadioTarget = NULL;
HWND hwndRadioChange = NULL;
HWND hwndBtnPickColor = NULL;
HWND hwndBtnStart = NULL;
HWND hwndBtnStop = NULL;
bool btnPickHover = false;
bool btnStartHover = false;
bool btnStopHover = false;
bool radioTargetHover = false;
bool radioChangeHover = false;

#define ID_RADIO_TARGET 101
#define ID_RADIO_CHANGE 102
#define ID_BTN_PICK 103
#define ID_BTN_START 104
#define ID_BTN_STOP 105

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void PickColor();
void StartDetection();
void StopDetection();
void DetectColor();
void DetectColorChange();
void UpdateGUI();
std::wstring ColorToString(COLORREF color);
std::wstring GetKeyName(int vkCode);
HFONT CreateCustomFont(int size, int weight);
void DrawGradientButton(HDC hdc, RECT rect, const wchar_t* text, bool enabled, bool hover, bool isStart);
void DrawModernRadio(HDC hdc, RECT rect, const wchar_t* text, bool selected, bool hover, bool enabled);
void DrawCard(HDC hdc, RECT rect);
void DrawColorPreview(HDC hdc, RECT rect, COLORREF color, const wchar_t* label);
void DrawGradientRect(HDC hdc, RECT rect, COLORREF color1, COLORREF color2, bool horizontal);
void DrawFieldset(HDC hdc, RECT rect, const wchar_t* title, HFONT hFont);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = CreateSolidBrush(Theme::BG_DARK);
    wc.lpszClassName = L"ColorAutoClickerClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 400;
    int windowHeight = 420;
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    hwndMain = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_COMPOSITED,
        L"ColorAutoClickerClass",
        L"Color Auto Clicker",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hwndMain == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwndMain, nCmdShow);
    UpdateWindow(hwndMain);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (detectionThread && detectionThread->joinable()) {
        detecting = false;
        detectionThread->join();
        delete detectionThread;
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            hwndRadioTarget = CreateWindowEx(0, L"BUTTON", L"Target Color",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                20, 85, 166, 35,
                hwnd, (HMENU)ID_RADIO_TARGET, NULL, NULL);

            hwndRadioChange = CreateWindowEx(0, L"BUTTON", L"Color Change",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                196, 85, 166, 35,
                hwnd, (HMENU)ID_RADIO_CHANGE, NULL, NULL);

            hwndBtnPickColor = CreateWindowEx(0, L"BUTTON", L"",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                20, 205, 342, 35,
                hwnd, (HMENU)ID_BTN_PICK, NULL, NULL);

            hwndBtnStart = CreateWindowEx(0, L"BUTTON", L"",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                10, 335, 362, 35,
                hwnd, (HMENU)ID_BTN_START, NULL, NULL);

            hwndBtnStop = CreateWindowEx(0, L"BUTTON", L"",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                0, 0, 0, 0,
                hwnd, (HMENU)ID_BTN_STOP, NULL, NULL);

            RegisterHotKey(hwnd, 1, MOD_NOREPEAT, hotkeys.pickColor);
            RegisterHotKey(hwnd, 2, MOD_NOREPEAT, hotkeys.toggle);
            RegisterHotKey(hwnd, 3, MOD_NOREPEAT, hotkeys.quit);

            SetTimer(hwnd, 1, 50, NULL);

            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
            HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

            DrawGradientRect(hdcMem, clientRect, RGB(18, 18, 28), RGB(28, 18, 38), false);

            HFONT hFontTitle = CreateCustomFont(22, FW_BOLD);
            HFONT hFontSubtitle = CreateCustomFont(14, FW_NORMAL);
            HFONT hFontLabel = CreateCustomFont(12, FW_SEMIBOLD);
            HFONT hFontOld = (HFONT)SelectObject(hdcMem, hFontTitle);

            SetBkMode(hdcMem, TRANSPARENT);
            SetTextColor(hdcMem, Theme::TEXT_PRIMARY);
            RECT titleRect = {30, 20, 280, 50};
            DrawText(hdcMem, L"Color Auto Clicker", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdcMem, hFontLabel);
            RECT statusBadge = {270, 25, 340, 45};
            HBRUSH statusBrush = CreateSolidBrush(detecting ? Theme::ACCENT_GREEN : RGB(71, 85, 105));
            HPEN statusPen = CreatePen(PS_SOLID, 0, detecting ? Theme::ACCENT_GREEN : RGB(71, 85, 105));
            SelectObject(hdcMem, statusBrush);
            SelectObject(hdcMem, statusPen);
            RoundRect(hdcMem, statusBadge.left, statusBadge.top, statusBadge.right, statusBadge.bottom, 3, 3);
            DeleteObject(statusBrush);
            DeleteObject(statusPen);

            SetTextColor(hdcMem, RGB(255, 255, 255));
            DrawText(hdcMem, detecting ? L"LIVE" : L"OFF", -1, &statusBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT modeFieldset = {10, 65, 372, 130};
            DrawFieldset(hdcMem, modeFieldset, L"Detection Mode", hFontSubtitle);

            RECT targetFieldset = {10, 140, 372, 250};
            DrawFieldset(hdcMem, targetFieldset, L"Target Color", hFontSubtitle);

            RECT colorPreview = {20, 160, 362, 195};
            DrawColorPreview(hdcMem, colorPreview, hasTargetColor ? targetColor : Theme::BG_CARD_HOVER,
                           hasTargetColor ? ColorToString(targetColor).c_str() : L"Not Set");

            RECT currentFieldset = {10, 260, 372, 325};
            DrawFieldset(hdcMem, currentFieldset, L"Current Color", hFontSubtitle);

            RECT currentPreview = {20, 280, 362, 315};
            DrawColorPreview(hdcMem, currentPreview, lastColor, ColorToString(lastColor).c_str());

            BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

            SelectObject(hdcMem, hFontOld);
            SelectObject(hdcMem, hbmOld);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
            DeleteObject(hFontTitle);
            DeleteObject(hFontSubtitle);
            DeleteObject(hFontLabel);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;

            if (dis->CtlID == ID_RADIO_TARGET) {
                bool selected = (currentMode == MODE_TARGET_COLOR);
                DrawModernRadio(dis->hDC, dis->rcItem, L"Target Color", selected, radioTargetHover, !detecting);
            }
            else if (dis->CtlID == ID_RADIO_CHANGE) {
                bool selected = (currentMode == MODE_COLOR_CHANGE);
                DrawModernRadio(dis->hDC, dis->rcItem, L"Color Change", selected, radioChangeHover, !detecting);
            }
            else if (dis->CtlID == ID_BTN_PICK) {
                bool enabled = !detecting;
                std::wstring keyName = GetKeyName(hotkeys.pickColor);
                std::wstring label = L"Pick Color  (" + keyName + L")";
                DrawGradientButton(dis->hDC, dis->rcItem, label.c_str(), enabled, btnPickHover, false);
            }
            else if (dis->CtlID == ID_BTN_START) {
                bool enabled = !detecting ? (currentMode == MODE_COLOR_CHANGE || hasTargetColor) : true;
                std::wstring keyName = GetKeyName(hotkeys.toggle);
                std::wstring label = detecting ? (L"STOP  (" + keyName + L")") : (L"START  (" + keyName + L")");
                bool isStart = !detecting;
                DrawGradientButton(dis->hDC, dis->rcItem, label.c_str(), enabled, btnStartHover, isStart);
            }
            else if (dis->CtlID == ID_BTN_STOP) {
            }
            return TRUE;
        }

        case WM_MOUSEMOVE: {
            static ULONGLONG lastHoverUpdate = 0;
            ULONGLONG currentTime = GetTickCount64();

            if (currentTime - lastHoverUpdate < 16) {
                break;
            }
            lastHoverUpdate = currentTime;

            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            HWND hwndHover = ChildWindowFromPoint(hwnd, pt);

            bool oldRadioTargetHover = radioTargetHover;
            bool oldRadioChangeHover = radioChangeHover;
            bool oldPickHover = btnPickHover;
            bool oldStartHover = btnStartHover;
            bool oldStopHover = btnStopHover;

            radioTargetHover = (hwndHover == hwndRadioTarget);
            radioChangeHover = (hwndHover == hwndRadioChange);
            btnPickHover = (hwndHover == hwndBtnPickColor);
            btnStartHover = (hwndHover == hwndBtnStart);
            btnStopHover = (hwndHover == hwndBtnStop);

            if (oldRadioTargetHover != radioTargetHover) InvalidateRect(hwndRadioTarget, NULL, FALSE);
            if (oldRadioChangeHover != radioChangeHover) InvalidateRect(hwndRadioChange, NULL, FALSE);
            if (oldPickHover != btnPickHover) InvalidateRect(hwndBtnPickColor, NULL, FALSE);
            if (oldStartHover != btnStartHover) InvalidateRect(hwndBtnStart, NULL, FALSE);
            if (oldStopHover != btnStopHover) InvalidateRect(hwndBtnStop, NULL, FALSE);

            TRACKMOUSEEVENT tme = {0};
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            break;
        }

        case WM_MOUSELEAVE: {
            if (radioTargetHover) {
                radioTargetHover = false;
                InvalidateRect(hwndRadioTarget, NULL, FALSE);
            }
            if (radioChangeHover) {
                radioChangeHover = false;
                InvalidateRect(hwndRadioChange, NULL, FALSE);
            }
            if (btnPickHover) {
                btnPickHover = false;
                InvalidateRect(hwndBtnPickColor, NULL, FALSE);
            }
            if (btnStartHover) {
                btnStartHover = false;
                InvalidateRect(hwndBtnStart, NULL, FALSE);
            }
            if (btnStopHover) {
                btnStopHover = false;
                InvalidateRect(hwndBtnStop, NULL, FALSE);
            }
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_RADIO_TARGET:
                    currentMode = MODE_TARGET_COLOR;
                    UpdateGUI();
                    break;
                case ID_RADIO_CHANGE:
                    currentMode = MODE_COLOR_CHANGE;
                    UpdateGUI();
                    break;
                case ID_BTN_PICK:
                    PickColor();
                    break;
                case ID_BTN_START:
                    if (detecting) {
                        StopDetection();
                    } else {
                        StartDetection();
                    }
                    break;
                case ID_BTN_STOP:
                    break;
            }
            break;

        case WM_HOTKEY:
            switch (wParam) {
                case 1:
                    PickColor();
                    break;
                case 2:
                    if (detecting) StopDetection();
                    else StartDetection();
                    break;
                case 3:
                    PostQuitMessage(0);
                    break;
            }
            break;

        case WM_TIMER: {
            if (wParam == 1 && !pickingColor) {
                POINT pt;
                GetCursorPos(&pt);
                HDC hdc = GetDC(NULL);
                COLORREF newColor = GetPixel(hdc, pt.x, pt.y);
                ReleaseDC(NULL, hdc);

                if (newColor != lastColor || !detecting) {
                    lastColor = newColor;
                    RECT currentSection = {10, 260, 372, 325};
                    InvalidateRect(hwnd, &currentSection, FALSE);
                }
            }
            break;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            SetTextColor(hdcStatic, Theme::TEXT_PRIMARY);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;

        case WM_USER + 1:
            UpdateGUI();
            break;

        case WM_DESTROY:
            UnregisterHotKey(hwnd, 1);
            UnregisterHotKey(hwnd, 2);
            UnregisterHotKey(hwnd, 3);
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void DrawGradientRect(HDC hdc, RECT rect, COLORREF color1, COLORREF color2, bool horizontal) {
    int steps = horizontal ? (rect.right - rect.left) : (rect.bottom - rect.top);
    for (int i = 0; i < steps; i++) {
        float ratio = (float)i / steps;
        int r = GetRValue(color1) + (int)((GetRValue(color2) - GetRValue(color1)) * ratio);
        int g = GetGValue(color1) + (int)((GetGValue(color2) - GetGValue(color1)) * ratio);
        int b = GetBValue(color1) + (int)((GetBValue(color2) - GetBValue(color1)) * ratio);

        HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
        RECT lineRect;
        if (horizontal) {
            lineRect = {rect.left + i, rect.top, rect.left + i + 1, rect.bottom};
        } else {
            lineRect = {rect.left, rect.top + i, rect.right, rect.top + i + 1};
        }
        FillRect(hdc, &lineRect, brush);
        DeleteObject(brush);
    }
}

void DrawFieldset(HDC hdc, RECT rect, const wchar_t* title, HFONT hFont) {
    HPEN borderPen = CreatePen(PS_SOLID, 1, Theme::BORDER);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    SIZE textSize;
    GetTextExtentPoint32(hdc, title, wcslen(title), &textSize);

    int cornerRadius = 3;
    int titleX = rect.left + 15;
    int titleGap = 6;

    RECT borderRect = {rect.left, rect.top + 10, rect.right, rect.bottom};

    RoundRect(hdc, borderRect.left, borderRect.top, borderRect.right, borderRect.bottom, cornerRadius, cornerRadius);

    HPEN erasePen = CreatePen(PS_SOLID, 2, RGB(18, 18, 28));
    SelectObject(hdc, erasePen);
    MoveToEx(hdc, titleX - titleGap, rect.top + 10, NULL);
    LineTo(hdc, titleX + textSize.cx + titleGap, rect.top + 10);
    SelectObject(hdc, oldPen);
    DeleteObject(erasePen);

    SetBkMode(hdc, OPAQUE);
    SetBkColor(hdc, RGB(18, 18, 28));
    SetTextColor(hdc, Theme::TEXT_SECONDARY);
    RECT titleRect = {titleX, rect.top + 3, titleX + textSize.cx, rect.top + 23};
    DrawText(hdc, title, -1, &titleRect, DT_LEFT | DT_SINGLELINE);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldFont);
    DeleteObject(borderPen);
}

void DrawCard(HDC hdc, RECT rect) {
    RECT shadowRect = rect;
    OffsetRect(&shadowRect, 2, 2);
    HBRUSH shadowBrush = CreateSolidBrush(RGB(10, 10, 15));
    HPEN shadowPen = CreatePen(PS_SOLID, 0, RGB(10, 10, 15));
    SelectObject(hdc, shadowBrush);
    SelectObject(hdc, shadowPen);
    RoundRect(hdc, shadowRect.left, shadowRect.top, shadowRect.right, shadowRect.bottom, 20, 20);
    DeleteObject(shadowBrush);
    DeleteObject(shadowPen);

    HBRUSH brush = CreateSolidBrush(Theme::BG_CARD);
    HPEN pen = CreatePen(PS_SOLID, 1, Theme::BORDER);
    SelectObject(hdc, brush);
    SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 20, 20);
    DeleteObject(brush);
    DeleteObject(pen);
}

void DrawColorPreview(HDC hdc, RECT rect, COLORREF color, const wchar_t* label) {
    HBRUSH colorBrush = CreateSolidBrush(color);
    HPEN borderPen = CreatePen(PS_SOLID, 3, Theme::ACCENT_START);
    SelectObject(hdc, colorBrush);
    SelectObject(hdc, borderPen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 3, 3);
    DeleteObject(colorBrush);
    DeleteObject(borderPen);

    HFONT hFont = CreateCustomFont(16, FW_BOLD);
    SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);

    SetTextColor(hdc, RGB(0, 0, 0));
    RECT shadowRect = rect;
    OffsetRect(&shadowRect, 2, 2);
    DrawText(hdc, label, -1, &shadowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SetTextColor(hdc, RGB(255, 255, 255));
    DrawText(hdc, label, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DeleteObject(hFont);
}

void DrawModernRadio(HDC hdc, RECT rect, const wchar_t* text, bool selected, bool hover, bool enabled) {
    COLORREF bgColor, textColor, borderColor;

    HBRUSH containerBrush = CreateSolidBrush(Theme::BG_DARK);
    FillRect(hdc, &rect, containerBrush);
    DeleteObject(containerBrush);

    RECT drawRect = rect;
    if (hover && enabled && !selected) {
        int inset = 1;
        InflateRect(&drawRect, -inset, -inset);
    }

    if (selected) {
        HBRUSH gradBrush = CreateSolidBrush(Theme::ACCENT_START);
        HPEN gradPen = CreatePen(PS_SOLID, 0, Theme::ACCENT_START);
        SelectObject(hdc, gradBrush);
        SelectObject(hdc, gradPen);
        RoundRect(hdc, drawRect.left, drawRect.top, drawRect.right, drawRect.bottom, 3, 3);
        DeleteObject(gradBrush);
        DeleteObject(gradPen);
        textColor = RGB(255, 255, 255);
    } else {
        if (hover && enabled) {
            bgColor = Theme::BG_CARD_HOVER;
        } else {
            bgColor = Theme::BG_CARD;
        }
        HBRUSH brush = CreateSolidBrush(bgColor);
        HPEN pen = CreatePen(PS_SOLID, 0, bgColor);
        SelectObject(hdc, brush);
        SelectObject(hdc, pen);
        RoundRect(hdc, drawRect.left, drawRect.top, drawRect.right, drawRect.bottom, 3, 3);
        DeleteObject(brush);
        DeleteObject(pen);
        textColor = enabled ? Theme::TEXT_PRIMARY : Theme::TEXT_SECONDARY;
    }

    borderColor = selected ? Theme::ACCENT_START : Theme::BORDER;
    HPEN pen = CreatePen(PS_SOLID, selected ? 2 : 1, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    RoundRect(hdc, drawRect.left, drawRect.top, drawRect.right, drawRect.bottom, 3, 3);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);

    HFONT hFont = CreateCustomFont(13, selected ? FW_BOLD : FW_NORMAL);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    DrawText(hdc, text, -1, &drawRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

void DrawGradientButton(HDC hdc, RECT rect, const wchar_t* text, bool enabled, bool hover, bool isStart) {
    COLORREF color1, color2;

    RECT drawRect = rect;
    if (hover && enabled) {
        int inset = 2;
        InflateRect(&drawRect, -inset, -inset);

        RECT shadowRect = drawRect;
        OffsetRect(&shadowRect, 3, 3);
        HBRUSH shadowBrush = CreateSolidBrush(RGB(0, 0, 0));
        HPEN shadowPen = CreatePen(PS_SOLID, 0, RGB(0, 0, 0));
        SelectObject(hdc, shadowBrush);
        SelectObject(hdc, shadowPen);
        RoundRect(hdc, shadowRect.left, shadowRect.top, shadowRect.right, shadowRect.bottom, 3, 3);
        DeleteObject(shadowBrush);
        DeleteObject(shadowPen);
    }

    if (!enabled) {
        color1 = color2 = RGB(51, 65, 85);
    } else if (isStart) {
        color1 = hover ? RGB(44, 217, 114) : Theme::ACCENT_GREEN;
        color2 = hover ? RGB(24, 197, 94) : RGB(21, 167, 74);
    } else {
        color1 = hover ? RGB(255, 88, 88) : Theme::ACCENT_RED;
        color2 = hover ? RGB(220, 38, 38) : RGB(185, 28, 28);
    }

    DrawGradientRect(hdc, drawRect, color1, color2, true);

    HBRUSH brush = CreateSolidBrush(color1);
    HPEN pen = CreatePen(PS_SOLID, 0, color1);
    SelectObject(hdc, brush);
    SelectObject(hdc, pen);
    RoundRect(hdc, drawRect.left, drawRect.top, drawRect.right, drawRect.bottom, 3, 3);
    DeleteObject(brush);
    DeleteObject(pen);

    RECT innerRect = {drawRect.left + 1, drawRect.top + 1, drawRect.right - 1, drawRect.bottom - 1};
    DrawGradientRect(hdc, innerRect, color1, color2, true);

    HFONT hFont = CreateCustomFont(16, FW_BOLD);
    SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, enabled ? RGB(255, 255, 255) : Theme::TEXT_SECONDARY);
    DrawText(hdc, text, -1, &drawRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(hFont);
}

void PickColor() {
    POINT pt;
    GetCursorPos(&pt);
    HDC hdc = GetDC(NULL);
    targetColor = GetPixel(hdc, pt.x, pt.y);
    ReleaseDC(NULL, hdc);
    hasTargetColor = true;

    Beep(1500, 100);
    UpdateGUI();
}

void StartDetection() {
    if (currentMode == MODE_COLOR_CHANGE) {
        POINT pt;
        GetCursorPos(&pt);
        HDC hdc = GetDC(NULL);
        lastColor = GetPixel(hdc, pt.x, pt.y);
        ReleaseDC(NULL, hdc);
        Beep(1200, 100);
    }
    else if (currentMode == MODE_TARGET_COLOR && !hasTargetColor) {
        MessageBox(hwndMain, L"Please pick a target color first!", L"No Target Color", MB_OK | MB_ICONWARNING);
        return;
    }

    detecting = true;

    if (detectionThread && detectionThread->joinable()) {
        detectionThread->join();
        delete detectionThread;
    }

    if (currentMode == MODE_TARGET_COLOR) {
        detectionThread = new std::thread(DetectColor);
    } else {
        detectionThread = new std::thread(DetectColorChange);
    }

    Beep(1000, 100);
    UpdateGUI();
}

void StopDetection() {
    detecting = false;

    if (detectionThread && detectionThread->joinable()) {
        detectionThread->join();
        delete detectionThread;
        detectionThread = nullptr;
    }

    Beep(600, 100);
    UpdateGUI();
}

void DetectColor() {
    while (detecting) {
        POINT pt;
        GetCursorPos(&pt);
        HDC hdc = GetDC(NULL);
        COLORREF currentColor = GetPixel(hdc, pt.x, pt.y);
        ReleaseDC(NULL, hdc);

        lastColor = currentColor;

        if (currentColor == targetColor) {
            INPUT input = {0};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &input, sizeof(INPUT));

            Sleep(50);

            ZeroMemory(&input, sizeof(INPUT));
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &input, sizeof(INPUT));

            Beep(2000, 100);

            detecting = false;
            PostMessage(hwndMain, WM_USER + 1, 0, 0);
            break;
        }

        static int updateCounter = 0;
        if (++updateCounter % 10 == 0) {
            RECT currentArea = {30, 345, 360, 370};
            InvalidateRect(hwndMain, &currentArea, FALSE);
            UpdateWindow(hwndMain);
        }

        Sleep(1);
    }
}

void DetectColorChange() {
    POINT pt;
    GetCursorPos(&pt);
    HDC hdc = GetDC(NULL);
    COLORREF baselineColor = GetPixel(hdc, pt.x, pt.y);
    ReleaseDC(NULL, hdc);

    while (detecting) {
        GetCursorPos(&pt);
        hdc = GetDC(NULL);
        COLORREF currentColor = GetPixel(hdc, pt.x, pt.y);
        ReleaseDC(NULL, hdc);

        lastColor = currentColor;

        if (currentColor != baselineColor) {
            INPUT input = {0};
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
            SendInput(1, &input, sizeof(INPUT));

            Sleep(50);

            ZeroMemory(&input, sizeof(INPUT));
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
            SendInput(1, &input, sizeof(INPUT));

            Beep(2000, 100);

            detecting = false;
            PostMessage(hwndMain, WM_USER + 1, 0, 0);
            break;
        }

        static int updateCounter = 0;
        if (++updateCounter % 10 == 0) {
            RECT currentArea = {30, 345, 360, 370};
            InvalidateRect(hwndMain, &currentArea, FALSE);
            UpdateWindow(hwndMain);
        }

        Sleep(1);
    }
}

void UpdateGUI() {
    EnableWindow(hwndRadioTarget, !detecting);
    EnableWindow(hwndRadioChange, !detecting);

    RECT statusBadge = {270, 25, 340, 45};
    InvalidateRect(hwndMain, &statusBadge, FALSE);

    RECT targetSection = {10, 140, 372, 250};
    InvalidateRect(hwndMain, &targetSection, FALSE);

    InvalidateRect(hwndRadioTarget, NULL, FALSE);
    InvalidateRect(hwndRadioChange, NULL, FALSE);
    InvalidateRect(hwndBtnPickColor, NULL, FALSE);
    InvalidateRect(hwndBtnStart, NULL, FALSE);
    InvalidateRect(hwndBtnStop, NULL, FALSE);
}

std::wstring ColorToString(COLORREF color) {
    wchar_t buffer[32];
    swprintf_s(buffer, L"#%02X%02X%02X",
        GetRValue(color), GetGValue(color), GetBValue(color));
    return std::wstring(buffer);
}

std::wstring GetKeyName(int vkCode) {
    switch (vkCode) {
        case VK_UP: return L"Up";
        case VK_DOWN: return L"Down";
        case VK_LEFT: return L"Left";
        case VK_RIGHT: return L"Right";
        case VK_ESCAPE: return L"Esc";
        case VK_SPACE: return L"Space";
        case VK_RETURN: return L"Enter";
        case VK_TAB: return L"Tab";
        case VK_BACK: return L"Backspace";
        case VK_DELETE: return L"Delete";
        case VK_HOME: return L"Home";
        case VK_END: return L"End";
        case VK_PRIOR: return L"PgUp";
        case VK_NEXT: return L"PgDn";
        case VK_INSERT: return L"Insert";
        case VK_NUMPAD0: return L"Numpad 0";
        case VK_NUMPAD1: return L"Numpad 1";
        case VK_NUMPAD2: return L"Numpad 2";
        case VK_NUMPAD3: return L"Numpad 3";
        case VK_NUMPAD4: return L"Numpad 4";
        case VK_NUMPAD5: return L"Numpad 5";
        case VK_NUMPAD6: return L"Numpad 6";
        case VK_NUMPAD7: return L"Numpad 7";
        case VK_NUMPAD8: return L"Numpad 8";
        case VK_NUMPAD9: return L"Numpad 9";
        case VK_F1: return L"F1";
        case VK_F2: return L"F2";
        case VK_F3: return L"F3";
        case VK_F4: return L"F4";
        case VK_F5: return L"F5";
        case VK_F6: return L"F6";
        case VK_F7: return L"F7";
        case VK_F8: return L"F8";
        case VK_F9: return L"F9";
        case VK_F10: return L"F10";
        case VK_F11: return L"F11";
        case VK_F12: return L"F12";
        default:
            if (vkCode >= 'A' && vkCode <= 'Z') {
                return std::wstring(1, (wchar_t)vkCode);
            }
            if (vkCode >= '0' && vkCode <= '9') {
                return std::wstring(1, (wchar_t)vkCode);
            }
            return L"?";
    }
}

HFONT CreateCustomFont(int size, int weight) {
    return CreateFont(size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
}
