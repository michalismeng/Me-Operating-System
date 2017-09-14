#ifndef KEYBOARD_H_180516
#define KEYBOARD_H_180516

#include "types.h"
#include "error.h"
#include "system.h"
#include "isr.h"

// inside keyboard encoder io ports
enum KYBRD_ENCODER_IO
{
	KYBRD_ENC_INPUT_BUF = 0x60,
	KYBRD_ENC_CMD_REG = 0x60
};

// on-motherboard keyboard controller io ports
enum KYBRD_CTRL_IO
{
	KYBRD_CTRL_STATS_REG = 0x64,
	KYBRD_CTRL_CMD_REG = 0x64
};

// keyboard controller status register masks
enum KYBRD_CTRL_STATS_MASK
{
	KYBRD_CTRL_STATS_MASK_OUT_BUF = 0x1,		// keyboard output buffer empty - full
	KYBRD_CTRL_STATS_MASK_IN_BUF = 0x2,			// keyboard input  buffer empty - full
	KYBRD_CTRL_STATS_MASK_SYSTEM = 0x4,			// keyboard system flag (after reset - BAT)
	KYBRD_CTRL_STATS_MASK_CMD_DATA = 0x8,		// keyboard last write data(0x60) - command(0x64)
	KYBRD_CTRL_STATS_MASK_LOCKED = 0x10,		// keyboard lock status
	KYBRD_CTRL_STATS_MASK_AUX_BUF = 0x20,		// differs based on keyboard mode. AT mode when set maybe keyboard not present
	KYBRD_CTRL_STATS_MASK_TIMEOUT = 0x40,		// differs based on keyboard mode. AT mode when set possible parity error
	KYBRD_CTRL_STATS_MASK_PARITY = 0x80			// keyboard OK - parity error
};

// inside keyboard encoder available commands
enum KYBRD_ENC_CMDS
{
	KYBRD_ENC_CMD_SET_LED = 0xED,					// set keyboard leds
	KYBRD_ENC_CMD_ECHO = 0xEE,						// returns 0xEE to port 0x60
	KYBRD_ENC_CMD_SCAN_CODE_SET = 0xF0,				// sets alternate scan code set
	KYBRD_ENC_CMD_ID = 0xF2,
	KYBRD_ENC_CMD_AUTODELAY = 0xF3,					// set autorepeat delay and rate
	KYBRD_ENC_CMD_ENABLE = 0xF4,					// enable keyboard
	KYBRD_ENC_CMD_RESETWAIT = 0xF5,					// reset and wait for enable
	KYBRD_ENC_CMD_RESETSCAN = 0xF6,					// reset and scan keyboard
	KYBRD_ENC_CMD_ALL_AUTO = 0xF7,					// set all keys to auto repeat (PS/2)
	KYBRD_ENC_CMD_ALL_MAKEBREAK = 0xF8,				// set all keys to send make and break code (PS/2)
	KYBRD_ENC_CMD_ALL_MAKEONLY = 0xF9,				// set all keys to send make code only
	KYBRD_ENC_CMD_ALL_MAKEBREAK_AUTO = 0xFA,		// set all keys to auto repeat and generate make/break codes
	KYBRD_ENC_CMD_SINGLE_AUTOREPEAT = 0xFB,			// set a single key to autorepeat
	KYBRD_ENC_CMD_SINGLE_MAKEBREAK = 0xFC,			// set a signle key to send make and break codes
	KYBRD_ENC_CMD_SINGLE_BREAKONLY = 0xFD,			// set a single to send break code only
	KYBRD_ENC_CMD_RESEND = 0xFE,					// resend last result
	KYBRD_ENC_CMD_RESET = 0xFF						// reset keyboard and start self test
};

