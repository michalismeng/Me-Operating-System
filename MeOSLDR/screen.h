#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"
#include "system.h"

#define DEFAULT_COLOR 0x3F

#define BLACK 			0
#define	DARK_BLUE 		1
#define	DARK_GREEN 		2
#define	DARK_CYAN 		3
#define	DARK_RED 		4
#define	DARK_MAGENTA 	5
#define	DARK_YELLOW 	6
#define	DARK_WHITE 		7
#define	GRAY 			8
#define	BLUE 			9
#define	GREEN 			10
#define	CYAN 			11
#define	RED 			12
#define	MAGENTA 		13
#define	YELLOW 			14
#define	WHITE 			15

// start address of vidmem is  0xB8000
// final address of vidmem is (0xB8FA0 - 1) if address >= 0xB8FA0 it is not video memory territory
// size of address of vidmem is 4000 decimal or FA0 hex

extern uint16 cursorX, cursorY;
extern uint8 color;

extern const uint8 SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH;		// screen width, height and depth
extern const uint8 TAB_SIZE;

void ClearLine(uint8 from, uint8 to);

void UpdateCursor();

void SetCursor(uint16 x, uint16 y);

void ClearScreen();

void ScrollUp(uint8 lineNumber);

void NewLineCheck();

void Printch(char c);

void Print(char* str);

void SetColor(uint8 color);

uint8 MakeColor(uint8 background, uint8 foreground);

#endif