; RESERVE.ASM - protect normal-640K Tandy mode-09 shared video RAM.
; MASM 5.1, .8086. DOS 3.x or newer. Build as COM via EXE2BIN.
; See RESERVE.MD and RESERVE.PY for scope and executable regression tests.
; No MCB writes, BIOS memory-size edits, interrupt hooks, or extra base RAM.
;
; Supported layout: BIOS 624K, conventional arena ends at 9C00h.
; Mode 09 uses 98000h-9FFFFh. BIOS already excludes 9C000h-9FFFFh.
; Keep 9800h:0000-3FFF allocated; its MCB is at 97FFh, OUTSIDE video.
; DOS AH=31h retains both this small owner block and the high allocation.

.8086
code segment para public 'CODE'
assume cs:code,ds:code,es:code,ss:code
org 100h
entry: jmp start
; Filled only after all checks and the allocation have succeeded.
resident_id db 8 dup(0)
resident_video dw 0
resident_count dw 0

; Expand long conditional branches without post-8086 opcodes.
JFAIL macro condition, target
 local taken, done
 condition taken
 jmp short done
taken: jmp target
done:
endm

start:
 cli
 mov sp,offset stack_end
 sti
 push cs
 pop ds
 cld
 mov dx,offset msg_platform
 ; Tandy signature used by this repository's TANBOX.ASM (exclude PCjr).
 mov ax,0ffffh
 mov es,ax
 cmp byte ptr es:[0eh],0ffh
 JFAIL jne,failed
 mov ax,0fc00h
 mov es,ax
 cmp byte ptr es:[0],21h
 JFAIL jne,failed
 mov ax,40h
 mov es,ax
 cmp word ptr es:[13h],624
 JFAIL jne,failed
 ; Do not start while the overlapping video page is already in use.
 mov ah,0fh
 int 10h
 cmp al,3
 JFAIL jne,failed
 mov ah,30h
 int 21h
 cmp al,3
 JFAIL jb,failed

 ; A COM initially owns most free memory. Release its unused tail first.
 ; Code, data and our explicit stack all remain below the video boundary.
 mov ax,cs
 mov bx,KEEP_PARAS
 add ax,bx
 cmp ax,9800h
 mov dx,offset msg_location
 JFAIL ja,failed
 push cs
 pop es
 mov ah,4ah
 int 21h
 mov dx,offset msg_dos
 JFAIL jc,failed

 ; Environment is not needed. Free it BEFORE examining the high end:
 ; an inherited last-fit strategy may have put our own environment there.
 mov ax,ds:[2ch]
 or ax,ax
 jz environment_done
 mov es,ax
 mov ah,49h
 int 21h
 mov dx,offset msg_dos
 JFAIL jc,failed
 mov word ptr ds:[2ch],0
environment_done:

 ; Walk the REAL DOS arena, not just the BIOS size word. Linked UMBs and
 ; unexpected or corrupt layouts fail closed without rewriting the chain.
 mov ah,52h
 int 21h
 cmp bx,2
 mov dx,offset msg_arena
 JFAIL jb,failed
 mov ax,es:[bx-2]
 mov first_mcb,ax
 call validate_arena
 JFAIL jc,failed
 mov ax,last_mcb
 mov es,ax
 cmp ax,97ffh
 jne try_new
 cmp word ptr es:[3],400h
 jne try_new
 cmp word ptr es:[1],0
 je try_new
 call is_our_reservation
 JFAIL jc,occupied
 mov dx,offset msg_already
 mov ah,9
 int 21h
 mov ax,4c00h
 int 21h

try_new:
 cmp word ptr es:[1],0
 JFAIL jne,occupied
 ; A terminal block starting at 9800h would put an MCB in video RAM.
 mov ax,free_run_start
 cmp ax,97ffh
 JFAIL ja,occupied
 mov ax,5800h
 int 21h
 mov dx,offset msg_strategy
 JFAIL jc,failed
 mov old_strategy,ax
 mov ax,5801h
 mov bx,2                       ; conventional LAST FIT
 int 21h
 JFAIL jc,failed
 mov strategy_changed,1
 mov ah,48h
 mov bx,400h
 int 21h
 JFAIL jc,allocate_failed
 mov high_segment,ax
 ; Never trust a successful allocation unless it is exactly the target.
 cmp ax,9800h
 JFAIL jne,allocate_failed
 dec ax
 mov es,ax
 cmp byte ptr es:[0],'Z'
 JFAIL jne,allocate_failed
 cmp word ptr es:[3],400h
 JFAIL jne,allocate_failed
 mov ax,cs
 cmp word ptr es:[1],ax
 JFAIL jne,allocate_failed
 call restore_strategy
 JFAIL jc,allocate_failed

 push cs
 pop es
 mov si,offset expected_id
 mov di,offset resident_id
 mov cx,8
 rep movsb
 mov resident_video,9800h
 mov resident_count,400h
 mov dx,offset msg_success
 mov ah,9
 int 21h
 ; AH=31h does not close inherited handles. Release them after printing,
 ; including redirected stdout, so a hookless TSR leaks no open files.
 xor bx,bx
 mov cx,20