// keyboard return codes
enum RETURN_CODES
{
	KYBRD_ERR_BUF_OVERRUN = 0,		// internal buffer overrun
	KYBRD_ERR_ID_RET = 0x83AB,		// keyboard return ID from above command
	KYBRD_ERR_BAT = 0xAA,			// Returned after BAT. Can also be L. Shift key make code
	KYBRD_ERR_ECHO_RET = 0xEE,		// Returned from echo command
	KYBRD_ERR_ACK = 0xFA,			// Keyboard acknowledge to command sent
	KYBRD_ERR_BAT_FAILED = 0xFC,	// BAT failed (PS/2)
	KYBRD_ERR_DIAG_FAILED = 0xFD,	// Diagnostic failure (no PS/2)
	KYBRD_ERR_RESEND_CMD = 0xFE,	// Keyboard requests to command resend
	KYBRD_ERR_KEY = 0xFF,			// Key error (PS/2)
	KYBRD_SELF_TEST = 0x55			// Keyboard self test return code
};

// keyboard controller available commands
enum KYBRD_CTRL_CMDS
{
	KYBRD_CTRL_CMD_READ = 0x20,
	KYBRD_CTRL_CMD_WRITE = 0x60,
	KYBRD_CTRL_CMD_SELF_TEST = 0xAA,			// controller self test
	KYBRD_CTRL_CMD_INTERFACE_TEST = 0xAB,		// controller serial interface test
	KYBRD_CTRL_CMD_DISABLE = 0xAD,				// disable keyboard
	KYBRD_CTRL_CMD_ENABLE = 0xAE,				// enable keyboard
	KYBRD_CTRL_CMD_READ_IN_PORT = 0xC0,
	KYBRD_CTRL_CMD_READ_OUT_PORT = 0xD0,
	KYBRD_CTRL_CMD_WRITE_OUT_PORT = 0xD1,
	KYBRD_CTRL_CMD_READ_TEST_INPUTS = 0xE0,
	KYBRD_CTRL_CMD_SYSTEM_RESET = 0xFE,			// resets the cpu
	KYBRD_CTRL_CMD_MOUSE_DISABLE = 0xA7,
	KYBRD_CTRL_CMD_MOUSE_ENABLE = 0xA8,
	KYBRD_CTRL_CMD_MOUSE_PORT_TEST = 0xA9,
	KYBRD_CTRL_CMD_MOUSE_WRITE = 0xD4
};

