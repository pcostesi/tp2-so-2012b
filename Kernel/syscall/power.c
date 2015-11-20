#include <syscalls.h>
#include <video.h>
#include <asm.h>

void syscall_halt(void)
{
	static const char msg[] = "System offline.";
	int msg_size = sizeof(msg) / sizeof(msg[0]);
	vid_clr();
	vid_cursor(VID_ROWS / 2, (VID_COLS - msg_size) / 2);
	vid_print(msg, msg_size);
	_halt();
}
