GLOBAL _cli
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
%macro PUSHALL 0
    push    RBX
    push    RDI
    push    RSI
    push    RSP
    push    R12
    push    R13
    push    R14
    push    R15
%endmacro

%macro POPALL 0
    pop     R15
    pop     R14
    pop     R13
    pop     R12
    pop     RSP
    pop     RSI
    pop     RDI
    pop     RBX
%endmacro

%macro PUSHALLSCHED 0
    pushf
    push    RBP
    push    RSP
    push    RSI
    push    RDX
    push    RDI
    push    RDI
    push    RCX
    push    RBX
    push    R9
    push    R8
    push    R15
    push    R14
    push    R13
    push    R12
%endmacro

%macro POPALLSCHED 0
    pop     R12
    pop     R13
    pop     R14
    pop     R15
    pop     R8
    pop     R9
    pop     RBX
    pop     RCX
    pop     RDI
    pop     RDI
    pop     RDX
    pop     RSI
    pop     RSP
    pop     RBP
    popf
%endmacro

%macro ENTER 0
    push    rbp
    mov     rbp, rsp
    PUSHALL
%endmacro

%macro LEAVE 0
    POPALL
    mov     rsp, rbp
    pop     rbp
    ret
%endmacro

%macro _idt_irq_master_handler 1
GLOBAL _irq_%1_handler

_irq_%1_handler:
    cli
    push rax
    push rdi
    mov rdi, %1
    call irq_handler
    
    ;signal master pic
    mov al, 20h
    out 20h, al
    
    pop rdi
    pop rax
    sti
    iretq
%endmacro

%macro _idt_irq_slave_handler 1
GLOBAL _irq_%1_handler

_irq_%1_handler:
    cli
    push rax
    push rdi
    mov rdi, %1
    call irq_handler
    
    ;signal master pic
    mov al, 20h
    out 20h, al

    ;signal slave pic
    mov al, 0xA0
    out 0xA0, al
    
    pop rdi
    pop rax
    sti
    iretq
%endmacro


idt_pic_master_mask:
    ENTER
    mov 	al, dil
    out		21h, al
    LEAVE


idt_pic_slave_mask:
    ENTER
    mov 	al, dil
    out		0A1h, al
    LEAVE


idt_pic_master_set_map:
    ENTER
    mov 	al, dil
    out		20h, al
    LEAVE


idt_pic_slave_set_map:
    ENTER
    mov 	al, dil
    out		0A0h, al
    LEAVE


;Int 80h
_int_sys_handler:
	cli
    push    RBP
    PUSHALL
    call sys_handler
    POPALL
    pop     RBP
	sti
	iretq

_int_mem_handler:
    cli
    push rax
    push rdi
    mov rdi, [rsp - 3]
    call mem_handler
    pop rdi
    pop rax
    sti
    iretq


GLOBAL _sched_init_stack

_sched_init_stack:
    ENTER
    mov     rbx, rsp
    mov     rsp, rdi
    push    RSI; the RIP
    pushf
    push    RDI; the RBP
    push    RDI; the RSP
    push    0;   RSI
    push    0;   RDX
    push    0;   RDI
    push    0;   RDI
    push    0;   RCX
    push    0;   RBX
    push    0;   R9
    push    0;   R8
    push    0;   R15
    push    0;   R14
    push    0;   R13
    push    0;   R12
    mov     rax, rsp
    mov     rsp, rbx
    LEAVE

GLOBAL _int_pit_handler

_int_pit_handler:
    cli

    ;save instruction pointer and everything else
    PUSHALLSCHED ;missing registers

    ;switch to kernelspace
    ;save the stack
    mov rdi, rsp
    call sched_switch_to_kernel_stack
    mov rsp, rax
    ;signal master pic
    mov al, 20h
    out 20h, al

    call sched_pick_process
    mov rsp, rax

    POPALLSCHED
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