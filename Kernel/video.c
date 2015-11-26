#include <video.h>
#include <lib.h>

#define COLS (VID_COLS)
#define ROWS (VID_ROWS)
#define VID_BUF_SIZE (COLS * ROWS)

#define VID_POS(row, col) (COLS * (row) + (col))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define TAB_SIZE (8)
#define VIDEO_BASE_ADDR ((vid_cell *) 0xB8000)


typedef struct {
	char c;
	char fmt;
} vid_cell;

struct vid_mem {
	vid_cell video[VID_BUF_SIZE];
	unsigned int cursor;
	char fmt;
};

static struct vid_mem terminals[2];
static enum vid_term active_term = VID_PROC;

static vid_cell * video = VIDEO_BASE_ADDR;

inline static void vid_scroll(enum vid_term term);
inline static void _print(struct vid_mem * term, char c, char fmt);

inline static void _print(struct vid_mem * term, char c, char fmt)
{
	int pos = term->cursor++;
	vid_cell * cell = &term->video[pos];
	cell->c = c;
	cell->fmt = fmt;
}

static inline struct vid_mem * get_active_term(enum vid_term term);

void vid_update(void)
{
	struct vid_mem * active = get_active_term(active_term);
	memcpy(video, active->video, VID_BUF_SIZE * sizeof(active->video[0]));
}

static inline struct vid_mem * get_active_term(enum vid_term term)
{
	return terminals + term;
}

void vid_show(enum vid_term term)
{
	active_term = term;
}

void vid_raw_print(enum vid_term term, const char * str, unsigned int n)
{
	int idx;

	/* if we're asked to print something length 2n + 1, print 'till 2n */
	for (idx = 0; idx < n / 2; idx++) {
		vid_raw_putc(term, str[2 * idx], str[2 * idx + 1]);
	}
}

void vid_print(enum vid_term term, const char * str, unsigned int n)
{
	char * p = (char *) str;
	while (n--) {
		vid_putc(term, *p++);
	}
}

void vid_clr(enum vid_term term) {
	int i;
	struct vid_mem * active = get_active_term(term);
	vid_cursor(term, 0, 0);
	for (i = 0; i <= VID_BUF_SIZE; i++) {
		vid_raw_putc(term, (char) 0, active->fmt);
	}
	vid_cursor(term, 0, 0);
}

char vid_color(enum vid_term term, const enum VID_COLOR foreground, const enum VID_COLOR background)
{
	return get_active_term(term)->fmt = VID_COLOR(foreground, background);
}

void vid_putc(enum vid_term term, const char c)
{
	struct vid_mem * active = get_active_term(term);
	vid_raw_putc(term, c, active->fmt);
}

int vid_cursor(enum vid_term term, const unsigned int row, const unsigned int col)
{
	struct vid_mem * active = get_active_term(term);
	int raw = VID_POS(row, col);
	int _VID_MAX_POS = VID_POS(ROWS - 1, COLS);
	int _VID_MIN_POS = VID_POS(0, 0);

	if (raw > _VID_MAX_POS) {
		 active->cursor = _VID_MAX_POS;
	} else if (raw < _VID_MIN_POS) {
		active->cursor = _VID_MIN_POS;
	} else {
		active->cursor = raw;
	}

	return active->cursor;
}

void vid_raw_putc(enum vid_term term, const char c, const enum VID_COLOR fmt)
{
	int row, col;
	struct vid_mem * active = get_active_term(term);

	if (active->cursor >= VID_POS(ROWS - 1, COLS)) {
		vid_scroll(term);
		vid_cursor(term, ROWS - 1, 0);
	}

	row = active->cursor / COLS;
	col = active->cursor % COLS;

	switch (c) {
		case '\n':
		while (active->cursor < VID_POS(row + 1, 0)) {
			_print(active, (char) 0, fmt);
		}
		vid_cursor(term, row + 1, 0);
		break;
	
		case '\r':
		vid_cursor(term, row, 0);
		break;

		case '\b':
            row = (col - 1 < 0) ? MAX(row - 1, 0) : row;
            col = (col - 1 < 0) ? COLS + col - 1 : col - 1;
		vid_cursor(term, row, col);
		break;

		case '\t':
		do {
			_print(active, (char) 0, fmt);
			col += 1;
		} while (col < COLS && col < TAB_SIZE);
		break;
	
		default:
		_print(active, c, fmt);
	}
}

inline static void _vid_copy_row(struct vid_mem * active, const int row_src, const int row_dst)
{
	int src = VID_POS(row_src, 0);
	int dst = VID_POS(row_dst, 0);
	memcpy(active->video + dst, active->video + src, COLS * sizeof(vid_cell));
}

inline static void vid_scroll(enum vid_term term)
{
	int row, col;
	struct vid_mem * active = get_active_term(term);

	for (row = 0; row < ROWS - 1; row++) {
		_vid_copy_row(active, row + 1, row);
	}

	active->cursor = VID_POS(ROWS - 1, 0);
	for (col = 0; col < COLS; col++) {
		_print(active, (char) 0, active->fmt);
	}
}
