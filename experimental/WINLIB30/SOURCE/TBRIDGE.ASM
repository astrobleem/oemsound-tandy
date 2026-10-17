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
JBL MACRO target
 LOCAL skip
 jae skip
 jmp target
skip:
ENDM
_STACK SEGMENT PARA STACK 'STACK'
 DW 128 DUP(0)
_STACK ENDS
_TEXT SEGMENT PARA PUBLIC 'CODE'
ASSUME CS:_TEXT,DS:_TEXT
oldBios DD 0
oldMux DD 0
winVec DD 0
callback DD 0
state DW 0
online DW 0
audio DW 0
phase DW 0
clockLo DW 0
clockHi DW 0
irqs DW 0
chains DW 0
fault DW 0
chainTag DW 0
prepared DW 23864
currentPeriod DW 23864
pendingPeriod DW 23864
nextPeriod DW 23864
periodDirty DW 0
residentPSP DW 0
rootParent DW 0
clientRef DW 0
PUBLIC BASEENTRY,TOPENTRY,APIENTRY
MUXENTRY PROC FAR
 cmp ax,0d7a0h
 JNEL mux_old
 cmp bx,5451h
 JNEL mux_old
 mov ax,0d7afh
 mov cx,2
 push cs
 pop es
 mov di,OFFSET APIENTRY
 iret
mux_old:
 jmp dword ptr cs:oldMux
MUXENTRY ENDP
; Register API, no DOS/Windows calls. IRQ-safe and resident for DOS handoff.
; AX operation; BX system segment/debug selector; DX:SI callback; CX audio flag.
; Returns AX result / DX auxiliary. Other registers preserved.
APIENTRY PROC FAR
 pushf
 cli
 push bx
 push cx
 push si
 push di
 push bp
 push ds
 push es
 push cs
 pop ds
 cmp ax,0
 JEL api_status
 cmp ax,1
 JEL api_begin
 cmp ax,2
 JEL api_offline
 cmp ax,3
 JEL api_stop
 cmp ax,4
 JEL api_online
 cmp ax,5
 JEL api_debug
 cmp ax,6
 JEL api_read
 cmp ax,7
 JEL api_unlink
 cmp ax,8
 JEL api_resume
 cmp ax,9
 JEL api_drop
 cmp ax,10
 JEL api_callback
 cmp ax,12
 JEL api_layer
 cmp ax,13
 JEL api_prepare
 cmp ax,14
 JEL api_program
 cmp ax,15
 JEL api_period
 cmp ax,16
 JEL api_attach
 cmp ax,17
 JEL api_release
 mov ax,1
 jmp api_done
api_attach:
 cmp clientRef,0
 JEL attach_client_ok
 cmp clientRef,bx
 JNEL api_bad
attach_client_ok:
 mov clientRef,bx
 jmp api_ok
api_release:
 cmp clientRef,bx
 JNEL api_bad
 cmp word ptr callback+2,0
 JNEL api_bad
 mov clientRef,0
 jmp api_ok
api_prepare:
 cmp bx,9000
 JBL api_bad
 mov prepared,bx
 jmp api_ok
api_program:
 cmp bx,9000
 JBL api_bad
 mov nextPeriod,bx
 mov periodDirty,1
 jmp api_ok
api_period:
 mov ax,currentPeriod
 jmp api_done
api_status:
 mov ax,state
 mov dx,online
 jmp api_done
api_begin:
 cmp state,0
 JNEL api_bad
 mov clockLo,0
 mov clockHi,0
 mov irqs,0
 mov chains,0
 mov fault,0
 jmp begin_common
api_resume:
 cmp state,0
 JNEL api_bad
begin_common:
 cmp online,1
 JNEL api_bad
 mov bp,dx
 xor ax,ax
 mov es,ax
 cmp word ptr es:[20h],354h
 JNEL begin_resident_vec
 cmp es:[22h],bx
 JNEL api_bad
 jmp begin_vec_ok
begin_resident_vec:
 mov ax,cs
 cmp es:[22h],ax
 JNEL api_bad
 cmp word ptr es:[20h],OFFSET TOPENTRY
 JEL begin_vec_ok
 cmp word ptr es:[20h],OFFSET BASEENTRY
 JNEL api_bad
