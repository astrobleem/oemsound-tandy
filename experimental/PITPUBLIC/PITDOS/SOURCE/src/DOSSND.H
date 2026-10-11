/* Cooperative foreground DOS player API, MSC6 C89 /G0 /AS cdecl. */
#ifndef TANDY_DOSSND_H
#define TANDY_DOSSND_H
#define DS_PSG 1
#define DS_PIT 2
#define DS_PWM 4
#define DOS_SOUND_VERSION 1
#define DOS_SOUND_BUSY (-3)
#define DOS_SOUND_OWNER (-1)
#define DOS_SOUND_PITCH (-13)
#define DOS_SOUND_CAPABILITY (-16)
#define DOS_SOUND_CONTEXT (-20)
#define DOS_SOUND_SPEECH (-21)
int _cdecl DosSoundAcquire(unsigned resources);
int _cdecl DosSoundPsgByte(unsigned value);
int _cdecl DosSoundPitNote(unsigned note,unsigned attack);
int _cdecl DosSoundPwmBegin(void);
void _cdecl DosSoundPwmEnd(void);
void _cdecl DosSoundRelease(void);
#endif
