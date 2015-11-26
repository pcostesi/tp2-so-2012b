#include <syscalls.h>
#include <video.h>
#include <asm.h>

void syscall_halt(void)
{
	static const char msg[] = "System offline.";
	int msg_size = sizeof(msg) / sizeof(msg[0]);
	vid_clr(VID_PROC);
	vid_clr(VID_SYSLOG);
	vid_cursor(VID_PROC, VID_ROWS / 2, (VID_COLS - msg_size) / 2);
	vid_cursor(VID_SYSLOG, VID_ROWS / 2, (VID_COLS - msg_size) / 2);
	vid_print(VID_PROC, msg, msg_size);
	vid_print(VID_SYSLOG, msg, msg_size);
	vid_update();
	_halt();
}
