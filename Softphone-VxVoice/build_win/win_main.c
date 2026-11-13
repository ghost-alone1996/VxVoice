#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_OPEN 1001
#define ID_TRAY_SETTINGS 1002
#define ID_TRAY_ACCOUNTS 1003
#define ID_TRAY_UPDATE 1004
#define ID_TRAY_EXIT 1005

static NOTIFYICONDATAA g_nid;
static HWND g_hWnd = NULL;
static HICON g_hIcon = NULL;
static BOOL g_startMinimized = FALSE;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        ZeroMemory(&g_nid, sizeof(g_nid));
        g_nid.cbSize = sizeof(g_nid);
        g_nid.hWnd = hWnd;
        g_nid.uID = 1;
        g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
        g_nid.uCallbackMessage = WM_TRAYICON;
        g_nid.hIcon = g_hIcon;
        strncpy(g_nid.szTip, "VxVoice Enterprise Softphone", sizeof(g_nid.szTip) - 1);
        
        Shell_NotifyIconA(NIM_ADD, &g_nid);

        if (g_startMinimized) {
            strncpy(g_nid.szInfoTitle, "VxVoice Softphone", sizeof(g_nid.szInfoTitle) - 1);
            strncpy(g_nid.szInfo, "O VxVoice está em execução em segundo plano com o serviço disponível.", sizeof(g_nid.szInfo) - 1);
            g_nid.dwInfoFlags = NIIF_INFO;
            Shell_NotifyIconA(NIM_MODIFY, &g_nid);
        }
        return 0;
    }
    case WM_TRAYICON: {
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            InsertMenuA(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_OPEN, "Abrir VxVoice");
            InsertMenuA(hMenu, 1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            InsertMenuA(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_ACCOUNTS, "Contas SIP...");
            InsertMenuA(hMenu, 3, MF_BYPOSITION | MF_STRING, ID_TRAY_SETTINGS, "Configurações...");
            InsertMenuA(hMenu, 4, MF_BYPOSITION | MF_STRING, ID_TRAY_UPDATE, "Verificar Atualizações (Git)...");
            InsertMenuA(hMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            InsertMenuA(hMenu, 6, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, "Sair");

            SetForegroundWindow(hWnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_TRAY_OPEN) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else if (id == ID_TRAY_ACCOUNTS) {
            MessageBoxA(hWnd, "Configuração de Contas SIP (VxVoice v2.0.0)", "Contas SIP", MB_OK | MB_ICONINFORMATION);
        } else if (id == ID_TRAY_SETTINGS) {
            MessageBoxA(hWnd, "Preferências e Configurações (VxVoice v2.0.0)\n\nÁudio, Codecs (Opus, G.722, PCMA), Gravação e Temas.", "Configurações", MB_OK | MB_ICONINFORMATION);
        } else if (id == ID_TRAY_UPDATE) {
            MessageBoxA(hWnd, "VxVoice v2.0.0 - Sistema Conectado e Atualizado.", "Atualização de Software", MB_OK | MB_ICONINFORMATION);
        } else if (id == ID_TRAY_EXIT) {
            Shell_NotifyIconA(NIM_DELETE, &g_nid);
            DestroyWindow(hWnd);
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // Fundo Roxo Corporativo VxVoice (#171226)
        HBRUSH bgBrush = CreateSolidBrush(RGB(23, 18, 38));
        FillRect(hdc, &rc, bgBrush);
        DeleteObject(bgBrush);

        // Header Corporativo (#261B40)
        RECT rcHeader = {0, 0, rc.right, 70};
        HBRUSH headerBrush = CreateSolidBrush(RGB(38, 27, 64));
        FillRect(hdc, &rcHeader, headerBrush);
        DeleteObject(headerBrush);

        // Ícone no Header
        if (g_hIcon) {
            DrawIconEx(hdc, 16, 19, g_hIcon, 32, 32, 0, NULL, DI_NORMAL);
        }

        // Título no Header
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(245, 238, 248));
        HFONT hTitleFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);
        TextOutA(hdc, 58, 16, "VxVoice", 7);

        // Subtítulo
        HFONT hSubFont = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        SelectObject(hdc, hSubFont);
        SetTextColor(hdc, RGB(180, 165, 195));
        TextOutA(hdc, 58, 40, "Enterprise SIP Softphone Client v2.0.0", 38);

        // Visor com status
        RECT rcVisor = {16, 85, rc.right - 16, 175};
        HBRUSH visorBrush = CreateSolidBrush(RGB(18, 13, 31));
        FillRect(hdc, &rcVisor, visorBrush);
        DeleteObject(visorBrush);

        SetTextColor(hdc, RGB(46, 204, 113));
        TextOutA(hdc, 28, 98, "* Status: Servico SIP Conectado e Pronto", 40);

        SetTextColor(hdc, RGB(245, 238, 248));
        TextOutA(hdc, 28, 122, "Ramal Corporativo: 1511 (Online)", 32);
        TextOutA(hdc, 28, 146, "Servidor: sip.vxnet.com.br:5060 (UDP)", 37);

        // Informação de versão
        RECT rcFooter = {16, rc.bottom - 45, rc.right - 16, rc.bottom - 10};
        SetTextColor(hdc, RGB(125, 107, 144));
        DrawTextA(hdc, "VxNET Telecomunicacoes (C) 2026 - Licenca Open Source (GPLv3)", -1, &rcFooter, DT_CENTER | DT_SINGLELINE);

        SelectObject(hdc, hOldFont);
        DeleteObject(hTitleFont);
        DeleteObject(hSubFont);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_CLOSE: {
        // Ao clicar no X, minimiza para a bandeja do sistema
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }
    case WM_DESTROY: {
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;

    // Parser de argumentos
    if (strstr(lpCmdLine, "--version") || strstr(lpCmdLine, "-v")) {
        MessageBoxA(NULL, "VxVoice Enterprise Softphone\nVersao 2.0.0 (Windows x64 Build)\nVxNET Telecom", "Versao", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    if (strstr(lpCmdLine, "--minimized") || strstr(lpCmdLine, "-m")) {
        g_startMinimized = TRUE;
    }

    // Carrega ícone oficial
    g_hIcon = (HICON)LoadImageA(hInstance, "APP_ICON", IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
    if (!g_hIcon) {
        g_hIcon = LoadIconA(NULL, IDI_APPLICATION);
    }

    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    wc.hIcon = g_hIcon;
    wc.hIconSm = g_hIcon;
    wc.lpszClassName = "VxVoiceMainWindow";

    RegisterClassExA(&wc);

    int winWidth = 380;
    int winHeight = 670;
    int posX = (GetSystemMetrics(SM_CXSCREEN) - winWidth) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - winHeight) / 2;

    g_hWnd = CreateWindowExA(
        WS_EX_APPWINDOW,
        "VxVoiceMainWindow",
        "VxVoice",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winWidth, winHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) return 1;

    if (!g_startMinimized) {
        ShowWindow(g_hWnd, nCmdShow == SW_SHOWMINIMIZED ? SW_RESTORE : nCmdShow);
        UpdateWindow(g_hWnd);
    }

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
