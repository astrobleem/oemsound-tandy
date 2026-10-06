#ifndef TNOTICE_H
#define TNOTICE_H

/* Include windows.h first. Export NoticeProc in each linked application's DEF. */
void TinyNotice(HWND owner, HINSTANCE instance, char *title, char *text);
BOOL FAR PASCAL NoticeProc(HWND window, UINT message,
                          WPARAM wparam, LPARAM lparam);

#endif
