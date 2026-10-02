.8086
_TEXT SEGMENT WORD PUBLIC 'CODE'
_TEXT ENDS
_DATA SEGMENT WORD PUBLIC 'DATA'
_DATA ENDS
DGROUP GROUP _DATA
_TEXT SEGMENT
ASSUME CS:_TEXT, DS:DGROUP
EXTRN _Tick:NEAR
PUBLIC LibEntry, TIMERENTRY, _SaveIRQ, _RestoreIRQ, _PsgByte
LibEntry PROC FAR
    mov ax,1
    retf
LibEntry ENDP
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
    jmp short io_delay
io_delay:
    pop bp
    ret
_PsgByte ENDP
_TEXT ENDS
END LibEntry
