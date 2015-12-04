%include "asm/stack.mac"

GLOBAL _sched_init_stack
GLOBAL sched_drop_to_user
GLOBAL sched_step_syscall_rax
GLOBAL sched_call_zygote

EXTERN panic
EXTERN show_stack

EXTERN _sched_get_current_process_entry
EXTERN sched_switch_to_user_stack
EXTERN sched_get_process
EXTERN syscall_exit

; Read this before making any changes (or reviewing the code):
; http://stackoverflow.com/questions/9383544

SECTION .data
warn: dw 'panic'

SECTION .text
_sched_init_stack:
    ENTER
    mov     r10,    rsp
    mov     rsp,    rdi

    ; If the interrupt is in a higher privelege level, SS and RSP
    ; are pushed to the stack, each one consuming 8 bytes.
    ; For the same level this step is skipped.

    ; SS / RSP get saved to the stack if/when a stack switch occurs.
    ; This is default for inter-privilege events (interrupt while
    ; in usermode / ring 3) and non-default for intra-privilege
    ; events (interrupt while already in kernel / ring 0)

    push    syscall_exit
    push    0       ;save stack segment
    push    rsp     ;save frame rsp

    ; Then RFLAGS, CS and RIP are pushed to the stack, each one
    ; using 8 bytes too.

    push    QWORD 0x202     ;save rflags
    push    QWORD 0x08      ;save code segment
    push    rsi             ;save RIP

    ; Create a faux trap frame. If / when we implement params,
    ; then this function should store argc and argv in the rdi
    ; and rsi stack positions.

    ; Initialize the process stack with the stack base rbp,
    ; argc 0, argv 0.
    INITPROC rsp, 0, 0, rdx

    ; Return the stack pointer for the process and jump back to caller
    mov     rax,    rsp
    mov     rsp,    r10
    LEAVE

sched_step_syscall_rax:
    ENTER
    SET_SYSCALL_RET rdi, rsi                ; Step rax at stack
    LEAVE

; Force a scheduler step
; then jump to userspace
sched_drop_to_user:
	call sched_get_process
    mov     rsp,    rax
    POPA
    pop     rax
    sti
    jmp     rax
    hlt

sched_call_zygote:
    ENTER
    call    rdx
    mov     rdi,    rax
    call    syscall_exit
    hlt
    LEAVE