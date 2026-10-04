; 8086/8088, near C calling convention, small model DS.
; One 0201h write resets EX's ramp; 512 uniformly spaced samples follow.
; No CLI/STI, BIOS, PIT/PIC writes, callbacks or unbounded loops.
; Interrupts retain caller state. Windows may add jitter to raw loop counts.
.8086
_TEXT SEGMENT WORD PUBLIC 'CODE'
ASSUME CS:_TEXT
PUBLIC _joy_capture
_joy_capture PROC NEAR
 push bp
 mov bp,sp
 push si
 push dx
 push cx
 mov si,[bp+4]
 mov dx,0201h
 mov cx,512
 xor ax,ax
 out dx,al
capture_next:
 in al,dx
 mov [si],al
 inc si
 loop capture_next
 pop cx
 pop dx
 pop si
 pop bp
 ret
_joy_capture ENDP
_TEXT ENDS
END
