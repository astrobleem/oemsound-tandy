.8086
_TEXT SEGMENT WORD PUBLIC 'CODE'
ASSUME CS:_TEXT
PUBLIC LibEntry, _PsgByte
LibEntry PROC FAR
    mov ax,1
    retf
LibEntry ENDP
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
_TEXT ENDS
END LibEntry
