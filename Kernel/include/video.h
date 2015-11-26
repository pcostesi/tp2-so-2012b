#ifndef __vid_drv
#define __vid_drv 1

#define VID_COLS (80)
#define VID_ROWS (25)
#define VID_COLOR(foreground, background) (((foreground) & 0x0F) | ((background) << 4))

enum VID_COLOR
{
	BLACK = 0,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	LIGHT_GRAY,

	GRAY = 8,
	LIGHT_BLUE,
	LIGHT_GREEN,
	LIGHT_CYAN,
	LIGHT_RED,
	LIGHT_MAGENTA,
	YELLOW,
	WHITE,
};

enum vid_term {
	VID_PROC,
	VID_SYSLOG,
};

void vid_show(enum vid_term term);
void vid_update(void);
void vid_putc(enum vid_term term, const char c);
int  vid_cursor(enum vid_term term, const unsigned int row, const unsigned int col);
void vid_raw_putc(enum vid_term term, const char c, const enum VID_COLOR);
char vid_color(enum vid_term term, const enum VID_COLOR, const enum VID_COLOR);
void vid_print(enum vid_term term, const char * str, unsigned int n);
void vid_raw_print(enum vid_term term, const char * str, unsigned int n);
void vid_clr(enum vid_term term);

#endif