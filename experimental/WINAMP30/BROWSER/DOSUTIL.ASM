.8086
.MODEL SMALL,PASCAL
.DATA
; Windows reserves DS:0000..000F for its local-heap/atom/stack words.
; Link this module FIRST, before the C module's merged _DATA contents.
PUBLIC WIN16HDR
WIN16HDR LABEL BYTE
 DD 0
 DW 5
 DW 5 DUP (0)
.CODE
EXTRN LIBMAIN:FAR
PUBLIC LIBENTRY
LIBENTRY PROC FAR
 inc bp
 push bp
 mov bp,sp
 push ds
 push di
 push ds
 push cx
 push es
 push si
 call LIBMAIN
 pop ds
 pop bp
 dec bp
 retf
LIBENTRY ENDP
PUBLIC DOSDRIVE,DOSCHDRV,DOSGETDIR,DOSSETDIR,DOSATTR,DOSNEW
DOSDRIVE PROC FAR
 inc bp
 push bp
 mov bp,sp
 push ds
 mov ah,19h
 int 21h
 xor ah,ah
 pop ds
 pop bp
 dec bp
 retf
DOSDRIVE ENDP
DOSCHDRV PROC FAR
 inc bp
 push bp
 mov bp,sp
 push ds
 push dx
 mov dx,[bp+6]
 mov ah,0Eh
 int 21h
 pop dx
 pop ds
 pop bp
 dec bp
 retf 2
DOSCHDRV ENDP
DOSGETDIR PROC FAR
 inc bp
 push bp
 mov bp,sp
 push ds
 push si
 push dx
 mov dx,[bp+6]
 lds si,[bp+8]
 mov ah,47h
 int 21h
 mov ax,0
 jc gdend
 inc ax
gdend:
 pop dx
 pop si
 pop ds
 pop bp
 dec bp
 retf 6
DOSGETDIR ENDP
DOSSETDIR PROC FAR
 inc bp
 push bp
 mov bp,sp
 push ds
 push dx
 lds dx,[bp+6]
 mov ah,3Bh
 int 21h
 mov ax,0
 jc sdend
 inc ax
sdend:
 pop dx
 pop ds
 pop bp
 dec bp
 retf 4
DOSSETDIR ENDP
DOSATTR PROC FAR
 inc bp
 push bp
 mov bp,sp
 push ds
 push dx
 push cx
 lds dx,[bp+6]
 mov ax,4300h
 int 21h
 jc aerr
 mov ax,cx
 jmp aend
aerr:
 mov ax,-1
aend:
 pop cx
 pop dx
 pop ds
 pop bp
 dec bp
 retf 4
DOSATTR ENDP
DOSNEW PROC FAR
 inc bp
 push bp
 mov bp,sp
 push ds
 push dx
 push cx
 lds dx,[bp+6]
 xor cx,cx
 mov ax,5B00h
 int 21h
 jnc newend
 mov ax,-1
newend:
 pop cx
 pop dx
 pop ds
 pop bp
 dec bp
 retf 4
DOSNEW ENDP
END LIBENTRY
