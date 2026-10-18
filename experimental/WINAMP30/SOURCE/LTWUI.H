#ifndef LTWUI_H
#define LTWUI_H
/* Include Windows 3.0 WINDOWS.H first. Zero-initialize each batch state. */
typedef struct {HWND list;unsigned depth;} LTWLISTPAINT;
void LtwListBegin(LTWLISTPAINT *state,HWND list);
void LtwListEnd(LTWLISTPAINT *state,HWND currentList);
BOOL LtwSetText(HWND control,LPCSTR text);
#endif