close_handles:
 mov ah,3eh
 int 21h
 inc bx
 loop close_handles
 mov dx,KEEP_PARAS
 mov ax,3100h
 int 21h

occupied:
 mov dx,offset msg_occupied
 jmp short failed
allocate_failed:
 mov dx,offset msg_allocate
failed:
 push dx
 call restore_strategy
 mov ax,high_segment
 or ax,ax
 jz nothing_to_free
 mov es,ax
 mov ah,49h
 int 21h
nothing_to_free:
 pop dx
 mov ah,9
 int 21h
 mov ax,4c01h
 int 21h

; CF=0 only if the complete conventional chain is well-formed and ends
; EXACTLY at 9C00h. Read-only, monotonic, bounded walk. ES=last MCB.
validate_arena proc near
 cmp ax,50h
 jb arena_bad
arena_next:
 cmp ax,9c00h
 jae arena_bad
 mov es,ax
 mov bl,es:[0]
 cmp bl,'M'
 je arena_type_ok
 cmp bl,'Z'
 jne arena_bad
arena_type_ok:
 ; Free/resize need not immediately coalesce adjacent free MCBs. Track
 ; the start of the final free run; DOS allocation performs the merge.
 cmp word ptr es:[1],0
 jne arena_not_free
 cmp free_run_start,0
 jne arena_free_done
 mov free_run_start,ax
 jmp short arena_free_done
arena_not_free:
 mov free_run_start,0
arena_free_done:
 mov cx,es:[3]
 add cx,ax
 jc arena_bad
 inc cx
 jz arena_bad
 cmp cx,9c00h
 ja arena_bad
 cmp bl,'Z'
 je arena_end
 cmp cx,9c00h
 jae arena_bad
 mov ax,cx
 jmp short arena_next
arena_end:
 cmp cx,9c00h
 jne arena_bad
 mov last_mcb,ax
 clc
 ret
arena_bad:
 stc
 ret
validate_arena endp

; ES=97FFh top MCB. Validate owner against the already-checked chain,
; the resident owner's MCB, and a versioned low-memory signature.
is_our_reservation proc near
 mov bp,es:[1]
 mov ax,first_mcb
owner_next:
 mov es,ax
 mov bx,ax
 inc bx
 cmp bx,bp
 je owner_found
 mov bx,es:[3]
 add ax,bx
 inc ax
 cmp ax,97ffh
 jb owner_next
 jmp short owner_bad
owner_found:
 cmp es:[1],bp
 jne owner_bad
 cmp word ptr es:[3],KEEP_PARAS
 jne owner_bad
 mov es,bp
 mov si,offset expected_id
 mov di,offset resident_id
 mov cx,8
 repe cmpsb
 jne owner_bad
 cmp word ptr es:[resident_video],9800h
 jne owner_bad
 cmp word ptr es:[resident_count],400h
 jne owner_bad
 clc
 ret
owner_bad:
 stc
 ret
is_our_reservation endp

restore_strategy proc near
 cmp strategy_changed,0
 je strategy_ok
 mov bx,old_strategy
 mov ax,5801h
 int 21h
 jc strategy_return
 mov strategy_changed,0
strategy_ok:
 clc
strategy_return:
 ret
restore_strategy endp

expected_id db 'TNDYRS01'
first_mcb dw 0
free_run_start dw 0
last_mcb dw 0
old_strategy dw 0
high_segment dw 0
strategy_changed db 0
msg_success db 'RESERVE: 16K video overlap reserved. Start Windows with WIN /R.',13,10,'$'
msg_already db 'RESERVE: video reservation already installed.',13,10,'$'
msg_platform db 'RESERVE: need Tandy, text mode 3, BIOS 624K and DOS 3+. No reservation.',13,10,'$'
msg_location db 'RESERVE: program loaded too high. No reservation.',13,10,'$'
msg_dos db 'RESERVE: DOS resize failed. No reservation.',13,10,'$'
msg_arena db 'RESERVE: unsupported or corrupt DOS arena. No reservation.',13,10,'$'
msg_strategy db 'RESERVE: DOS last-fit allocation unavailable. No reservation.',13,10,'$'
msg_occupied db 'RESERVE: top video overlap already occupied. No reservation.',13,10,'$'
msg_allocate db 'RESERVE: allocation or verification failed. No reservation.',13,10,'$'
; Keep the explicit transient stack as well: AH=31h uses it during exit.
even
 db 256 dup(0)
stack_end label byte
KEEP_PARAS equ (stack_end-entry+100h+15)/16
code ends
end entry
