/* Optional XP-note startup chime for known Tandy PSG hardware.
   Windows 3.0 real mode / 8086. No IRQ/PIT changes or resident helper. */
#define WINVER 0x0300
#include <windows.h>
#define TIMER 1
static DWORD began;
static unsigned phase;
static int active;
static int soundOwned;
static int testing;
static unsigned psgWrites;
static void testReport(char *result)
{
    HFILE f; char line[80];
    if (!testing) return;
    f = _lopen("C:\\XPCHIME.LOG", 1);
    if (f == HFILE_ERROR) f = _lcreat("C:\\XPCHIME.LOG", 0);
    if (f == HFILE_ERROR) return;
    _llseek(f, 0L, 2);
    wsprintf(line, "%s WRITES=%u OWNED=%u\r\n", (LPSTR)result,
             psgWrites, soundOwned);
    _lwrite(f, line, lstrlen(line)); _lclose(f);
}
/* User-supplied Bill Brown score, arranged by David Caldarella.
   Quarter = 100, 6/8: Eb6 Eb5 Bb5 Ab5 Eb6 Bb6.
   PSG periods round 3579545 / (32 * equal-tempered Hz). */
static unsigned notes[6] = { 90, 180, 120, 135, 90, 60 };
static unsigned onsets[6] = { 0, 300, 450, 900, 1500, 1800 };
static void psgByte(unsigned char value)
{
    if (testing) ++psgWrites;
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
    if (!soundOwned) return;
    psgByte(0x9f); psgByte(0xbf); psgByte(0xdf); psgByte(0xff);
}
static void tone(unsigned voice, unsigned period, unsigned attenuation)
{
    psgByte((unsigned char)(0x80 | (voice << 5) | (period & 15)));
    psgByte((unsigned char)((period >> 4) & 63));
    psgByte((unsigned char)(0x90 | (voice << 5) | attenuation));
}
static void cleanup(HWND w)
{
    KillTimer(w, TIMER); active = 0; muteAll();
    if (soundOwned) { CloseSound(); soundOwned = 0; }
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
        phase = 7; active = 1;
        /* Wait for the first delivered timer, so initial desktop painting
           can finish before any sustained tone begins. No startup wait. */
        return 0;
    case WM_TIMER:
        if (wp != TIMER || !active) return 0;
        if (phase == 7) {
            began = GetTickCount(); phase = 0;
            tone(0, notes[0], 6); /* Melody, quieter than the tone demo. */
            tone(1, 269, 10); /* Ab4. */
            tone(2, 641, 11); /* F3: cello F2 raised one octave. */
            return 0;
        }
        elapsed = GetTickCount() - began;
        /* Late messages never trigger a catch-up burst. Cooperative
           Windows cannot promise wall-clock mute while another task blocks. */
        if (elapsed >= 2700L) { finish(w); return 0; }
        if (elapsed >= 2400L && phase == 5) {
            psgByte(0x9f); psgByte(0xbf); phase = 6;
            return 0; /* Final dotted-quarter bass continues to 2700 ms. */
        }
        if (phase >= 5) return 0;
        due = onsets[phase + 1];
        if (elapsed > due + 165L) { finish(w); return 0; }
        if (elapsed < due) return 0;
        ++phase; tone(0, notes[phase], 6);
        if (phase == 3) tone(2, 539, 11); /* Ab3. */
        if (phase == 5) {
            tone(1, 285, 10); /* G4. */
            tone(2, 719, 11); /* Eb3. */
        }
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
    testing = !lstrcmp(cmd, "/test");
    /* Opt-in is for a known original Tandy. Signature is only a fail-closed
       compatibility guard; it does not identify an exact EX/HX model. */
    if (previous || (GetWinFlags() & WF_PMODE)) return 1;
    if (*(unsigned char FAR *)0xfc000000UL != 0x21) return 2;
    c.style = 0; c.lpfnWndProc = ChimeProc;
    c.cbClsExtra = 0; c.cbWndExtra = 0; c.hInstance = inst;
    c.hIcon = NULL; c.hCursor = NULL; c.hbrBackground = NULL;
    c.lpszMenuName = NULL; c.lpszClassName = "TandyXPStartupChime";
    if (!RegisterClass(&c)) return 3;
    /* Refuse older direct-I/O companions as well as leased sound owners.
       A failed acquisition must never write even a mute to the PSG. */
    if (FindWindow("TandyPSGTest", NULL) ||
        FindWindow("TandyAutomaticMouth", NULL) ||
        FindWindow("TandyPreExitChime", NULL) ||
        FindWindow("TandyStartupChime", NULL)) { testReport("BUSY WINDOW"); return 5; }
    if (OpenSound() < 0) { testReport("BUSY OWNER"); return 5; }
    soundOwned = 1; StopSound();
    w = CreateWindow("TandyXPStartupChime", "Tandy XP-note startup chime", WS_POPUP,
        0, 0, 0, 0, NULL, NULL, inst, NULL);
    if (!w) { muteAll(); if (soundOwned) CloseSound();
        soundOwned = 0; return 4; }
    /* Deliberately no ShowWindow: the hidden owner only receives messages.
       GetMessage yields while Program Manager and other startup apps run. */
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    muteAll(); if (soundOwned) CloseSound();
    soundOwned = 0; testReport("COMPLETE"); return msg.wParam;
}
