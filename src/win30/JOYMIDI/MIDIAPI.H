/* Bounded direct-driver MIDI-message harness, not MMSYSTEM. */
#define MODM_GETNUMDEVS 1
#define MODM_GETDEVCAPS 2
#define MODM_OPEN 3
#define MODM_CLOSE 4
#define MODM_DATA 7
#define MODM_LONGDATA 8
#define MODM_RESET 9
#define MMSYSERR_ERROR 1
#define MMSYSERR_BADDEVICEID 2
#define MMSYSERR_ALLOCATED 4
#define MMSYSERR_NOTSUPPORTED 8
#define MMSYSERR_INVALPARAM 11
#define MIDIERR_INVALIDSETUP 69
#define MIDI_COOKIE 0x544dUL
/* Status byte in bits 0..7, first data byte 8..15, second 16..23. */
typedef DWORD (FAR PASCAL *MIDIPROC)(WORD,WORD,DWORD,DWORD,DWORD);
typedef WORD (FAR PASCAL *DEBUGPROC)(WORD);
typedef struct {
    WORD wMid,wPid,vDriverVersion;
    char szPname[32];
    WORD wTechnology,wVoices,wNotes,wChannelMask;
    DWORD dwSupport;
} TCAPS;
typedef struct { WORD hMidi; DWORD dwCallback,dwInstance; } TOPEN;
