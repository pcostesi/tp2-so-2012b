#include <stdio.h>
#include <libc.h>
#include <string.h>

int fputc(int fd, char c)
{
	return write(fd, &c, 1);
}

int putc(char c)
{
	return fputc(STDOUT, c);
}

int fputsn(int fd, char * c, int n)
{
	return write(fd, c, n);
}

int fputs(int fd, char * c)
{
	return write(fd, c, strlen(c));
}

int puts(char * c)
{
	return fputs(STDOUT, c);
}