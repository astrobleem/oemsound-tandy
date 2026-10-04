.8086
.MODEL SMALL
.CODE
PUBLIC _ClockTick, _VideoMode, _SetMode, _VideoRow, _PsgByte
PUBLIC _KeyRead, _IsWindows
_ClockTick PROC NEAR
    push es
    mov ax,40h
    mov es,ax
    pushf
    cli
    mov ax,es:[6ch]
    mov dx,es:[6eh]
    popf
    pop es
    ret
_ClockTick ENDP
_VideoMode PROC NEAR
    push bx
    mov ah,0fh
    int 10h
    and ax,7fh
    pop bx
    ret
_VideoMode ENDP
_SetMode PROC NEAR
    push bp
    mov bp,sp
    push bx
    mov ax,[bp+4]
    int 10h
    pop bx
    pop bp
    ret
_SetMode ENDP
_VideoRow PROC NEAR
    push bp
    mov bp,sp
    push si
    push di
    push es
    mov ax,0b800h
    mov es,ax
    mov si,[bp+4]
    mov di,[bp+6]
    mov cx,[bp+8]
    shr cx,1
    cld
    rep movsw
    pop es
    pop di
    pop si
    pop bp
    ret
_VideoRow ENDP
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
_KeyRead PROC NEAR
    mov ah,1
    int 16h
    jz NoKey
    mov ah,0
    int 16h
    xor ah,ah
    ret
NoKey:
    mov ax,0ffffh
    ret
_KeyRead ENDP
_IsWindows PROC NEAR
    push bx
    push si
    push di
    push es
    mov ax,1600h
    int 2fh
    or al,al
    jz CheckReal
    cmp al,80h
    jne FoundWindows
CheckReal:
    mov ax,4680h
    int 2fh
    or ax,ax
    jz FoundWindows
    xor ax,ax
    jmp SHORT WindowsDone
FoundWindows:
    mov ax,1
WindowsDone:
    pop es
    pop di
    pop si
    pop bx
    ret
_IsWindows ENDP
END
