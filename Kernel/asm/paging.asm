%include "asm/stack.mac"

GLOBAL _read_cr0
GLOBAL _write_cr0
GLOBAL _read_cr3
GLOBAL _write_cr3

SECTION .text

_read_cr0:
    push rbp
    mov rbp, rsp
    mov rax, cr0
    mov rsp, rbp
    pop rbp
    ret


_write_cr0:
    push rbp
    mov rbp, rsp
    mov cr0, rdi
    mov rsp, rbp
    pop rbp
    ret


_read_cr3:
    push rbp
    mov rbp, rsp
    mov rax, cr3
    and rax, 0xFFFFFFFFF000
    mov rsp, rbp
    pop rbp
    ret


_write_cr3:
    push rbp
    mov rbp, rsp
    mov cr3, rdi
    mov rsp, rbp
    pop rbp
    ret