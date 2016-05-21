#ifndef KEYBOARD_H_180516
#define KEYBOARD_H_180516

#include "types.h"
#include "system.h"

// inside keyboard encoder io ports
enum KYBRD_ENCODER_IO
{
	KYBRD_ENC_INPUT_BUF		= 0x60,
	KYBRD_ENC_CMD_REG		= 0x60
};

// on-motherboard keyboard controller io ports
enum KYBRD_CTRL_IO
{
	KYBRD_CTRL_STATS_REG	= 0x64,
	KYBRD_CTRL_CMD_REG		= 0x60
};

// keyboard controller status register masks
enum KYBRD_CTRL_STATS_MASK
{
	KYBRD_CTRL_STATS_MASK_OUT_BUF	= 0x1,		// keyboard output buffer empty - full
	KYBRD_CTRL_STATS_MASK_IN_BUF	= 0x2,		// keyboard input  buffer empty - full
	KYBRD_CTRL_STATS_MASK_SYSTEM	= 0x4,		// keyboard system flag (after reset - BAT)
	KYBRD_CTRL_STATS_MASK_CMD_DATA	= 0x8,		// keyboard last write data(0x60) - command(0x64)
	KYBRD_CTRL_STATS_MASK_LOCKED	= 0x10,		// keyboard lock status
	KYBRD_CTRL_STATS_MASK_AUX_BUF	= 0x20,		// differs based on keyboard mode. AT mode when set maybe keyboard not present
	KYBRD_CTRL_STATS_MASK_TIMEOUT	= 0x40,		// differs based on keyboard mode. AT mode when set possible parity error
	KYBRD_CTRL_STATS_MASK_PARITY	= 0x80		// keyboard OK - parity error
};

// inside keyboard encoder available commands
enum KYBRD_ENC_CMDS
{
	KYBRD_ENC_CMD_SET_LED			= 0xED,		// set the keyboard leds
};

enum KEYCODE 
{
	// Alphanumeric keys

	KEY_SPACE = ' ',
	KEY_0 = '0',
	KEY_1 = '1',
	KEY_2 = '2',
	KEY_3 = '3',
	KEY_4 = '4',
	KEY_5 = '5',
	KEY_6 = '6',
	KEY_7 = '7',
	KEY_8 = '8',
	KEY_9 = '9',

	KEY_A = 'a',
	KEY_B = 'b',
	KEY_C = 'c',
	KEY_D = 'd',
	KEY_E = 'e',
	KEY_F = 'f',
	KEY_G = 'g',
	KEY_H = 'h',
	KEY_I = 'i',
	KEY_J = 'j',
	KEY_K = 'k',
	KEY_L = 'l',
	KEY_M = 'm',
	KEY_N = 'n',
	KEY_O = 'o',
	KEY_P = 'p',
	KEY_Q = 'q',
	KEY_R = 'r',
	KEY_S = 's',
	KEY_T = 't',
	KEY_U = 'u',
	KEY_V = 'v',
	KEY_W = 'w',
	KEY_X = 'x',
	KEY_Y = 'y',
	KEY_Z = 'z',

	KEY_RETURN		= '\r',
	KEY_ESCAPE		= 0x1001,
	KEY_BACKSPACE	= '\b',

	// Arrow keys

	KEY_UP		= 0x1100,
	KEY_DOWN	= 0x1101,
	KEY_LEFT	= 0x1102,
	KEY_RIGHT	= 0x1103,

	// Function keys /////////////////////

	KEY_F1		= 0x1201,
	KEY_F2		= 0x1202,
	KEY_F3		= 0x1203,
	KEY_F4		= 0x1204,
	KEY_F5		= 0x1205,
	KEY_F6		= 0x1206,
	KEY_F7		= 0x1207,
	KEY_F8		= 0x1208,
	KEY_F9		= 0x1209,
	KEY_F10		= 0x120a,
	KEY_F11		= 0x120b,
	KEY_F12		= 0x120b,
	KEY_F13		= 0x120c,
	KEY_F14		= 0x120d,
	KEY_F15		= 0x120e,

