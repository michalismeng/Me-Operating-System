#include "screen_gfx.h"
#include "file.h"
#include "error.h"
#include "memory.h"
#include "vfs.h"
#include "print_utility.h"

const int FONT_CHAR_WIDTH = 8, FONT_CHAR_PAD = 1, FONT_CHAR_HEIGHT = 16, FONT_ENTRIES = 256;
const int FONT_SIZE = (FONT_CHAR_WIDTH + FONT_CHAR_PAD) * FONT_CHAR_HEIGHT * FONT_ENTRIES;

char font[FONT_SIZE];
uint32 font_fd;

vbe_mode_info_block* vbe;

point cursor;
uint16 charsHorizontal, charsVertical;
uint32 foreground = 0, background = 0;

bool screen_initialized = false;

error_t screen_gfx_open(vfs_node* node);
size_t screen_gfx_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
size_t screen_gfx_write(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
error_t screen_gfx_ioctl(vfs_node* node, uint32 command, ...);


// file operations
static fs_operations screen_gfx_operations =
{
	screen_gfx_read,		// read
	screen_gfx_write,		// write
	screen_gfx_open,		// open
	NULL,					// close
	NULL,					// sync
	NULL,					// lookup
	screen_gfx_ioctl		// ioctl?
};

size_t screen_gfx_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	serial_printf("WARNING: tried to read from screen\n");
	return ERROR_OK;
}

error_t screen_gfx_open(vfs_node* node)
{
	return ERROR_OK;
}

size_t screen_gfx_write(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	uint8* ptr = ((uint8*)address) + start;
	size_t res;

	if (count != 0)
	{
		for (size_t i = 0; i < count; i++)
			draw_char(ptr[i]);

		res = count;
	}
	else
	{
		while (*ptr != 0)
			draw_char(*ptr++);

		res = (size_t)ptr - address;
	}

	return res;
}

error_t screen_gfx_ioctl(vfs_node* node, uint32 command, ...)
{
	return ERROR_OK;
}

point make_point(uint16 x, uint16 y)
{
	point p;
	p.x = x;
	p.y = y;

	return p;
}

void init_screen_gfx(vbe_mode_info_block* _vbe)
{
	vbe = _vbe;

	uint32 frame_length = vbe->pitch * vbe->height;

	if (!vfs_mmap(vbe->framebuffer & (~0xFFF), INVALID_FD, 0, frame_length + PAGE_SIZE - (frame_length % PAGE_SIZE), 
		PROT_READ | PROT_WRITE, MMAP_PRIVATE | MMAP_ANONYMOUS | MMAP_IDENTITY_MAP | MMAP_ALLOC_IMMEDIATE) == MAP_FAILED)
		PANIC("Could not map screen region");

	cursor = make_point(0, 0);

	charsHorizontal = vbe->width / 8;
	charsVertical = vbe->height / 16;

	//clear_screen();
	load_default_font();

	// print diagnostics
	screen_gfx_print();

	vfs_create_device("screen", 0, 0, &screen_gfx_operations);

	screen_initialized = true;
}

void set_foreground_color(uint32 color)
{
	foreground = color & 0x00FFFFFF;
}

void set_background_color(uint32 color)
{
	background = color & 0x00FFFFFF;
}

point get_cursor()
{
	return cursor;
}

void set_cursor(uint16 x, uint16 y)
{
	cursor.x = x;
	cursor.y = y;
}

uint16 get_chars_vertical()
{
	return charsVertical;
}

void clear_screen()
{
	for (uint16 i = 0; i < charsHorizontal; i++)
		for (uint16 j = 0; j < charsVertical; j++)
			draw_char(0);

	cursor = make_point(0, 0);
}

void load_default_font()
{
	if (open_file("sdc_mount/FONT.RAW", &font_fd, O_NOCACHE) != ERROR_OK)
	{
		serial_printf("**************Font could not be loaded. File not found.*******************\n");
		//PANIC("");
		return;
	}

	if (read_file(font_fd, 0, FONT_SIZE, (virtual_addr)font) != FONT_SIZE)
	{
		serial_printf("Font could not be read %u.\n", get_last_error());
		PANIC("");
	}
}

void put_pixel(point p, uint32 color)
{
	if (p.x >= vbe->width || p.y >= vbe->height)
		return;

	uint32* pixel_offset = (uint32*)(p.y * vbe->pitch + p.x * vbe->bpp / 8 + vbe->framebuffer);
	*pixel_offset = color;
}

