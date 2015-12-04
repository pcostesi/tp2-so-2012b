SECTION .text

; see https://msdn.microsoft.com/en-us/library/6t169e9c.aspx
%macro PUSHALL 0
    push    RBX
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
    pop     RSP
    pop     RBX
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

%macro _int80h 2
GLOBAL %1
%1:
    ENTER
	mov rax, %2
	int 80h
    LEAVE
%endmacro


_int80h	write, 0
_int80h read, 1
_int80h open, 2
_int80h close, 3

_int80h mmap, 9
_int80h munmap, 11

_int80h ioctl, 16

_int80h opipe, 22
_int80h cpipe, 23
_int80h wpipe, 24
_int80h rpipe, 25

_int80h pause, 34
_int80h getpid, 39
_int80h beep, 42

_int80h halt, 48
_int80h shutdown, 48

_int80h fork, 57
_int80h execv, 59
_int80h exit, 60
_int80h kill, 62

_int80h gettime, 228
_int80h settime, 227

_int80h gpipes, 26