	KEY_DOT			= '.',
	KEY_COMMA		= ',',
	KEY_COLON		= ':',
	KEY_SEMICOLON	= ';',
	KEY_SLASH		= '/',
	KEY_BACKSLASH	= '\\',
	KEY_PLUS		= '+',
	KEY_MINUS		= '-',
	KEY_ASTERISK	= '*',
	KEY_EXCLAMATION = '!',
	KEY_QUESTION	= '?',
	KEY_QUOTEDOUBLE = '\"',
	KEY_QUOTE		= '\'',
	KEY_EQUAL		= '=',
	KEY_HASH		= '#',
	KEY_PERCENT		= '%',
	KEY_AMPERSAND	= '&',
	KEY_UNDERSCORE	= '_',
	KEY_LEFTPARENTHESIS		= '(',
	KEY_RIGHTPARENTHESIS	= ')',
	KEY_LEFTBRACKET			= '[',
	KEY_RIGHTBRACKET		= ']',
	KEY_LEFTCURL			= '{',
	KEY_RIGHTCURL			= '}',
	KEY_DOLLAR				= '$',
	KEY_POUND				= '£',
	KEY_EURO				= '$',
	KEY_LESS				= '<',
	KEY_GREATER				= '>',
	KEY_BAR					= '|',
	KEY_GRAVE				= '`',
	KEY_TILDE				= '~',
	KEY_AT					= '@',
	KEY_CARRET				= '^',

	// Numeric keypad

	KEY_KP_0		= '0',
	KEY_KP_1		= '1',
	KEY_KP_2		= '2',
	KEY_KP_3		= '3',
	KEY_KP_4		= '4',
	KEY_KP_5		= '5',
	KEY_KP_6		= '6',
	KEY_KP_7		= '7',
	KEY_KP_8		= '8',
	KEY_KP_9		= '9',
	KEY_KP_PLUS		= '+',
	KEY_KP_MINUS	= '-',
	KEY_KP_DECIMAL	= '.',
	KEY_KP_DIVIDE	= '/',
	KEY_KP_ASTERISK = '*',
	KEY_KP_NUMLOCK	= 0x300f,
	KEY_KP_ENTER	= 0x3010,

	KEY_TAB			= 0x4000,
	KEY_CAPSLOCK	= 0x4001,

	// Modify keys

	KEY_LSHIFT		= 0x4002,
	KEY_LCTRL		= 0x4003,
	KEY_LALT		= 0x4004,
	KEY_LWIN		= 0x4005,
	KEY_RSHIFT		= 0x4006,
	KEY_RCTRL		= 0x4007,
	KEY_RALT		= 0x4008,
	KEY_RWIN		= 0x4009,

	KEY_INSERT		= 0x400a,
	KEY_DELETE		= 0x400b,
	KEY_HOME		= 0x400c,
	KEY_END			= 0x400d,
	KEY_PAGEUP		= 0x400e,
	KEY_PAGEDOWN	= 0x400f,
	KEY_SCROLLLOCK	= 0x4010,
	KEY_PAUSE		= 0x4011,

	KEY_UNKNOWN,
	KEY_NUMKEYCODES
};

// returns whether keyboard controller is ready to receive commands. (input buffer empty)
extern volatile bool kybrd_ctrl_input_ready();

// reads the keybaord controller status byte (to be used with above masks)
uint8 kybrd_ctrl_read_status();

// sends a command to the keyboard controller
void kybrd_ctrl_send_cmd(uint8 cmd);

// reads the keybaord encoder status byte
uint8 kybrd_enc_read_status();

// sends a command to the keyboard encoder
void kybrd_enc_send_cmd(uint8 cmd);



void kybrd_set_leds(bool num, bool caps, bool scroll);

extern bool kybrd_get_scroll_lock();
extern bool kybrd_get_num_lock();
extern bool kybrd_get_caps_lock();

extern bool kybrd_get_alt();
extern bool kybrd_get_ctrl();
extern bool kybrd_get_shift();



extern void kybrd_disable();
extern void kybrd_enable();
extern bool kybrd_is_disabled();

extern void kybrd_reset_system();

#endif