begin_vec_ok:
 mov word ptr winVec,354h
 mov word ptr winVec+2,bx
 mov word ptr callback,si
 mov word ptr callback+2,bp
 mov audio,cx
 mov phase,0
 mov ax,prepared
 mov currentPeriod,ax
 mov pendingPeriod,ax
 mov nextPeriod,ax
 mov periodDirty,0
 mov state,1
 mov word ptr es:[20h],OFFSET TOPENTRY
 mov ax,cs
 mov es:[22h],ax
 jmp api_ok
api_offline:
 mov online,0
 mov word ptr winVec,0
 mov word ptr winVec+2,0
 call silence
 mov word ptr callback,0
 mov word ptr callback+2,0
 cmp state,1
 JNEL offline_fast
 mov state,0
 jmp offline_vec
offline_fast:
 cmp state,2
 JNEL offline_vec
 mov state,3
offline_vec:
 xor ax,ax
 mov es,ax
 mov word ptr es:[20h],OFFSET BASEENTRY
 mov ax,cs
 mov es:[22h],ax
 jmp api_ok
api_stop:
 cmp state,0
 JNEL stop_active
 call restore_vector
 jmp api_ok
stop_active:
 xor ax,ax
 mov es,ax
 cmp word ptr es:[20h],OFFSET TOPENTRY
 JNEL api_bad
 mov ax,cs
 cmp es:[22h],ax
 JNEL api_bad
 call silence
 mov word ptr callback,0
 mov word ptr callback+2,0
 cmp state,1
 JNEL stop_fast
 mov state,0
 call restore_vector
 jmp api_ok
stop_fast:
 cmp state,2
 JNEL api_ok
 mov state,3
 jmp api_ok
api_online:
 mov online,1
 mov word ptr winVec,354h
 mov word ptr winVec+2,bx
 cmp state,0
 JEL api_ok
 xor ax,ax
 mov es,ax
 ; Caller masks IRQ0 around stock Enable, then reinstalls resident top.
 mov word ptr es:[20h],OFFSET TOPENTRY
 mov ax,cs
 mov es:[22h],ax
 jmp api_ok
api_unlink:
 cmp clientRef,0
 JNEL api_bad
 cmp bx,rootParent
 JNEL api_bad
 cmp state,0
 JNEL api_bad
 cmp online,0
 JNEL api_bad
 cmp word ptr callback+2,0
 JNEL api_bad
 xor ax,ax
 mov es,ax
 mov ax,cs
 cmp es:[22h],ax
 JNEL api_bad
 cmp word ptr es:[20h],OFFSET BASEENTRY
 JNEL api_bad
 cmp es:[0beh],ax
 JNEL api_bad
 cmp word ptr es:[0bch],OFFSET MUXENTRY
 JNEL api_bad
 mov ax,word ptr oldBios
 mov es:[20h],ax
 mov ax,word ptr oldBios+2
 mov es:[22h],ax
 mov ax,word ptr oldMux
 mov es:[0bch],ax
 mov ax,word ptr oldMux+2
 mov es:[0beh],ax
 mov dx,residentPSP
 jmp api_ok
api_drop:
 cmp clientRef,bx
 JNEL api_bad
 cmp state,0
 JNEL api_bad
 cmp word ptr callback+2,0
 JNEL api_bad
 mov online,0
 mov word ptr winVec,0
 mov word ptr winVec+2,0
 mov audio,0
 mov clientRef,0
 jmp api_ok
api_callback:
 mov ax,word ptr callback+2
 mov dx,word ptr callback
 jmp api_done
api_read:
 mov ax,clockLo
 mov dx,clockHi
 jmp api_done
api_debug:
 xor dx,dx
 cmp bx,0
 JEL api_status
 cmp bx,1
 JNEL dbg2
 mov ax,irqs
 jmp api_done
dbg2:
 cmp bx,2
 JNEL dbg3
 mov ax,chains
 jmp api_done
dbg3:
 cmp bx,3
 JNEL dbg8
 mov ax,phase
 jmp api_done
dbg8:
 cmp bx,8
 JEL dbg_online
 cmp bx,9
 JEL api_callback
 cmp bx,13
 JNEL dbg14
 mov ax,cs
 jmp api_done
