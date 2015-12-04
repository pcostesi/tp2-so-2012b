%include "asm/stack.mac"

GLOBAL _cli
GLOBAL _sti
GLOBAL _lidt
GLOBAL _sidt
GLOBAL idt_pic_master_mask
GLOBAL idt_pic_slave_mask
GLOBAL idt_pic_master_set_map
GLOBAL idt_pic_slave_set_map

EXTERN irq_handler

EXTERN sched_switch_to_kernel_stack
EXTERN sched_switch_from_kernel_stack
EXTERN sched_switch_to_user_stack

SECTION .text

%macro EOIM 0
    mov     al,     20h
    out     20h,    al
%endmacro

%macro EOIS 0
    ;signal master pic
    EOIM

    ;signal slave pic
    mov     al,     0xA0
    out     0xA0,   al
%endmacro

%macro HANDLE_IRQ 1
    ;switch to kernelspace
    mov     rdi,    rsp
    call    sched_switch_to_kernel_stack
    mov     rsp,    rax

    ;call the irq handler
    mov     rdi,    %1
    call    irq_handler

    mov     rdi,    rsp
    call    sched_switch_to_user_stack
    mov     rsp,    rax
%endmacro

%macro _idt_irq_master_handler 1
GLOBAL _irq_%1_handler

_irq_%1_handler:
    cli
    PUSHA

    HANDLE_IRQ %1
    EOIM
    
    POPA
    sti
    iretq
%endmacro


%macro _idt_irq_slave_handler 1
GLOBAL _irq_%1_handler

_irq_%1_handler:
    cli
    PUSHA

    HANDLE_IRQ %1
    EOIS
    
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

