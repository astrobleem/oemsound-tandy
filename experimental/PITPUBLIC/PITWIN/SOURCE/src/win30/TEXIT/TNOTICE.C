/* Compact native warning shared by TSHELL and TEXIT: Windows 3.0 / 8086. */
#define WINVER 0x0300
#include <windows.h>
#include "TNOTICE.H"

#define NOTICE_WIDTH 128
#define NOTICE_BODY 116
#define NOTICE_MAX 160
#define NOTICE_HEIGHT 100

static HINSTANCE noticeInstance;
static HWND noticeWindow;
static int noticeBusy, noticeTextHeight;
static char noticeText[NOTICE_MAX + 1];

static void noticeWord(BYTE FAR **p, unsigned value)
{
    *(*p)++ = (BYTE)value;
    *(*p)++ = (BYTE)(value >> 8);
}

BOOL FAR PASCAL NoticeProc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    RECT r, c, body;
    HDC dc;
    PAINTSTRUCT ps;
    HFONT oldfont;
    HWND button;
    int ww, hh;
    if (m == WM_INITDIALOG) {
        noticeWindow = w;
        dc = GetDC(w);
        oldfont = SelectObject(dc, GetStockObject(SYSTEM_FIXED_FONT));
        SetRect(&body, 0, 0, NOTICE_BODY, 0);
        DrawText(dc, noticeText, -1, &body,
                 DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
        if (body.bottom > NOTICE_HEIGHT || body.right > NOTICE_BODY) {
            lstrcpy(noticeText, "Notice too long.\nSee README.TXT.");
            SetRect(&body, 0, 0, NOTICE_BODY, 0);
            DrawText(dc, noticeText, -1, &body,
                     DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_CALCRECT);
        }
        SelectObject(dc, oldfont);
        ReleaseDC(w, dc);
        noticeTextHeight = body.bottom;
        if (noticeTextHeight < 16) noticeTextHeight = 16;
        GetWindowRect(w, &r);
        GetClientRect(w, &c);
        ww = NOTICE_WIDTH + r.right - r.left - c.right;
        hh = noticeTextHeight + 38 + r.bottom - r.top - c.bottom;
        SetWindowPos(w, NULL, (GetSystemMetrics(SM_CXSCREEN) - ww) / 2,
                     (GetSystemMetrics(SM_CYSCREEN) - hh) / 2,
                     ww, hh, SWP_NOZORDER);
        button = CreateWindow("BUTTON", "&OK",
                              WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                              BS_DEFPUSHBUTTON, 35, noticeTextHeight + 12,
                              58, 20, w, (HMENU)IDOK, noticeInstance, NULL);
        if (!button) { EndDialog(w, IDCANCEL); return TRUE; }
        SendMessage(button, WM_SETFONT,
                    (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), 0L);
        SetFocus(button);
        return FALSE;
    }
    if (m == WM_PAINT) {
        dc = BeginPaint(w, &ps);
        oldfont = SelectObject(dc, GetStockObject(SYSTEM_FIXED_FONT));
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
        SetRect(&body, 6, 6, 6 + NOTICE_BODY, 6 + noticeTextHeight);
        DrawText(dc, noticeText, -1, &body,
                 DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
        SelectObject(dc, oldfont);
        EndPaint(w, &ps);
        return TRUE;
    }
    if ((m == WM_COMMAND && (wp == IDOK || wp == IDCANCEL)) ||
        m == WM_CLOSE) {
        EndDialog(w, IDOK);
        return TRUE;
    }
    /* Leave QUERYENDSESSION to the default (TRUE); unwind only on real exit. */
    if (m == WM_ENDSESSION && wp) { EndDialog(w, IDCANCEL); return TRUE; }
    if (m == WM_DESTROY) noticeWindow = NULL;
    return FALSE;
}

void TinyNotice(HWND owner, HINSTANCE instance, char *title, char *text)
{
    HGLOBAL memory;
    BYTE FAR *p;
    FARPROC proc;
    DWORD style, units;
    unsigned n;
    int result;
    if (noticeBusy) {
        if (noticeWindow) {
            SetActiveWindow(noticeWindow);
            SetFocus(GetDlgItem(noticeWindow, IDOK));
        }
        /* Silent notice failure: never bypass sound ownership. */
        return;
    }
    if (!instance) { /* Silent notice failure: never bypass sound ownership. */ return; }
    noticeBusy = 1;
    noticeInstance = instance;
    if (!text) text = "Notice.";
    n = 0;
    while (n < NOTICE_MAX && text[n]) { noticeText[n] = text[n]; ++n; }
    noticeText[n] = 0;
    if (text[n]) lstrcpy(noticeText, "Notice too long.\nSee README.TXT.");
    if (!title) title = "Notice";
    for (n = 0; n < 32 && title[n]; ++n) ;
    if (n == 32) title = "Notice";
    memory = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 64);
    if (!memory) { noticeBusy = 0; /* Silent notice failure: never bypass sound ownership. */ return; }
    p = (BYTE FAR *)GlobalLock(memory);
    if (!p) {
        GlobalFree(memory); noticeBusy = 0; /* Silent notice failure: never bypass sound ownership. */ return;
    }
    style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME;
    noticeWord(&p, LOWORD(style)); noticeWord(&p, HIWORD(style)); *p++ = 0;
    units = GetDialogBaseUnits();
    noticeWord(&p, 0); noticeWord(&p, 0);
    noticeWord(&p, NOTICE_WIDTH * 4 / LOWORD(units));
    noticeWord(&p, 60 * 8 / HIWORD(units));
    *p++ = 0; *p++ = 0;
    while (*title) *p++ = *title++;
    *p = 0;
    GlobalUnlock(memory);
    proc = MakeProcInstance((FARPROC)NoticeProc, instance);
    result = -1;
    if (proc) {
        result = DialogBoxIndirect(instance, memory, owner, (DLGPROC)proc);
        FreeProcInstance(proc);
    }
    GlobalFree(memory);
    noticeWindow = NULL;
    noticeBusy = 0;
    if (result == -1) { /* Silent notice failure. */ return; }
}
