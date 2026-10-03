/* Optional one-shot pre-exit chime for known Tandy PSG hardware.
   Windows 3.0 real mode / 8086. No IRQ/PIT changes or resident helper. */
#define WINVER 0x0300
#include <windows.h>
#define TIMER 1
static DWORD began;
static unsigned phase;
static int active;
static int preview, testmode, requestExit;
static void logResult(char *s)
{
    HFILE f;
    if (!testmode) return;
    f = _lopen("C:\\TEXIT.LOG", 1);
    if (f == HFILE_ERROR) f = _lcreat("C:\\TEXIT.LOG", 0);
    if (f != HFILE_ERROR) {
        _llseek(f, 0L, 2); _lwrite(f, s, lstrlen(s)); _lclose(f);
    }
}
static int argIs(LPSTR p, char *s)
{
    unsigned i = 0;
    while (*p == ' ') ++p;
    while (s[i] && p[i] == s[i]) ++i;
    if (s[i]) return 0;
    p += i; while (*p == ' ') ++p;
    return !*p;
}
static int busy(void)
{
    HMODULE h;
    if (FindWindow("TandyPSGTest", NULL) ||
        FindWindow("TandyStartupChime", NULL) ||
        FindWindow("TandyXPStartupChime", NULL) ||
        FindWindow("TandyAutomaticMouth", NULL)) return 1;
    h = GetModuleHandle("SOUND");
    return h && GetProcAddress(h, "DEBUGTIMER") != NULL;
}
/* Compact native prompt: standard two-button MessageBox clips at 160px. */
static HINSTANCE promptInst;
static HWND yesButton, noButton;
static int promptDone, promptResult;
LONG FAR PASCAL ConfirmProc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    switch (m) {
    case WM_CREATE:
        CreateWindow("STATIC", "Play chime,\nthen exit?",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 8, 6, 132, 30,
            w, (HMENU)10, promptInst, NULL);
        yesButton = CreateWindow("BUTTON", "Exit",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 8, 43, 56, 20,
            w, (HMENU)IDOK, promptInst, NULL);
        noButton = CreateWindow("BUTTON", "Cancel",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 76, 43, 62, 20,
            w, (HMENU)IDCANCEL, promptInst, NULL);
        return 0;
    case WM_COMMAND:
        if (wp == IDOK || wp == IDCANCEL) {
            promptResult = wp; DestroyWindow(w);
        }
        return 0;
    case WM_QUERYENDSESSION:
        PostMessage(w, WM_CLOSE, 0, 0); return TRUE;
    case WM_ENDSESSION:
        if (!wp) return 0;
    case WM_CLOSE:
        promptResult = IDCANCEL; DestroyWindow(w); return 0;
    case WM_DESTROY:
        promptDone = 1; return 0;
    }
    return DefWindowProc(w, m, wp, lp);
}
static int confirmExit(HINSTANCE inst)
{
    WNDCLASS c; HWND w; MSG msg; int x, y;
    promptInst = inst; promptDone = 0; promptResult = IDCANCEL;
    yesButton = noButton = NULL;
    c.style = 0; c.lpfnWndProc = ConfirmProc;
    c.cbClsExtra = c.cbWndExtra = 0; c.hInstance = inst;
    c.hIcon = NULL; c.hCursor = LoadCursor(NULL, IDC_ARROW);
    c.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    c.lpszMenuName = NULL; c.lpszClassName = "TandyExitConfirm";
    if (!RegisterClass(&c)) return IDCANCEL;
    x = (GetSystemMetrics(SM_CXSCREEN) - 150) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - 92) / 2;
    if (x < 0) x = 0; if (y < 0) y = 0;
    w = CreateWindow("TandyExitConfirm", "Exit",
        WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, 150, 92,
        NULL, NULL, inst, NULL);
    if (!w) return IDCANCEL;
    if (!yesButton || !noButton) { DestroyWindow(w); return IDCANCEL; }
    ShowWindow(w, SW_SHOWNORMAL); UpdateWindow(w); SetFocus(noButton);
    while (!promptDone && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_ESCAPE || msg.wParam == VK_RETURN) {
                SendMessage(w, WM_COMMAND,
                    msg.wParam == VK_RETURN && GetFocus() == yesButton ?
                    IDOK : IDCANCEL, 0);
                continue;
            }
            if (msg.wParam == VK_TAB) {
                SetFocus(GetFocus() == yesButton ? noButton : yesButton);
                continue;
            }
        }
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    if (!promptDone) { promptResult = IDCANCEL; DestroyWindow(w); }
    return promptResult;
}
static void psgByte(unsigned char value)
{
    _asm { mov dx, 0c0h
           mov al, value
           out dx, al
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop
           nop }
}
static void muteAll(void)
{
    psgByte(0x9f); psgByte(0xbf); psgByte(0xdf); psgByte(0xff);
}
static void tone(unsigned voice, unsigned period)
{
    psgByte((unsigned char)(0x80 | (voice << 5) | (period & 15)));
    psgByte((unsigned char)((period >> 4) & 63));
    psgByte((unsigned char)(0x96 | (voice << 5)));
}
static void cleanup(HWND w)
{
    KillTimer(w, TIMER); active = 0; muteAll();
}
static void finish(HWND w)
{
    cleanup(w); DestroyWindow(w);
}
LONG FAR PASCAL ChimeProc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    DWORD elapsed, due;
    switch (m) {
    case WM_CREATE:
        muteAll();
        if (!SetTimer(w, TIMER, 55, NULL)) return -1;
        phase = 4; active = 1;
        /* Wait for the first delivered timer, so initial desktop painting
           can finish before any sustained tone begins. No startup wait. */
        return 0;
    case WM_TIMER:
        if (wp != TIMER || !active) return 0;
        if (phase == 4) {
            began = GetTickCount(); phase = 0;
            tone(0, 240); /* Bb4: original descending companion phrase. */
            return 0;
        }
        elapsed = GetTickCount() - began;
        /* Never play a burst of overdue notes during a slow startup.
           Normal completion is 825 ms. An overdue transition ends quietly. */
        if (elapsed >= 825L) { requestExit = !preview && phase == 3; finish(w); return 0; }
        due = phase == 0 ? 165L : phase == 1 ? 330L : 650L;
        if (elapsed > due + 220L) { logResult("LATE ABORT\r\n"); finish(w); return 0; }
        if (elapsed < due) return 0;
        if (phase == 0) tone(1, 285);       /* G4. */
        else if (phase == 1) tone(2, 359);  /* Eb4. */
        else if (phase == 2) {
            psgByte(0x9a); psgByte(0xba); psgByte(0xda);
        }
        if (phase < 3) ++phase;
        return 0;
    case WM_QUERYENDSESSION:
        /* Another exit request cancels this chime promptly. Do not wait
           for a timer while Windows is asking applications to save. */
        requestExit = 0; cleanup(w); PostMessage(w, WM_CLOSE, 0, 0);
        return TRUE;
    case WM_ENDSESSION:
        if (wp) { requestExit = 0; cleanup(w); }
        return 0;
    case WM_CLOSE:
        finish(w); return 0;
    case WM_DESTROY:
        cleanup(w); PostQuitMessage(0); return 0;
    }
    return DefWindowProc(w, m, wp, lp);
}
int PASCAL WinMain(HINSTANCE inst, HINSTANCE previous, LPSTR cmd, int show)
{
    WNDCLASS c; HWND w; MSG msg; BOOL exited;
    /* Opt-in is for a known original Tandy. Signature is only a fail-closed
       compatibility guard; it does not identify an exact EX/HX model. */
    if (previous || (GetWinFlags() & WF_PMODE)) return 1;
    if (*(unsigned char FAR *)0xfc000000UL != 0x21) return 2;
    preview = argIs(cmd, "/preview");
    testmode = argIs(cmd, "/testgo") || argIs(cmd, "/testpreview");
    if (argIs(cmd, "/testpreview")) preview = 1;
    if (!argIs(cmd, "") && !argIs(cmd, "/go") && !preview && !testmode)
        return 6;
    if (!preview && !testmode && !argIs(cmd, "/go")) {
        if (confirmExit(inst) != IDOK) return 0;
    }
    if (busy()) {
        logResult("BUSY: NO PSG OR EXIT\r\n");
        if (!testmode) MessageBox(NULL, "Close other PSG apps first.\nUse plain Exit if TSOUND is loaded.",
            "Exit cancelled", MB_OK);
        return 7;
    }
    logResult("START\r\n");
    c.style = 0; c.lpfnWndProc = ChimeProc;
    c.cbClsExtra = 0; c.cbWndExtra = 0; c.hInstance = inst;
    c.hIcon = NULL; c.hCursor = NULL; c.hbrBackground = NULL;
    c.lpszMenuName = NULL; c.lpszClassName = "TandyPreExitChime";
    if (!RegisterClass(&c)) return 3;
    w = CreateWindow("TandyPreExitChime", "Tandy pre-exit chime", WS_POPUP,
        0, 0, 0, 0, NULL, NULL, inst, NULL);
    if (!w) { muteAll(); return 4; }
    /* Deliberately no ShowWindow: the hidden owner only receives messages.
       GetMessage yields while Program Manager and other startup apps run. */
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    muteAll(); logResult("MUTED WINDOW GONE\r\n");
    if (!requestExit) { logResult("NO EXIT REQUEST\r\n"); return 0; }
    /* Call only after cleanup and leaving the message loop. Windows itself
       sends save/exit queries. A veto must leave the desktop running. */
    logResult("CALL ExitWindows\r\n");
    exited = ExitWindows(0L, 0);
    logResult(exited ? "RETURN TRUE\r\n" : "VETO OR FAILURE\r\n");
    if (!exited && !testmode) MessageBox(NULL,
        "Windows is still running.\nExit was cancelled or failed.",
        "Exit Windows", MB_OK);
    return exited ? 0 : 5;
}