void put_pixel_raw(uint32 index, uint32 color)
{
	*((uint32*)index) = color;
}

uint32 get_LFA_index(point p)
{
	return p.y * vbe->pitch + p.x * vbe->bpp / 8 + vbe->framebuffer;
}

uint32 increment_LFA_index(uint32 cur_index)
{
	return (cur_index + vbe->bpp / 8);
}

uint32 skip_LFA_line(uint32 cur_index)
{
	return cur_index + vbe->pitch;
}

void _draw_char(char c, point p)
{
	if(!screen_initialized)
		serial_printf("Screen is not initialized but drawing");

	uint32 start_vidmem = get_LFA_index(p);

	for (int y = 0; y < 16; y++)
	{
		uint32 vidmem = start_vidmem;

		for (int x = 0; x < 8; x++)
		{
			if (font[c * (FONT_CHAR_WIDTH + FONT_CHAR_PAD) + y * FONT_ENTRIES * (FONT_CHAR_WIDTH + FONT_CHAR_PAD) + x])
				put_pixel_raw(vidmem, foreground);
			else
				put_pixel_raw(vidmem, background);

			vidmem = increment_LFA_index(vidmem);
		}

		start_vidmem = skip_LFA_line(start_vidmem);
	}
}

void draw_string(char* str)
{
	if (screen_initialized == false)
		return;

	while (*str != 0)
	{
		draw_char(*str);
		str++;

	}
}

void draw_char(char c)
{
	if (screen_initialized == false)
		return;

	switch (c)
	{
	case 0x08:		// backspace
		if (cursor.x > 0)
		{
			cursor.x--;
			_draw_char(0, make_point(cursor.x * 8, cursor.y * 16));
		}
		else if(cursor.y > 0)
		{
			cursor.x = charsHorizontal - 1;
			cursor.y--;
			_draw_char(0, make_point(cursor.x * 8, cursor.y * 16));
		}
		break;
	/*case 0x09:
		cursorX += TAB_SIZE - cursorX % TAB_SIZE;
		break;*/
	case '\r':
	case '\n':
		cursor.x = 0;
		cursor.y++;
		break;	
	default:
		if (c == 0x09)			// TAB CHARACTER
			c = 0x20;			// MAKE IT A SPACE
		_draw_char(c, make_point(cursor.x * 8, cursor.y * 16));
		cursor.x++;
		break;
	}

	if (cursor.x >= charsHorizontal)
	{
		cursor.x = 0;
		cursor.y++;
	}

	if (cursor.y >= charsVertical)
		cursor.y = 0;
}

void draw_rectangle(point p1, point p2, uint32 color)
{
	draw_line(p1, make_point(p1.x + p2.x, p1.y), color);
	draw_line(p1, make_point(p1.x, p1.y + p2.y), color);

	draw_line(make_point(p1.x + p2.x, p1.y), make_point(p1.x + p2.x, p1.y + p2.y), color);
	draw_line(make_point(p1.x, p1.y + p2.y), make_point(p1.x + p2.x, p1.y + p2.y), color);
}

void draw_line(point p1, point p2, uint32 color)
{
	if (p1.x == p2.x)
	{
		for (uint16 y = min(p1.y, p2.y); y <= max(p1.y, p2.y); y++)
			put_pixel(make_point(p1.x, y), color);
	}
	else
	{
		float32 slope = (float32)(p2.y - p1.y) / (p2.x - p1.x);

		for (uint16 x = min(p1.x, p2.x); x <= max(p1.x, p2.x); x++)
			put_pixel(make_point(x, slope * (x - p1.x) + p1.y), color);
	}	
}

// prints memory diagnostics
void screen_gfx_print()
{
	serial_printf("width: %u\nheight: %u\nbpp: %u\nvidmem ptr: %h\n", vbe->width, vbe->height, vbe->bpp, vbe->framebuffer);

	serial_printf("positions: %u:%u:%u:%u\n", vbe->reserved_position, vbe->red_position, vbe->green_position, vbe->blue_position);
	serial_printf("RGB mode: %u:%u:%u\n", vbe->red_mask, vbe->green_mask, vbe->blue_mask);

	serial_printf("video memory = %u (bytes)\n", vbe->pitch * vbe->height);
	serial_printf("-----text mode in use.-----\n");
	serial_printf("characters per line: %u\nlines per screen: %u\ntotal characters: %u\n", charsHorizontal, charsVertical, charsHorizontal * charsVertical);
}

