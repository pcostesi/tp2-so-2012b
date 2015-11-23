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
    ENTER
    mov rax, cr3
    LEAVE


_write_cr3:
    ENTER
    mov rax, rdi
    mov cr3, rax
    LEAVE