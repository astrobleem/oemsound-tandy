.8086
JEL MACRO target
 LOCAL skip
 jne skip
 jmp target
skip:
ENDM
JNEL MACRO target
 LOCAL skip
 je skip
 jmp target
skip:
ENDM
JCL MACRO target
 LOCAL skip
 jnc skip
 jmp target
skip:
ENDM
_TEXT SEGMENT WORD PUBLIC 'CODE'
_TEXT ENDS
_DATA SEGMENT WORD PUBLIC 'DATA'
hiAudio DW 0
initialPeriod DW 23864
hookInstalled DW 0
enableCalls DW 0
disableCalls DW 0
hookFault DW 0
lastDisFlags DW 0
lastEnFlags DW 0
enableBytes DW 3 DUP(0)
disableBytes DW 3 DUP(0)
_DATA ENDS
DGROUP GROUP _DATA
_TEXT SEGMENT
ASSUME CS:_TEXT,DS:DGROUP
EXTRN TIMERENTRY:FAR
EXTRN _TCBassSuspend:NEAR
PUBLIC _HiPrepare,_HiProgram,_HiPeriod,_HiFind,_HiInstall,_HiStop,_HiRead,_HiStatus,_HiMetric,_HiLayer
PUBLIC _HiRestart,_HiDetach,_HiAbandon,_HiAudio,_HookMetric
brokerPtr DD 0
systemSeg DW 0
systemData DW 0
enableTail DW 407h,0
disableTail DW 433h,0
_HiFind PROC NEAR
 push bx
 push cx
 push di
 push es
 mov ax,0d7a0h
 mov bx,5451h
 int 2fh
 cmp ax,0d7afh
 JNEL find_bad
 cmp cx,2
 JNEL find_bad
 mov word ptr cs:brokerPtr,di
 mov word ptr cs:brokerPtr+2,es
 xor ax,ax
 jmp find_done
find_bad:
 mov ax,1
find_done:
 pop es
 pop di
 pop cx
 pop bx
 ret
_HiFind ENDP
_HookAttach PROC NEAR
 pushf
 cli
 push es
 push si
 push di
 push cx
 cmp hookInstalled,0
 JNEL attach_good
 mov ax,cs:systemSeg
 mov es,ax
 cmp word ptr es:[401h],802eh
 JNEL attach_bad
 cmp word ptr es:[403h],7e3eh
 JNEL attach_bad
 cmp word ptr es:[405h],0002h
 JNEL attach_bad
 cmp word ptr es:[42dh],2e1eh
 JNEL attach_bad
 cmp word ptr es:[42fh],1e8eh
 JNEL attach_bad
 cmp word ptr es:[431h],000fh
 JNEL attach_bad
 mov ax,es:[0fh]
 mov cs:systemData,ax
 mov ax,cs:systemSeg
 mov word ptr cs:enableTail+2,ax
 mov word ptr cs:disableTail+2,ax
 mov si,OFFSET enableBytes
 mov di,401h
 mov cx,3
save_en:
 mov ax,es:[di]
 mov [si],ax
 inc si
 inc si
 inc di
 inc di
 loop save_en
 mov si,OFFSET disableBytes
 mov di,42dh
 mov cx,3
save_dis:
 mov ax,es:[di]
 mov [si],ax
 inc si
 inc si
 inc di
 inc di
 loop save_dis
 mov byte ptr es:[401h],0eah
 mov word ptr es:[402h],OFFSET ENABLEHOOK
 mov ax,cs
 mov word ptr es:[404h],ax
 mov byte ptr es:[406h],90h
 mov byte ptr es:[42dh],0eah
 mov word ptr es:[42eh],OFFSET DISABLEHOOK
 mov word ptr es:[430h],ax
 mov byte ptr es:[432h],90h
 mov hookInstalled,1
 mov enableCalls,0
 mov disableCalls,0
 mov hookFault,0
attach_good:
 xor ax,ax
 jmp attach_done
attach_bad:
 mov ax,1
attach_done:
 pop cx
 pop di
 pop si
 pop es
 popf
 ret
