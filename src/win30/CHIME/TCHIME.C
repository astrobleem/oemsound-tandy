/* Optional one-shot startup chime for known Tandy PSG hardware.
   Windows 3.0 real mode / 8086. No IRQ/PIT changes or resident helper. */
#define WINVER 0x0300
#include <windows.h>
#define TIMER 1
static DWORD began;
static unsigned phase;
static int active;
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
            tone(0, 214); /* C5, approximately 523 Hz. */
            return 0;
        }
        elapsed = GetTickCount() - began;
        /* Never play a burst of overdue notes during a slow startup.
           Normal completion is 825 ms. An overdue transition ends quietly. */
        if (elapsed >= 825L) { finish(w); return 0; }
        due = phase == 0 ? 165L : phase == 1 ? 330L : 650L;
        if (elapsed > due + 220L) { finish(w); return 0; }
        if (elapsed < due) return 0;
        if (phase == 0) tone(1, 170);       /* E5, approximately 658 Hz. */
        else if (phase == 1) tone(2, 127);  /* A5, approximately 881 Hz. */
        else if (phase == 2) {
            psgByte(0x9a); psgByte(0xba); psgByte(0xda);
        }
        if (phase < 3) ++phase;
        return 0;
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        if (wp) cleanup(w);
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
    WNDCLASS c; HWND w; MSG msg;
    /* Opt-in is for a known original Tandy. Signature is only a fail-closed
       compatibility guard; it does not identify an exact EX/HX model. */
    if (previous || (GetWinFlags() & WF_PMODE)) return 1;
    if (*(unsigned char FAR *)0xfc000000UL != 0x21) return 2;
    c.style = 0; c.lpfnWndProc = ChimeProc;
    c.cbClsExtra = 0; c.cbWndExtra = 0; c.hInstance = inst;
    c.hIcon = NULL; c.hCursor = NULL; c.hbrBackground = NULL;
    c.lpszMenuName = NULL; c.lpszClassName = "TandyStartupChime";
    if (!RegisterClass(&c)) return 3;
    w = CreateWindow("TandyStartupChime", "Tandy startup chime", WS_POPUP,
        0, 0, 0, 0, NULL, NULL, inst, NULL);
    if (!w) { muteAll(); return 4; }
    /* Deliberately no ShowWindow: the hidden owner only receives messages.
       GetMessage yields while Program Manager and other startup apps run. */
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    muteAll(); return msg.wParam;
}
