.8086
_TEXT SEGMENT WORD PUBLIC 'CODE'
PUBLIC _PitSilence
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
_PitSilence PROC NEAR
    pushf
    cli
    in al,61h
    and al,0fch
    out 61h,al
    popf
    ret
_PitSilence ENDP
_TEXT ENDS
END LibEntry