enum KEYCODE : uint32
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

	KEY_RETURN = '\r',
	KEY_ESCAPE = 0x1001,
	KEY_BACKSPACE = '\b',

	// Arrow keys

	KEY_UP = 0x1100,
	KEY_DOWN = 0x1101,
	KEY_LEFT = 0x1102,
	KEY_RIGHT = 0x1103,

	// Function keys /////////////////////

	KEY_F1 = 0x1201,
	KEY_F2 = 0x1202,
	KEY_F3 = 0x1203,
	KEY_F4 = 0x1204,
	KEY_F5 = 0x1205,
	KEY_F6 = 0x1206,
	KEY_F7 = 0x1207,
	KEY_F8 = 0x1208,
	KEY_F9 = 0x1209,
	KEY_F10 = 0x120a,
	KEY_F11 = 0x120b,
	KEY_F12 = 0x120b,
	KEY_F13 = 0x120c,
	KEY_F14 = 0x120d,
	KEY_F15 = 0x120e,

	KEY_DOT = '.',
	KEY_COMMA = ',',
	KEY_COLON = ':',
	KEY_SEMICOLON = ';',
	KEY_SLASH = '/',
	KEY_BACKSLASH = '\\',
	KEY_PLUS = '+',
	KEY_MINUS = '-',
	KEY_ASTERISK = '*',
	KEY_EXCLAMATION = '!',
	KEY_QUESTION = '?',
	KEY_QUOTEDOUBLE = '\"',
	KEY_QUOTE = '\'',
	KEY_EQUAL = '=',
	KEY_HASH = '#',
	KEY_PERCENT = '%',
	KEY_AMPERSAND = '&',
	KEY_UNDERSCORE = '_',
	KEY_LEFTPARENTHESIS = '(',
	KEY_RIGHTPARENTHESIS = ')',
	KEY_LEFTBRACKET = '[',
	KEY_RIGHTBRACKET = ']',
	KEY_LEFTCURL = '{',
	KEY_RIGHTCURL = '}',
	KEY_DOLLAR = '$',
	KEY_POUND = '£',
	KEY_EURO = '$',
	KEY_LESS = '<',
	KEY_GREATER = '>',
	KEY_BAR = '|',
	KEY_GRAVE = '`',
	KEY_TILDE = '~',
	KEY_AT = '@',
	KEY_CARRET = '^',

	// Numeric keypad

	KEY_KP_0 = '0',
	KEY_KP_1 = '1',
	KEY_KP_2 = '2',
	KEY_KP_3 = '3',
	KEY_KP_4 = '4',
	KEY_KP_5 = '5',
	KEY_KP_6 = '6',
	KEY_KP_7 = '7',
	KEY_KP_8 = '8',
	KEY_KP_9 = '9',
	KEY_KP_PLUS = '+',
	KEY_KP_MINUS = '-',
	KEY_KP_DECIMAL = '.',
	KEY_KP_DIVIDE = '/',
	KEY_KP_ASTERISK = '*',
	KEY_KP_NUMLOCK = 0x300f,
	KEY_KP_ENTER = 0x3010,

	KEY_TAB = 0x4000,
	KEY_CAPSLOCK = 0x4001,

	// Modify keys

	KEY_LSHIFT = 0x4002,
	KEY_LCTRL = 0x4003,
	KEY_LALT = 0x4004,
	KEY_LWIN = 0x4005,
	KEY_RSHIFT = 0x4006,
	KEY_RCTRL = 0x4007,
	KEY_RALT = 0x4008,
	KEY_RWIN = 0x4009,

	KEY_INSERT = 0x400a,
	KEY_DELETE = 0x400b,
	KEY_HOME = 0x400c,
	KEY_END = 0x400d,
	KEY_PAGEUP = 0x400e,
	KEY_PAGEDOWN = 0x400f,
	KEY_SCROLLLOCK = 0x4010,
	KEY_PAUSE = 0x4011,

	KEY_UNKNOWN,
	KEY_NUMKEYCODES
};

// returns whether keyboard controller is ready to receive commands. (input buffer empty)
extern volatile bool kybrd_ctrl_input_ready();

// returns whether keyboard controller is ready... data waiting at the output buffer
extern volatile bool kybrd_ctrl_output_ready();

// reads the keybaord controller status byte (to be used with above masks)
uint8 kybrd_ctrl_read_status();

// sends a command to the keyboard controller
void kybrd_ctrl_send_cmd(uint8 cmd);

// reads the keybaord encoder status byte
uint8 kybrd_enc_read_status();

// sends a command to the keyboard encoder
void kybrd_enc_send_cmd(uint8 cmd);

bool kybrd_get_scroll_lock();
bool kybrd_get_num_lock();
bool kybrd_get_caps_lock();

bool kybrd_get_alt();
bool kybrd_get_ctrl();
bool kybrd_get_shift();

void kybrd_ignore_resend();
bool kybrd_check_resend();

bool kybrd_get_diagnostic_res();
bool kybrd_get_bat_res();
bool kybrd_self_test();

uint8 kybrd_get_last_scan();
KEYCODE kybrd_get_last_key();
void kybrd_discard_last_key();

void kybrd_set_leds(bool num, bool caps, bool scroll);

char kybrd_key_to_ascii(KEYCODE code);

void kybrd_disable();
void kybrd_enable();
bool kybrd_is_disabled();

void kybrd_reset_system();
bool kybrd_is_disabled();

void keyboard_callback(registers_t* regs);
void init_keyboard();

#endif