dbg14:
 cmp bx,14
 JNEL dbg15
 mov ax,clientRef
 jmp api_done
dbg15:
 cmp bx,15
 JNEL dbg16
 mov ax,rootParent
 jmp api_done
dbg16:
 mov ax,fault
 jmp api_done
dbg_online:
 mov ax,online
 jmp api_done
api_layer:
 xor ax,ax
 mov es,ax
 mov ax,cs
 cmp es:[22h],ax
 JNEL api_bad
 cmp bx,0
 JEL layer_remove
 cmp word ptr es:[20h],OFFSET TOPENTRY
 JNEL api_bad
 mov word ptr es:[20h],OFFSET LAYERENTRY
 jmp api_ok
layer_remove:
 cmp word ptr es:[20h],OFFSET LAYERENTRY
 JNEL api_bad
 mov word ptr es:[20h],OFFSET TOPENTRY
 jmp api_ok
api_bad:
 mov ax,1
 jmp api_done
api_ok:
 xor ax,ax
api_done:
 pop es
 pop ds
 pop bp
 pop di
 pop si
 pop cx
 pop bx
 popf
 retf
APIENTRY ENDP
silence PROC NEAR
 cmp audio,0
 JEL silence_done
 mov al,9fh
 out 0c0h,al
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 mov al,0bfh
 out 0c0h,al
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 mov al,0dfh
 out 0c0h,al
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 mov al,0ffh
 out 0c0h,al
 nop
 nop
 nop
 nop
 nop
 nop
 nop
 nop
silence_done:
 ret
silence ENDP
restore_vector PROC NEAR
 ; Never discard a foreign top hook. All residual targets are DOS resident.
 xor ax,ax
 mov es,ax
 mov ax,cs
 cmp es:[22h],ax
 JNEL restore_done
 cmp word ptr es:[20h],OFFSET TOPENTRY
 JEL restore_owned
 cmp word ptr es:[20h],OFFSET BASEENTRY
 JNEL restore_done
restore_owned:
 cmp online,0
 JEL restore_base
 mov ax,word ptr winVec
 mov es:[20h],ax
 mov ax,word ptr winVec+2
 mov es:[22h],ax
 ret
restore_base:
 mov word ptr es:[20h],OFFSET BASEENTRY
 mov ax,cs
 mov es:[22h],ax
restore_done:
 ret
restore_vector ENDP
LAYERENTRY PROC FAR
 jmp NEAR PTR TOPENTRY
LAYERENTRY ENDP
BASEENTRY PROC FAR
 cmp word ptr cs:chainTag,0
 JEL base_direct
 mov word ptr cs:chainTag,0
 jmp dword ptr cs:oldBios
base_direct:
 cmp word ptr cs:state,0
 JNEL <NEAR PTR TOPENTRY>
 jmp dword ptr cs:oldBios
BASEENTRY ENDP
TOPENTRY PROC FAR
 push ax
 push bx
 push cx
 push dx
 push si
 push di
 push bp
 push ds
 push es
 push cs
 pop ds
 cld
 cmp state,0
 JEL do_chain
 inc irqs
 cmp state,1
 JEL first_edge
 cmp state,4
 JEL last_edge
 mov ax,currentPeriod
 cmp word ptr callback+2,0
 JEL no_audio_clock
 add clockLo,ax
 adc clockHi,0
no_audio_clock:
 add phase,ax
 pushf
 mov ax,pendingPeriod
 mov currentPeriod,ax
 popf
 JCL bios_edge
 cmp state,3
 JEL begin_drain
 cmp word ptr callback+2,0
 JEL local_eoi
 call dword ptr callback
 call publish_period
 jmp local_eoi
bios_edge:
 inc chains
 cmp state,3
 JEL drain_chain
 cmp word ptr callback+2,0
 JEL do_chain
 call dword ptr callback
 call publish_period
 jmp do_chain
drain_chain:
 call drain_pit
 jmp do_chain
begin_drain:
 call drain_pit
