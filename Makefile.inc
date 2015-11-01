GCC=gcc
GPP=g++
LD=ld
AR=ar
ASM=nasm

CFLAGS=-ffreestanding -fno-asynchronous-unwind-tables -fno-builtin-free -fno-builtin-malloc -fno-builtin-realloc -fno-common -fno-exceptions -m64 -mno-mmx -mno-red-zone -mno-sse -mno-sse2 -nostdlib
STDFLAGS=-std=c99 -Wall
GCCFLAGS=${CFLAGS} ${STDFLAGS}
ARFLAGS=rvs
ASMFLAGS=-felf64
