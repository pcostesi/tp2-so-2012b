; Platform:                 System V x86_64
; Return Value:             rax, rdx
; Parameter Registers:      rdi, rsi, rdx, rcx, r8, r9
; Additional Parameters:    stack (right to left)
; Stack Alignment:          16-byte at call
; Scratch Registers:        rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11
; Preserved Registers:      rbx, rsp, rbp, r12, r13, r14, r15
; Call List:                rbp

%include "asm/stack.mac"

GLOBAL cpu_vendor
GLOBAL _halt
GLOBAL _drool
GLOBAL get_memory_size

section .text
    
cpu_vendor:
    ENTER

    push rbx

    mov rax, 0
    cpuid

    mov [rdi], ebx
    mov [rdi + 4], edx
    mov [rdi + 8], ecx

    mov byte [rdi+13], 0

    mov rax, rdi

    pop rbx

    LEAVE


get_memory_size:
    ENTER
    xor     rax,    rax
    mov     eax,    [0x5020]
    LEAVE


_halt:
    cli
    hlt
    ret


_drool:
    hlt
    ret