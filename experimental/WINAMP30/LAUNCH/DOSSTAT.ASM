.8086
_TEXT SEGMENT WORD PUBLIC 'CODE'
ASSUME CS:_TEXT
PUBLIC _DOSStat,_DOSUnlink
_DOSStat PROC NEAR
 push bp
 mov bp,sp
 push bx
 push cx
 push si
 push di
 push es
 mov ax,0d7a0h
 mov bx,5451h
 int 2fh
 cmp ax,0d7afh
 jne bad
 sub sp,4
 mov bx,sp
 mov ss:[bx],di
 mov ss:[bx+2],es
 mov bx,[bp+4]
 mov ax,5
 push bp
 mov bp,sp
 call dword ptr ss:[bp+2]
 pop bp
 add sp,4
 jmp short done
bad:
 mov ax,0ffffh
done:
 pop es
 pop di
 pop si
 pop cx
 pop bx
 pop bp
 ret
_DOSStat ENDP
_DOSUnlink PROC NEAR
 push bp
 mov bp,sp
 push bx
 push cx
 push si
 push di
 push es
 mov ax,0d7a0h
 mov bx,5451h
 int 2fh
 cmp ax,0d7afh
 jne un_bad
 sub sp,4
 mov bx,sp
 mov ss:[bx],di
 mov ss:[bx+2],es
 mov bx,[bp+4]
 mov ax,7
 push bp
 mov bp,sp
 call dword ptr ss:[bp+2]
 pop bp
 add sp,4
 jmp short un_done
un_bad:
 mov ax,0ffffh
un_done:
 pop es
 pop di
 pop si
 pop cx
 pop bx
 pop bp
 ret
_DOSUnlink ENDP
_TEXT ENDS
END
