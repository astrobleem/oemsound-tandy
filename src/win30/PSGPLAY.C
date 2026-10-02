/* Native Windows 3.0 / 8088 PSG milestone. No IRQ or PIT hooks. */
#define WINVER 0x0300
#include <windows.h>
#define PLAY 101
#define STOP 102
#define TIMER 1
static HINSTANCE instance;
static HWND playButton, stopButton;
static int playing, stage, automatic, testStep;
static DWORD tickStart, biosStart, irq8Start, irq1cStart;
static DWORD started, stageStarted, stopped;
static char *status = "Ready";
struct event { DWORD tick; unsigned char value; };
static struct event trace[128];
static unsigned traces, traceHead, dropped;
/* OUT has no side effects on IF. Sixteen NOPs allow the SN76489 write
   settling interval at the target 4.77 MHz 8088. */
static void output(unsigned char value)
{
    unsigned index;
    if (traces < 128) index = (traceHead + traces++) & 127;
    else { index = traceHead; traceHead = (traceHead + 1) & 127;
        if (dropped < 65535U) ++dropped; }
    trace[index].tick = GetTickCount(); trace[index].value = value;
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
static void silence(void)
{
    output(0x9f); output(0xbf); output(0xdf); output(0xff);
}
static void tone(unsigned channel, unsigned period)
{
    output((unsigned char)(0x80 | (channel << 5) | (period & 15)));
    output((unsigned char)((period >> 4) & 63));
    output((unsigned char)(0x94 | (channel << 5)));
}
static void redraw(HWND w)
{
    RECT r;
    r.left = 4; r.top = 42; r.right = 148; r.bottom = 58;
    InvalidateRect(w, &r, TRUE);
}
static void stop(HWND w)
{
    KillTimer(w, TIMER); playing = 0; stage = 0;
    silence(); status = "Stopped: muted";
    EnableWindow(playButton, TRUE); redraw(w);
}
static void play(HWND w)
{
    stop(w);
    if (!SetTimer(w, TIMER, 50, NULL)) {
        status = "No timer"; redraw(w);
        if (automatic) PostMessage(w, WM_CLOSE, 0, 0L);
        return;
    }
    started = GetTickCount(); stageStarted = started; playing = 1; stage = 1;
    tone(0, 427); status = "Voice 1: C4";
    redraw(w);
}
/* Read-only diagnostic snapshots, used only after real-mode check. */
static DWORD readFar(WORD segment, WORD offset)
{
    volatile WORD FAR *p = (volatile WORD FAR *)MAKELONG(offset, segment);
    WORD high1, low, high2;
    do { high1 = p[1]; low = p[0]; high2 = p[1]; } while (high1 != high2);
    return MAKELONG(low, high1);
}
static int argIs(LPSTR cmd, char *s)
{
    while (*cmd == ' ') ++cmd;
    while (*s && *cmd == *s) { ++s; ++cmd; }
    while (*cmd == ' ') ++cmd;
    return *s == 0 && *cmd == 0;
}
static void saveTrace(void)
{
    HFILE f; char line[96]; unsigned i, index;
    f = _lcreat("C:\\PSGTRACE.LOG", 0);
    if (f == HFILE_ERROR) return;
    wsprintf(line, "MODE %u DROPPED %u\r\n", automatic, dropped);
    _lwrite(f, line, lstrlen(line));
    wsprintf(line, "TICKS %lu %lu BIOS %lu %lu\r\n", tickStart,
        GetTickCount(), biosStart, readFar(0x40, 0x6c));
    _lwrite(f, line, lstrlen(line));
    wsprintf(line, "IRQ8 %08lX %08lX IRQ1C %08lX %08lX\r\n", irq8Start,
        readFar(0, 0x20), irq1cStart, readFar(0, 0x70));
    _lwrite(f, line, lstrlen(line));
    for (i = 0; i < traces; ++i) {
        index = (traceHead + i) & 127;
        wsprintf(line, "%lu %02X\r\n", trace[index].tick,
                 (unsigned)trace[index].value);
        _lwrite(f, line, lstrlen(line));
    }
    _lclose(f);
}
LONG FAR PASCAL WndProc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    HDC dc; PAINTSTRUCT ps; DWORD elapsed;
    switch (m) {
    case WM_CREATE:
        playButton = CreateWindow("BUTTON", "&Play", WS_CHILD | WS_VISIBLE |
            WS_TABSTOP, 8, 8, 60, 24, w, (HMENU)PLAY, instance, NULL);
        stopButton = CreateWindow("BUTTON", "&Stop", WS_CHILD | WS_VISIBLE |
            WS_TABSTOP, 76, 8, 60, 24, w, (HMENU)STOP, instance, NULL);
        silence();
        if (automatic) {
            PostMessage(w, WM_COMMAND, PLAY, 0L);
        }
        return 0;
    case WM_COMMAND:
        if (wp == PLAY) play(w);
        if (wp == STOP) stop(w);
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE || wp == 'S') stop(w);
        if (wp == 'P') play(w);
        return 0;
    case WM_TIMER:
        if (wp != TIMER) return 0;
        if (!playing) {
            if (automatic && testStep && GetTickCount() - stopped >= 700L)
                PostMessage(w, WM_CLOSE, 0, 0L);
            return 0;
        }
        elapsed = GetTickCount() - started;
        if (automatic == 3 && elapsed >= 250L) {
            PostMessage(w, WM_CLOSE, 0, 0L); return 0;
        }
        if (automatic == 2 && elapsed >= 250L) {
            stop(w); testStep = 1; stopped = GetTickCount();
            if (!SetTimer(w, TIMER, 50, NULL))
                PostMessage(w, WM_CLOSE, 0, 0L);
            return 0;
        }
        elapsed = GetTickCount() - stageStarted;
        if (stage == 3 && elapsed >= 1200L) {
            stop(w);
            if (automatic) { testStep = 1; stopped = GetTickCount();
                if (!SetTimer(w, TIMER, 50, NULL))
                    PostMessage(w, WM_CLOSE, 0, 0L); }
        } else {
            if (elapsed >= 600L && stage == 1) {
                tone(1, 339); stage = 2; stageStarted = GetTickCount();
                status = "C4 + E4"; redraw(w);
            } else if (elapsed >= 600L && stage == 2) {
                tone(2, 285); stage = 3; stageStarted = GetTickCount();
                status = "C4 + E4 + G4"; redraw(w);
            }
        }
        return 0;
    case WM_PAINT:
        dc = BeginPaint(w, &ps);
        SelectObject(dc, GetStockObject(SYSTEM_FIXED_FONT));
        TextOut(dc, 8, 44, status, lstrlen(status));
        TextOut(dc, 8, 64, "3 tones: ~2.4s", 14);
        TextOut(dc, 8, 80, "Esc/S = mute", 12);
        EndPaint(w, &ps); return 0;
    case WM_ENDSESSION:
        if (wp) { stop(w); saveTrace(); }
        return 0;
    case WM_CLOSE:
        stop(w); DestroyWindow(w); return 0;
    case WM_DESTROY:
        stop(w); PostQuitMessage(0); return 0;
    }
    return DefWindowProc(w, m, wp, lp);
}
int PASCAL WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    WNDCLASS wc; HWND w; MSG msg;
    instance = inst; automatic = 0;
    if (cmd && argIs(cmd, "/test")) automatic = 1;
    if (cmd && argIs(cmd, "/stoptest")) automatic = 2;
    if (cmd && argIs(cmd, "/closetest")) automatic = 3;
    if (GetWinFlags() & WF_PMODE) return 4;
    tickStart = GetTickCount(); biosStart = readFar(0x40, 0x6c);
    irq8Start = readFar(0, 0x20); irq1cStart = readFar(0, 0x70);
    /* The PSG is global hardware: keep this direct-I/O test single-instance. */
    if (prev) return 1;
    wc.style = CS_HREDRAW | CS_VREDRAW; wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0; wc.cbWndExtra = 0; wc.hInstance = inst;
    wc.hIcon = NULL; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL; wc.lpszClassName = "TandyPSGTest";
    if (!RegisterClass(&wc)) return 2;
    w = CreateWindow("TandyPSGTest", "PSG", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        0, 0, 160, 140, NULL, NULL, inst, NULL);
    if (!w) { silence(); return 3; }
    ShowWindow(w, show); UpdateWindow(w);
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN &&
            (msg.wParam == VK_ESCAPE || msg.wParam == 'S')) {
            SendMessage(w, WM_COMMAND, STOP, 0L); continue;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == 'P') {
            SendMessage(w, WM_COMMAND, PLAY, 0L); continue;
        }
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    silence(); saveTrace();
    if (automatic) ExitWindows(0L, 0);
    return msg.wParam;
}
