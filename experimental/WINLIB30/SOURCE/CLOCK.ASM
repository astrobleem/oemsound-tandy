.8086
_TEXT SEGMENT WORD PUBLIC 'CODE'
_TEXT ENDS
_DATA SEGMENT WORD PUBLIC 'DATA'
EXTRN _timer:WORD,_probeTimer:WORD,_active:WORD,_error:WORD,_owner:WORD
_DATA ENDS
DGROUP GROUP _DATA
_TEXT SEGMENT
ASSUME CS:_TEXT,DS:DGROUP
EXTRN _Tick:NEAR,_HiFind:NEAR,_HiAbandon:NEAR
EXTRN KILLSYSTEMTIMER:FAR
PUBLIC WEP
PUBLIC NullEntry,LibEntry,TIMERENTRY,_SaveIRQ,_RestoreIRQ,_PsgByte,_ReadStamp
LibEntry PROC FAR
 call _HiFind
 or ax,ax
 jne lib_bad
 mov ax,1
 retf
lib_bad:
 xor ax,ax
 retf
LibEntry ENDP
NullEntry PROC FAR
 retf
NullEntry ENDP
TIMERENTRY PROC FAR
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
 call _Tick
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
TIMERENTRY ENDP
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
_PsgByte PROC NEAR
 push bp
 mov bp,sp
 mov ax,[bp+4]
 out 0c0h,al
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 pop bp
 ret
_PsgByte ENDP
; Atomic read of the BIOS-maintained tick count; no PIT/PIC/IRQ writes.
_ReadStamp PROC NEAR
 pushf
 cli
 push es
 mov ax,40h
 mov es,ax
 mov ax,es:[6ch]
 mov dx,es:[6eh]
 pop es
 popf
 ret
_ReadStamp ENDP
; No loader-time cleanup APIs: active Windows timers pin this DLL.
; Foreground owner Close/Kill releases the timer pin before actual unload.
WEP PROC FAR
 pushf
 push bp
 mov bp,sp
 ; Free-DLL calls cannot disarm an active owner. Timer pin prevents unload.
 cmp word ptr ss:[bp+8],1
 je wep_done
 push bx
 push cx
 push dx
 push si
 push di
 push ds
 push es
 mov ax,DGROUP
 mov ds,ax
 cld
 call _HiAbandon
 pop es
 pop ds
 pop di
 pop si
 pop dx
 pop cx
 pop bx
wep_done:
 pop bp
 popf
 mov ax,1
 retf 2
WEP ENDP
_TEXT ENDS
END LibEntry
