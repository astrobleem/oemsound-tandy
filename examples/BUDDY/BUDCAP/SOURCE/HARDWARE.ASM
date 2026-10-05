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
.DATA
EXTRN _interrupted:WORD, _pcmTickBase:WORD, _pcmTickLimit:WORD
PUBLIC _Late
_Late DW 0
PUBLIC _PCMReason
_PCMReason DW 0
saved61 DB 0
left DW 0
played DW 0
.CODE
PUBLIC _SpeakerPort
_SpeakerPort PROC NEAR
 in al,61h
 xor ah,ah
 ret
_SpeakerPort ENDP
PUBLIC _ValidatePCM
_ValidatePCM PROC NEAR
 push bp
 mov bp,sp
 push si
 push es
 mov si,[bp+4]
 mov ax,[bp+6]
 mov es,ax
 mov cx,[bp+8]
ValidateNext:
 mov al,es:[si]
 sub al,1
 cmp al,71
 ja ValidateBad
 inc si
 loop ValidateNext
 mov ax,1
 jmp SHORT ValidateDone
ValidateBad:
 xor ax,ax
ValidateDone:
 pop es
 pop si
 pop bp
 ret
_ValidatePCM ENDP
PUBLIC _PlayPCM
_PlayPCM PROC NEAR
 push bp
 mov bp,sp
 push bx
 push si
 push di
 push es
 mov si,[bp+4]
 mov ax,[bp+6]
 mov es,ax
 mov ax,[bp+8]
 mov left,ax
 mov played,0
 mov _Late,0
 mov _PCMReason,0
 in al,61h
 mov saved61,al
 and al,0fch
 out 61h,al
 ; 8253 channel 2 mode 0, low byte. Reload creates a timed low pulse.
 mov al,090h
 out 43h,al
 mov al,1
 out 42h,al
 in al,61h
 or al,3
 out 61h,al
 call ReadPit
 mov di,ax
SampleWait:
 call ReadPit
 mov bx,di
 sub bx,ax
 cmp bx,398
 jb SampleWait
 cmp bx,796
 jb OnTime
 inc _Late
 mov di,ax
 jmp SHORT Emit
OnTime:
 sub di,398
Emit:
 mov al,es:[si]
 inc si
 out 42h,al
 inc played
 dec left
 jz Finished
 test played,63
 jnz SampleWait
 push es
 mov ax,40h
 mov es,ax
 mov ax,es:[6ch]
 pop es
 sub ax,_pcmTickBase
 cmp ax,_pcmTickLimit
 jae Deadline
 cmp _interrupted,0
 jne UserStop
 mov ah,1
 int 16h
 jz SampleWait
 mov ah,0
 int 16h
 cmp al,27
 je UserStop
 cmp al,3
 je UserStop
 cmp al,32
 jne SampleWait
UserStop:
 mov _PCMReason,1
 jmp SHORT Finished
Deadline:
 mov _PCMReason,2
Finished:
 mov al,saved61
 and al,0fch
 out 61h,al
 ; Conventional DOS inactive speaker baseline (PIT2 mode 3 divisor 0).
 mov al,0b6h
 out 43h,al
 xor al,al
 out 42h,al
 out 42h,al
 mov ax,played
 pop es
 pop di
 pop si
 pop bx
 pop bp
 ret
_PlayPCM ENDP
ReadPit PROC NEAR
 xor al,al
 out 43h,al
 in al,40h
 mov ah,al
 in al,40h
 xchg ah,al
 ret
ReadPit ENDP
END
