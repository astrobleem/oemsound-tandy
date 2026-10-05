/* Optional preloaded absolute-movie-time captions. MSC 6 /G0 /AS. */
#ifndef CAPTION_H
#define CAPTION_H
#define CAP_MAX 256
#define CAP_WIDTH 52
/* 0: absent; 1: loaded; -1: malformed/read error. Never fatal to playback. */
int CaptionLoad(char *name, unsigned long duration);
/* Call only in graphics mode, outside sampled speech. Callback services music. */
int CaptionService(unsigned long now, int (*music)(void));
/* Next unconsumed absolute timestamp, or 0xffffffffUL if none. */
unsigned long CaptionNextTime(void);
unsigned CaptionCount(void);
unsigned CaptionUpdates(void);
#endif
