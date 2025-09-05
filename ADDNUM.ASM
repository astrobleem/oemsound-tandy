; ADDNUM.ASM - simple function to add two integers
.MODEL small
.CODE
PUBLIC _add_numbers
_add_numbers PROC
    push bp
    mov bp, sp
    mov ax, [bp+4]
    add ax, [bp+6]
    pop bp
    ret
_add_numbers ENDP
END
