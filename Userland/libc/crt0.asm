SECTION .text

EXTERN main
EXTERN exit

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


%macro ENTER 0
    push    rbp
    mov     rbp, rsp
    PUSHCALL
%endmacro

%macro LEAVE 0
    POPCALL
    mov     rsp, rbp
    pop     rbp
    ret
%endmacro


;kinda like crt0 but hacky
GLOBAL _start
_start:
    ENTER
    call 	main
    mov     rdi,    rax
    call    exit
    LEAVE
