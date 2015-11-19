GLOBAL _cli
GLOBAL _drool
GLOBAL _sti
GLOBAL _halt
GLOBAL _lidt
GLOBAL _sidt
GLOBAL idt_pic_master_mask
GLOBAL idt_pic_slave_mask
GLOBAL idt_pic_master_set_map
GLOBAL idt_pic_slave_set_map
GLOBAL _int_sys_handler
GLOBAL _int_mem_handler

EXTERN irq_handler
EXTERN mem_handler
EXTERN sys_handler

EXTERN sched_switch_to_kernel_stack
EXTERN sched_switch_from_kernel_stack
EXTERN sched_pick_process


SECTION .text


; see https://msdn.microsoft.com/en-us/library/6t169e9c.aspx
%macro PUSHCALL 0
    push    RBX
    push    RDI
    push    RSI
    push    RSP
    push    R12
    push    R13
    push    R14
    push    R15
%endmacro

%macro POPCALL 0
    pop     R15
    pop     R14
    pop     R13
    pop     R12
    pop     RSP
    pop     RSI
    pop     RDI
    pop     RBX
%endmacro


; This macro will save the current process state into
; the stack. It does not, however, set the trap frame.
; See _sched_init_stack for that
%macro PUSHA 0
    push    rax      ;save current rax
    push    rbx      ;save current rbx
    push    rcx      ;save current rcx
    push    rdx      ;save current rdx
    push    rbp      ;save current rbp
    push    rdi      ;save current rdi
    push    rsi      ;save current rsi
    push    r8       ;save current r8
    push    r9       ;save current r9
    push    r10      ;save current r10
    push    r11      ;save current r11
    push    r12      ;save current r12
    push    r13      ;save current r13
    push    r14      ;save current r14
    push    r15      ;save current r15
    push    fs       ;save fs
    push    gs       ;save gs
%endmacro

%macro POPA 0
    pop     gs
    pop     fs
    pop     r15      ;set r15
    pop     r14      ;set r14
    pop     r13      ;set r13
    pop     r12      ;set r12
    pop     r11      ;set r11
    pop     r10      ;set r10
    pop     r9       ;set r9
    pop     r8       ;set r8
    pop     rsi      ;set rsi
    pop     rdi      ;set rdi
    pop     rbp      ;set rbp
    pop     rdx      ;set rdx
    pop     rcx      ;set rcx
    pop     rbx      ;set rbx
    pop     rax      ;set rax
%endmacro

%macro ENTER 0
    push    rbp
    mov     rbp,    rsp
    PUSHCALL
%endmacro

%macro LEAVE 0
    POPCALL
    mov     rsp,    rbp
    pop     rbp
    ret
%endmacro

%macro _idt_irq_master_handler 1
GLOBAL _irq_%1_handler

_irq_%1_handler:
    cli
    PUSHA

    mov     rdi,    %1
    call    irq_handler
    
    ;signal master pic
    mov     al,     20h
    out     20h,    al
    
    POPA
    sti
    iretq
%endmacro

%macro _idt_irq_slave_handler 1
GLOBAL _irq_%1_handler

_irq_%1_handler:
    cli
    PUSHA
    mov     rdi,    %1
    call    irq_handler
    
    ;signal master pic
    mov     al,     20h
    out     20h,    al

    ;signal slave pic
    mov     al,     0xA0
    out     0xA0,   al
    
    POPA
    sti
    iretq
%endmacro


idt_pic_master_mask:
    ENTER
    mov 	al,    dil
    out		21h,   al
    LEAVE


idt_pic_slave_mask:
    ENTER
    mov 	al,    dil
    out		0A1h,  al
    LEAVE


idt_pic_master_set_map:
    ENTER
    mov 	al,    dil
    out		20h,   al
    LEAVE


idt_pic_slave_set_map:
    ENTER
    mov 	al,    dil
    out		0A0h,  al
    LEAVE


;Int 80h
_int_sys_handler:
	cli
    PUSHA
    call    sys_handler
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


GLOBAL _sched_init_stack

; Read this before making any changes:
; http://stackoverflow.com/questions/9383544

_sched_init_stack:
    ENTER
    mov     rbx,    rsp
    mov     rsp,    rdi

    ; If the interrupt is in a higher privelege level, SS and RSP
    ; are pushed to the stack, each one consuming 8 bytes.
    ; For the same level this step is skipped.

    ; SS / RSP get saved to the stack if/when a stack switch occurs.
    ; This is default for inter-privilege events (interrupt while
    ; in usermode / ring 3) and non-default for intra-privilege
    ; events (interrupt while already in kernel / ring 0)

    
    push    0       ;save stack segment
    push    rsp     ;save frame rsp

    ; Then RFLAGS, CS and RIP are pushed to the stack, each one
    ; using 8 bytes too.

    push    0x202   ;save rflags
    push    0x08    ;save code segment
    push    rsi     ;save RIP

    mov     rax,    rsp

    ; Create a faux trap frame. If / when we implement params,
    ; then this function should store argc and argv in the rdi
    ; and rsi stack positions.

    push    0       ;save current rax
    push    0       ;save current rbx
    push    0       ;save current rcx
    push    0       ;save current rdx
    push    rbp     ;save current rbp
    push    0       ;save current rdi
    push    0       ;save current rsi
    push    0       ;save current r8
    push    0       ;save current r9
    push    0       ;save current r10
    push    0       ;save current r11
    push    0       ;save current r12
    push    0       ;save current r13
    push    0       ;save current r14
    push    0       ;save current r15
    push    0       ;save fs
    push    0       ;save gs

    mov     rax,    rsp
    mov     rsp,    rbx
    LEAVE

GLOBAL _int_pit_handler


_int_pit_handler:
    cli
    ;save instruction pointer and everything else
    PUSHA

    ;switch to kernelspace
    mov     rdi,    rsp
    call    sched_switch_to_kernel_stack
    mov     rsp,    rax

    ;call the irq handler
    mov     rdi,    20h
    call    irq_handler

    call    sched_pick_process
    mov     rsp,    rax

    ;signal master pic
    mov     al,     20h
    out     20h,    al

    POPA
    sti
    iretq


; PIC Master ints
_idt_irq_master_handler 20h
_idt_irq_master_handler 21h
_idt_irq_master_handler 22h
_idt_irq_master_handler 23h
_idt_irq_master_handler 24h
_idt_irq_master_handler 25h
_idt_irq_master_handler 26h
_idt_irq_master_handler 27h

; PIC Slave ints
_idt_irq_slave_handler 70h
_idt_irq_slave_handler 71h
_idt_irq_slave_handler 72h
_idt_irq_slave_handler 73h
_idt_irq_slave_handler 74h
_idt_irq_slave_handler 75h
_idt_irq_slave_handler 76h
_idt_irq_slave_handler 77h


_lidt:
    ENTER
    lidt 	[rdi]
    LEAVE


_sidt:
    ENTER
    sidt 	[rdi]
    LEAVE


_cli:
	cli
	ret


_sti:
	sti
	ret


_halt:
	cli
	hlt
	ret


_drool:
    hlt
    ret