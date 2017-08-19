#ifndef VBE_DEFINITIONS_H_19082017
#define VBE_DEFINITIONS_H_19082017

#include "types.h"

#pragma pack(push, 1)

struct vbe_mode_info_block
{
	uint16	attributes;				// deprecated mode attributes, bit 7 is of interest as it indicates support for LFA
	uint8	useless1[14];			// useless info, mostly deprecated.Check specification
	uint16	pitch;					// number of bytes per horizontal line
	uint16	width;					// width in pixels
	uint16	height;					// height in pixels
	uint8	useless2[3];			// more useless stuff
	uint8	bpp;					// bytes per pixel
	uint8	useless3[5];			// more useless stuff
	uint8	red_mask;
	uint8	red_position;
	uint8	green_mask;
	uint8	green_position;
	uint8	blue_mask;
	uint8	blue_position;
	uint8	reserved_mask;
	uint8	reserved_position;
	uint8	direct_color_attr;		// more useless stuff
	uint32	framebuffer;			// the LFA used by this mode(if indicated by attibutes)
	uint8	useless4[212];			// more useless stuff
};

#pragma pack(pop, 1)

#endif
