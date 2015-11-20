%include "asm/stack.mac"

EXTERN sched_switch_to_kernel_stack
EXTERN sched_switch_from_kernel_stack
EXTERN sched_pick_process

EXTERN syscall_halt


GLOBAL _int_sys_handler
GLOBAL _int_mem_handler

EXTERN mem_handler
EXTERN sys_handler

SECTION .text

; Int 80h
; Return Value:             rax
; Syscall Index:            rax
; Parameter Registers:           rdi, rsi, rdx, rcx, r8, r9
; Scratch Registers:        rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11
; Preserved Registers:      rbx, rsp, rbp, r12, r13, r14, r15

_int_sys_handler:
	cli
    PUSHA

    mov     r15,     rsp                    ; Store proc stack
    mov     r12,     rax                    ; Store syscall idx

    PUSH_PARAMS
    mov     rdi,    rsp                     ; Save process stack
;    call    sched_switch_to_kernel_stack    ; Get kernel stack
;    mov     rsp,    rax                     ; Switch stacks
    POP_PARAMS

    mov     rax,    r12                     ; Restore syscall idx

    call    sys_handler                     ; Do the syscall thing
;    SET_SYSCALL_RET r15, rax                ; Step rax at stack

;    call    sched_pick_process              ; Get new process stack
;    mov     rsp,    rax                     ; Switch stacks

    POPA
    sti
	iretq

_int_mem_handler:
    cli
    PUSHA
    mov     rdi,    [rsp - 8]
    call    mem_handler
    POPA
    sti
    iretq

