/* Optional XP-note startup chime for known Tandy PSG hardware.
   Windows 3.0 real mode / 8086. No IRQ changes; optional PIT2 tone through the shared owner. */
#define WINVER 0x0300
#include <windows.h>
#include "SNDOWN.H"
#define TIMER 1
static DWORD began;
static unsigned phase;
static int active;
/* User-supplied Bill Brown score, arranged by David Caldarella.
   Quarter = 100, 6/8: Eb6 Eb5 Bb5 Ab5 Eb6 Bb6.
   PSG periods round 3579545 / (32 * equal-tempered Hz). */
static unsigned notes[6] = { 90, 180, 120, 135, 90, 60 };
static unsigned onsets[6] = { 0, 300, 450, 900, 1500, 1800 };
static void psgByte(unsigned char value)
{
    if(!soundOwned)return;
    ++soundWrites;
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
static void tone(unsigned voice, unsigned period, unsigned attenuation)
{
    psgByte((unsigned char)(0x80 | (voice << 5) | (period & 15)));
    psgByte((unsigned char)((period >> 4) & 63));
    psgByte((unsigned char)(0x90 | (voice << 5) | attenuation));
}
static void cleanup(HWND w)
{
    KillTimer(w, TIMER); active = 0; muteAll(); SoundRelease();
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
        if(!SoundAcquire())return -1;
        muteAll();
        if (!SetTimer(w, TIMER, 55, NULL)) {
            cleanup(w);return -1;
        }
        phase = 7; active = 1;
        /* Wait for the first delivered timer, so initial desktop painting
           can finish before any sustained tone begins. No startup wait. */
        return 0;
    case WM_TIMER:
        if (wp != TIMER || !active) return 0;
        if (phase == 7) {
            began = GetTickCount(); phase = 0;
            if(!SoundPitNote(53)){finish(w);return 0;}
            tone(0, notes[0], 6); /* Melody, quieter than the tone demo. */
            tone(1, 269, 10); /* Ab4. */
            tone(2, 641, 11); /* F3: cello F2 raised one octave. */
            return 0;
        }
        if(soundTest>=5) {
            SendMessage(w,soundTest==5?WM_CLOSE:
                soundTest==6?WM_ACTIVATE:WM_KEYDOWN,
                soundTest==7?VK_ESCAPE:0,0L);return 0;
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
        if (phase == 3) {
            if(!SoundPitNote(56)){finish(w);return 0;}
            tone(2, 539, 11);
        } /* Ab3. */
        if (phase == 5) {
            if(!SoundPitNote(51)){finish(w);return 0;}
            tone(1, 285, 10); /* G4. */
            tone(2, 719, 11); /* Eb3. */
        }
        return 0;
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        if (wp) cleanup(w);
        return 0;
    case WM_ACTIVATE:
        if(pitOption && active && LOWORD(wp)==0)finish(w);
        return 0;
    case WM_KEYDOWN:
        if(wp==VK_ESCAPE)finish(w);return 0;
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
    SoundTestArgs(cmd);
    pitOption = pitOption || lstrcmp(cmd,"/pit")==0;
    if(*cmd && !pitOption && !soundTest)return 6;
    if(soundTest==3)soundProbe=OpenSound()>0;
    c.style = 0; c.lpfnWndProc = ChimeProc;
    c.cbClsExtra = 0; c.cbWndExtra = 0; c.hInstance = inst;
    c.hIcon = NULL; c.hCursor = NULL; c.hbrBackground = NULL;
    c.lpszMenuName = NULL; c.lpszClassName = "TandyXPStartupChime";
    if (!RegisterClass(&c)) return 3;
    w = CreateWindow("TandyXPStartupChime", "Tandy XP-note startup chime", WS_POPUP,
        0, 0, 0, 0, NULL, NULL, inst, NULL);
    if (!w) {
        muteAll();SoundRelease();if(soundProbe)CloseSound();
        if(soundTest){SoundAudit();ExitWindows(0L,0);return 0;}
        return 4;
    }
    /* Deliberately no ShowWindow: the hidden owner only receives messages.
       GetMessage yields while Program Manager and other startup apps run. */
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
    muteAll(); SoundRelease();
    if(soundTest){SoundAudit();ExitWindows(0L,0);}
    return msg.wParam;
}
