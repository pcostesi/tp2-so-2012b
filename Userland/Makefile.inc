include ../../Makefile.inc

INCLUDES=-I../libc/include -I./include
ARFLAGS=rvs
STDFLAGS+= -Werror
ASMFLAGS=-felf64
LIBC=-L../ -L./ -lc
CRT0=../crt0.o

GCCFLAGS+=${INCLUDES} ${LIBC}