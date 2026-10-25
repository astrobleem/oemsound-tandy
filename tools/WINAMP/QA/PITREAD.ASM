.MODEL SMALL
.CODE
PUBLIC _PitBits
_PitBits PROC NEAR
    xor ax,ax
    in al,61h
    ret
_PitBits ENDP
END