_HookAttach ENDP
_RemoveHooks PROC NEAR
 pushf
 cli
 push es
 push si
 push di
 push cx
 cmp hookInstalled,0
 JEL remove_good
 mov ax,cs:systemSeg
 mov es,ax
 cmp byte ptr es:[401h],0eah
 JNEL remove_bad
 cmp word ptr es:[402h],OFFSET ENABLEHOOK
 JNEL remove_bad
 mov ax,cs
 cmp word ptr es:[404h],ax
 JNEL remove_bad
 cmp byte ptr es:[42dh],0eah
 JNEL remove_bad
 cmp word ptr es:[42eh],OFFSET DISABLEHOOK
 JNEL remove_bad
 cmp word ptr es:[430h],ax
 JNEL remove_bad
 cld
 mov si,OFFSET enableBytes
 mov di,401h
 mov cx,3
 rep movsw
 mov si,OFFSET disableBytes
 mov di,42dh
 mov cx,3
 rep movsw
 mov hookInstalled,0
remove_good:
 xor ax,ax
 jmp remove_done
remove_bad:
 mov hookFault,1
 mov ax,1
remove_done:
 pop cx
 pop di
 pop si
 pop es
 popf
 ret
_RemoveHooks ENDP
_HiInstall PROC NEAR
 push bp
 mov bp,sp
 push bx
 push cx
 push si
 push es
 call _HiFind
 or ax,ax
 JNEL install_done
 mov ax,[bp+4]
 mov cs:systemSeg,ax
 mov es,ax
 cmp byte ptr es:[27eh],1
 JEL install_enabled
 mov ax,1
 jmp install_done
install_enabled:
 call _HookAttach
 or ax,ax
 JNEL install_done
 mov bx,cs
 mov ax,16
 call dword ptr cs:brokerPtr
 or ax,ax
 JNEL install_done
 mov bx,cs:systemSeg
 mov ax,4
 call dword ptr cs:brokerPtr
 mov bx,initialPeriod
 mov ax,13
 call dword ptr cs:brokerPtr
 mov bx,cs:systemSeg
 mov dx,cs
 mov si,OFFSET TIMERENTRY
 mov cx,hiAudio
 mov ax,1
 call dword ptr cs:brokerPtr
install_done:
 pop es
 pop si
 pop cx
 pop bx
 pop bp
 ret
_HiInstall ENDP
_HiRestart PROC NEAR
 push bx
 push cx
 push si
 mov bx,initialPeriod
 mov ax,13
 call dword ptr cs:brokerPtr
 mov bx,cs:systemSeg
 mov dx,cs
 mov si,OFFSET TIMERENTRY
 mov cx,hiAudio
 mov ax,8
 call dword ptr cs:brokerPtr
 pop si
 pop cx
 pop bx
 ret
_HiRestart ENDP
_HiPrepare PROC NEAR
 push bp
 mov bp,sp
 mov ax,[bp+4]
 mov initialPeriod,ax
 pop bp
 ret
_HiPrepare ENDP
_HiProgram PROC NEAR
 push bp
 mov bp,sp
 push bx
 mov bx,[bp+4]
 mov ax,14
 call dword ptr cs:brokerPtr
 pop bx
 pop bp
 ret
_HiProgram ENDP
_HiPeriod PROC NEAR
 mov ax,15
 call dword ptr cs:brokerPtr
 ret
_HiPeriod ENDP
_HiAudio PROC NEAR
 push bp
 mov bp,sp
 mov ax,[bp+4]
 mov hiAudio,ax
 pop bp
 ret
_HiAudio ENDP
_HiStop PROC NEAR
 mov ax,3
 call dword ptr cs:brokerPtr
 ret
_HiStop ENDP
_HiStatus PROC NEAR
 mov ax,0
 call dword ptr cs:brokerPtr
 ret
_HiStatus ENDP
_HiRead PROC NEAR
 mov ax,6
 call dword ptr cs:brokerPtr
 ret
_HiRead ENDP
_HiMetric PROC NEAR
 push bp
 mov bp,sp
 push bx
 mov bx,[bp+4]
 mov ax,5
 call dword ptr cs:brokerPtr
 pop bx
 pop bp
 ret
_HiMetric ENDP
_HookMetric PROC NEAR
 push bp
 mov bp,sp
 mov ax,[bp+4]
 cmp ax,0
 JNEL hm1
 mov ax,hookInstalled
 jmp hm_done
