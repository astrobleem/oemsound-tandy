.8086
_TEXT SEGMENT WORD PUBLIC 'CODE'
_TEXT ENDS
_DATA SEGMENT WORD PUBLIC 'DATA'
_DATA ENDS
DGROUP GROUP _DATA
_TEXT SEGMENT
ASSUME CS:_TEXT,DS:DGROUP
EXTRN _SpeakerTick:NEAR
PUBLIC LibEntry,TimerEntry,_SaveIRQ,_RestoreIRQ,_TicksMS,WEP,_small_code_
_small_code_ LABEL BYTE
LibEntry PROC FAR
 mov ax,1
 retf
LibEntry ENDP
TimerEntry PROC FAR
 pushf
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push bp
 push ds
 push es
 mov ax,DGROUP
 mov ds,ax
 cld
 call _SpeakerTick
 pop es
 pop ds
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 popf
 retf
TimerEntry ENDP
_SaveIRQ PROC NEAR
 pushf
 pop ax
 cli
 ret
_SaveIRQ ENDP
_RestoreIRQ PROC NEAR
 push bp
 mov bp,sp
 push word ptr [bp+4]
 popf
 pop bp
 ret
_RestoreIRQ ENDP
; 54 + 60647/65536 ms per tick: rate error below 0.02 ms/ten minutes;
; integer timestamps round down by less than one millisecond.
; Two bounded 8086 multiplies, no division or runtime helper.
_TicksMS PROC NEAR
 push bp
 mov bp,sp
 push bx
 mov bx,[bp+4]
 mov ax,bx
 mov cx,60647
 mul cx
 mov cx,dx
 mov ax,bx
 mov bx,54
 mul bx
 add ax,cx
 adc dx,0
 pop bx
 pop bp
 ret
_TicksMS ENDP
WEP PROC FAR
 mov ax,1
 retf 2
WEP ENDP
_TEXT ENDS
END LibEntry
