.8086
_TEXT SEGMENT WORD PUBLIC 'CODE'
ASSUME CS:_TEXT
PUBLIC LibEntry
LibEntry PROC FAR
    mov ax,1
    retf
LibEntry ENDP
_TEXT ENDS
END LibEntry
