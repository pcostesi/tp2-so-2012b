%include "asm/stack.mac"

GLOBAL _read_cr0
GLOBAL _write_cr0
GLOBAL _read_cr3
GLOBAL _write_cr3

SECTION .text

_read_cr0:
    ENTER
    mov rax, cr0
    LEAVE


_write_cr0:
    ENTER
    mov rax, rdi
    mov cr0,  rax
    LEAVE


_read_cr3:
    push rbp
    mov rbp, rsp
    mov rax, cr3
    mov rsp, rbp
    pop rbp
    ret


_write_cr3:
    push rbp
    mov rbp, rsp
    mov rax, rdi
    mov cr3, rax
    mov rsp, rbp
    pop rbp
    ret