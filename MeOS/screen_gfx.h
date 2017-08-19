#ifndef SCREEN_GFX_H_15082017
#define SCREEN_GFX_H_15082017

#include "types.h"
#include "utility.h"
#include "VBEDefinitions.h"
#include "SerialDebugger.h"

struct point
{
	uint16 x;
	uint16 y;
};

struct point make_point(uint16 x, uint16 y);

void load_default_font();
void init_screen_gfx(struct vbe_mode_info_block* _vbe);

void set_foreground_color(uint32 color);
void set_background_color(uint32 color);

point get_cursor();
void set_cursor(uint16 x, uint16 y);
uint16 get_chars_vertical();

void clear_screen();

// returns the index in the LFS corresponding to the given point
uint32 get_LFA_index(struct point p);

void put_pixel(struct point p, uint32 color);
void draw_line(struct point p1, struct point p2, uint32 color);
void draw_char(char c);
void draw_string(char* str);
void draw_rectangle(struct point p1, struct point p2, uint32 color);

void screen_gfx_print();

#endif