local_eoi:
 mov al,20h
 out 20h,al
 pop es
 pop ds
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 iret
first_edge:
 mov al,34h
 out 43h,al
 mov ax,currentPeriod
 out 40h,al
 mov al,ah
 out 40h,al
 mov state,2
 inc chains
 cmp word ptr callback+2,0
 JEL first_done
 call dword ptr callback
 call publish_period
first_done:
 jmp do_chain
last_edge:
 mov al,36h
 out 43h,al
 xor ax,ax
 out 40h,al
 out 40h,al
 mov state,0
 inc chains
 call restore_vector
do_chain:
 cmp online,0
 JEL chain_bios
 mov chainTag,1
 pop es
 pop ds
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 jmp dword ptr cs:winVec
chain_bios:
 pop es
 pop ds
 pop bp
 pop di
 pop si
 pop dx
 pop cx
 pop bx
 pop ax
 jmp dword ptr cs:oldBios
TOPENTRY ENDP
publish_period PROC NEAR
 cmp periodDirty,0
 JEL publish_done
 mov ax,nextPeriod
 mov pendingPeriod,ax
 out 40h,al
 mov al,ah
 out 40h,al
 mov periodDirty,0
publish_done:
 ret
publish_period ENDP
drain_pit PROC NEAR
 mov al,30h
 out 43h,al
 xor ax,ax
 sub ax,phase
 out 40h,al
 mov al,ah
 out 40h,al
 mov state,4
 ret
drain_pit ENDP
ResidentEnd LABEL BYTE
; Nonresident installation program. Only DOS before Windows.
msgOk DB 'TBRIDGE v2 installed',13,10,'$'
msgOld DB 'TBRIDGE v2 already resident',13,10,'$'
msgRemoved DB 'TBRIDGE v2 removed, original BIOS/INT2F vectors restored',13,10,'$'
msgRefused DB 'TBRIDGE removal refused: not quiescent or foreign vector owner',13,10,'$'
removeRequested DW 0
removePtr DD 0
START PROC FAR
 mov si,ds
 push cs
 pop ds
 mov residentPSP,si
 mov es,si
 mov ax,es:[16h]
 mov rootParent,ax
 mov di,81h
scan_cmd:
 mov al,es:[di]
 cmp al,13
 JEL scan_done
 cmp al,'u'
 JEL want_remove
 cmp al,'U'
 JEL want_remove
 inc di
 jmp scan_cmd
want_remove:
 mov removeRequested,1
scan_done:
 mov ax,0d7a0h
 mov bx,5451h
 int 2fh
 cmp ax,0d7afh
 JEL already
 cmp removeRequested,0
 JNEL remove_refused
 mov ax,3508h
 int 21h
 mov word ptr oldBios,bx
 mov word ptr oldBios+2,es
 mov ax,352fh
 int 21h
 mov word ptr oldMux,bx
 mov word ptr oldMux+2,es
 mov dx,OFFSET BASEENTRY
 mov ax,2508h
 int 21h
 mov dx,OFFSET MUXENTRY
 mov ax,252fh
 int 21h
 mov dx,OFFSET msgOk
 mov ah,9
 int 21h
 mov es,residentPSP
 mov ax,es:[2ch]
 or ax,ax
 jz keep
 mov es,ax
 mov ah,49h
 int 21h
keep:
 mov dx,OFFSET ResidentEnd
 add dx,15
 mov cl,4
 shr dx,cl
 mov ax,cs
 sub ax,residentPSP
 add dx,ax
 mov ax,3100h
 int 21h
already:
 cmp cx,2
 JNEL remove_refused
 cmp removeRequested,0
 JEL already_message
 mov word ptr removePtr,di
 mov word ptr removePtr+2,es
 mov bx,rootParent
 mov ax,7
 call dword ptr removePtr
 or ax,ax
 JNEL remove_refused
 mov es,dx
 mov ah,49h
 int 21h
 jc remove_refused
 mov dx,OFFSET msgRemoved
 mov ah,9
 int 21h
 mov ax,4c00h
 int 21h
remove_refused:
 mov dx,OFFSET msgRefused
 mov ah,9
 int 21h
 mov ax,4c02h
 int 21h
already_message:
 mov dx,OFFSET msgOld
 mov ah,9
 int 21h
 mov ax,4c00h
 int 21h
START ENDP
_TEXT ENDS
END START
