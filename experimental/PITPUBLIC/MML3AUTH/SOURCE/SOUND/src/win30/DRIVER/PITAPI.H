/* Private opt-in API v1; include windows.h first. Not the Win3 sound ABI.
   Acquire OpenSound before calling. Resolve TANDYPITVOICE from SOUND with
   GetModuleHandle/GetProcAddress; NULL means unavailable, never fallback.
   hz=0 or level=0 stops. Sounding hz=37..7902, level exactly 1.
   No independent volume control. StopSound/CloseSound also stop this voice.
   Negative return: lease unavailable -1, busy speaker -3, pitch -13,
   version/level -16. Standard OpenSound still returns 3 voices.
   Never run this simultaneously with PWM speech on PIT2. */
#ifndef TANDYPIT_API_H
#define TANDYPIT_API_H
#define TANDYPIT_VERSION 1
typedef int (FAR PASCAL *TANDYPITPROC)(unsigned,unsigned,unsigned);
#endif