hm1:
 cmp ax,1
 JNEL hm2
 mov ax,disableCalls
 jmp hm_done
hm2:
 cmp ax,2
 JNEL hm3
 mov ax,enableCalls
 jmp hm_done
hm3:
 cmp ax,4
 JNEL hm4
 mov ax,lastDisFlags
 jmp hm_done
hm4:
 cmp ax,5
 JNEL hm5
 mov ax,lastEnFlags
 jmp hm_done
hm5:
 mov ax,hookFault
hm_done:
 pop bp
 ret
_HookMetric ENDP
_HiLayer PROC NEAR
 push bp
 mov bp,sp
 push bx
 mov bx,[bp+4]
 mov ax,12
 call dword ptr cs:brokerPtr
 pop bx
 pop bp
 ret
_HiLayer ENDP
_HiDetach PROC NEAR
 push bx
 cmp hookInstalled,0
 JEL detach_good
 call _HiStatus
 or ax,ax
 JNEL detach_bad
 mov ax,10
 call dword ptr cs:brokerPtr
 or ax,ax
 JNEL detach_bad
 call _RemoveHooks
 or ax,ax
 JNEL detach_bad
 mov bx,cs
 mov ax,9
 call dword ptr cs:brokerPtr
 pop bx
 ret
detach_bad:
 mov ax,1
 pop bx
 ret
detach_good:
 xor ax,ax
 pop bx
 ret
_HiDetach ENDP
_HiAbandon PROC NEAR
 push bx
 cmp hookInstalled,0
 JEL abandon_done
 mov ax,2
 call dword ptr cs:brokerPtr
 call _RemoveHooks
 or ax,ax
 JNEL abandon_done
 mov bx,cs
 mov ax,17
 call dword ptr cs:brokerPtr
abandon_done:
 pop bx
 ret
_HiAbandon ENDP
; Both hooks mask ONLY IRQ0 across the stock DOS vector update. No waiting,
; allocation or Windows APIs are used. Other PIC mask bits are retained.
DISABLEHOOK PROC FAR
 pushf
 cli
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push bp
 push ds
 push es
 mov ax,DGROUP
 mov ds,ax
 mov bp,sp
 mov ax,ss:[bp+18]
 mov lastDisFlags,ax
 inc disableCalls
 ; Fixed capability call only; speaker is muted before the DOS transition.
 call _TCBassSuspend
 in al,21h
 and ax,1
 push ax
 in al,21h
 or al,1
 out 21h,al
 mov ax,2
 call dword ptr cs:brokerPtr
 call FAR PTR ORIGDISABLE
 pop bx
 in al,21h
 and al,0feh
 or al,bl
 out 21h,al
 pop es
 pop ds
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 popf
 retf
DISABLEHOOK ENDP
ENABLEHOOK PROC FAR
 pushf
 cli
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push bp
 push ds
 push es
 mov ax,DGROUP
 mov ds,ax
 mov bp,sp
 mov ax,ss:[bp+18]
 mov lastEnFlags,ax
 inc enableCalls
 in al,21h
 and ax,1
 push ax
 in al,21h
 or al,1
 out 21h,al
 call FAR PTR ORIGENABLE
 mov bx,cs:systemSeg
 mov ax,4
 call dword ptr cs:brokerPtr
 pop bx
 in al,21h
 and al,0feh
 or al,bl
 out 21h,al
 pop es
 pop ds
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 popf
 retf
ENABLEHOOK ENDP
; Source-matched six-byte prefix semantics, with original SYSTEM segment
; references made explicit. Remaining stock code executes in its own CS.
ORIGENABLE PROC FAR
 push ax
 push es
 mov ax,cs:systemSeg
 mov es,ax
 cmp byte ptr es:[27eh],0
 pop es
 pop ax
 jmp dword ptr cs:enableTail
ORIGENABLE ENDP
ORIGDISABLE PROC FAR
 push ds
 push ax
 mov ax,cs:systemData
 mov ds,ax
 pop ax
 jmp dword ptr cs:disableTail
ORIGDISABLE ENDP
_TEXT ENDS
END
