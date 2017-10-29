#include "keyboard.h"
#include "vfs.h"
#include "process.h"
#include "thread_sched.h"
#include "queue_lf.h"
#include "print_utility.h"
#include "kernel_stack.h"

// driver private data

uint32 _scancode;
queue_lf<uint8> keycode_buffer;
queue_lf<uint32> user_buffer;
int code = 0;								// the code received by the keyboard irq
bool _numlock, _capslock, _scrolllock;
bool _shift, _alt, _ctrl;
int _kybrd_error = 0;
bool _kybrd_BAT_res = false;
bool _kybrd_diag_res = false;
bool _kybrd_resend_res = false;
bool _kybrd_disable = false;

TCB* keyboard_daemon = 0;
TCB* active = 0;
size_t kybd_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address);
error_t kybd_open(vfs_node* node, uint32 capabilities);
error_t kybd_ioctl(vfs_node* node, uint32 command, ...);

static fs_operations kybd_operations =
{
	kybd_read,		// read
	NULL,			// write
	kybd_open,		// open
	NULL,			// close
	NULL,			// sync
	NULL,			// lookup
	kybd_ioctl		// ioctl?
};

size_t kybd_read(uint32 fd, vfs_node* file, uint32 start, size_t count, virtual_addr address)
{
	size_t temp_count = count;
	uint8* buffer = (uint8*)address;
	active = thread_get_current();
	// perhaps we will need to empty the user_buffer at some point

	while (temp_count > 0)
	{
		if (queue_lf_is_empty(&user_buffer) == false)
		{
			*buffer = queue_lf_peek(&user_buffer);
			queue_lf_remove(&user_buffer);
			temp_count--;
			buffer++;
		}
		else
			thread_block(thread_get_current());
	}

	active = 0;
	return count;
}

error_t kybd_open(vfs_node* node, uint32 capabilities)
{
	return ERROR_OK;
}

error_t kybd_ioctl(vfs_node* node, uint32 command, ...)
{
	return ERROR_OK;
}

// original scane set where array index == make code.
KEYCODE _kybrd_scancode_std[] =
{
	//! key			scancode
	KEY_UNKNOWN,	//0
	KEY_ESCAPE,		//1
	KEY_1,			//2
	KEY_2,			//3
	KEY_3,			//4
	KEY_4,			//5
	KEY_5,			//6
	KEY_6,			//7
	KEY_7,			//8
	KEY_8,			//9
	KEY_9,			//0xa
	KEY_0,			//0xb
	KEY_MINUS,		//0xc
	KEY_EQUAL,		//0xd
	KEY_BACKSPACE,	//0xe
	KEY_TAB,		//0xf
	KEY_Q,			//0x10
	KEY_W,			//0x11
	KEY_E,			//0x12
	KEY_R,			//0x13
	KEY_T,			//0x14
	KEY_Y,			//0x15
	KEY_U,			//0x16
	KEY_I,			//0x17
	KEY_O,			//0x18
	KEY_P,			//0x19
	KEY_LEFTBRACKET,//0x1a
	KEY_RIGHTBRACKET,//0x1b
	KEY_RETURN,		//0x1c
	KEY_LCTRL,		//0x1d
	KEY_A,			//0x1e
	KEY_S,			//0x1f
	KEY_D,			//0x20
	KEY_F,			//0x21
	KEY_G,			//0x22
	KEY_H,			//0x23
	KEY_J,			//0x24
	KEY_K,			//0x25
	KEY_L,			//0x26
	KEY_SEMICOLON,	//0x27
	KEY_QUOTE,		//0x28
	KEY_GRAVE,		//0x29
	KEY_LSHIFT,		//0x2a
	KEY_BACKSLASH,	//0x2b
	KEY_Z,			//0x2c
	KEY_X,			//0x2d
	KEY_C,			//0x2e
	KEY_V,			//0x2f
	KEY_B,			//0x30
	KEY_N,			//0x31
	KEY_M,			//0x32
	KEY_COMMA,		//0x33
	KEY_DOT,		//0x34
	KEY_SLASH,		//0x35
	KEY_RSHIFT,		//0x36
	KEY_KP_ASTERISK,//0x37
	KEY_RALT,		//0x38
	KEY_SPACE,		//0x39
	KEY_CAPSLOCK,	//0x3a
	KEY_F1,			//0x3b
	KEY_F2,			//0x3c
	KEY_F3,			//0x3d
	KEY_F4,			//0x3e
	KEY_F5,			//0x3f
	KEY_F6,			//0x40
	KEY_F7,			//0x41
	KEY_F8,			//0x42
	KEY_F9,			//0x43
	KEY_F10,		//0x44
	KEY_KP_NUMLOCK,	//0x45
	KEY_SCROLLLOCK,	//0x46
	KEY_HOME,		//0x47
	KEY_KP_8,		//0x48	//keypad up arrow
	KEY_PAGEUP,		//0x49
	KEY_KP_2,		//0x50	//keypad down arrow
	KEY_KP_3,		//0x51	//keypad page down
	KEY_KP_0,		//0x52	//keypad insert key
	KEY_KP_DECIMAL,	//0x53	//keypad delete key
	KEY_UNKNOWN,	//0x54
	KEY_UNKNOWN,	//0x55
	KEY_UNKNOWN,	//0x56
	KEY_F11,		//0x57
	KEY_F12			//0x58
};

// used to indicate last scan code is not to be re-used.
const int INVALID_SCAN_CODE = 0;

inline volatile bool kybrd_ctrl_input_ready()
{
	return ((kybrd_ctrl_read_status() & KYBRD_CTRL_STATS_MASK_IN_BUF) == 0);
}

inline volatile bool kybrd_ctrl_output_ready()
{
	return ((kybrd_ctrl_read_status() & KYBRD_CTRL_STATS_MASK_OUT_BUF) != 0);
}

uint8 kybrd_ctrl_read_status()
{
	return inportb(KYBRD_CTRL_STATS_REG);
}

void kybrd_ctrl_send_cmd(uint8 cmd)
{
	while (kybrd_ctrl_input_ready() == false);

	outportb(KYBRD_CTRL_CMD_REG, cmd);
}

uint8 kybrd_enc_read_status()
{
	return inportb(KYBRD_ENC_INPUT_BUF);
}

void kybrd_enc_send_cmd(uint8 cmd)
{
	// commands to the inside keyboard enconder go through the on motherboard controller so wait to clear input buffer
	while (kybrd_ctrl_input_ready() == false);

	outportb(KYBRD_ENC_CMD_REG, cmd);
}

bool kybrd_get_scroll_lock()
{
	return _scrolllock;
}

bool kybrd_get_num_lock()
{
	return _numlock;
}

bool kybrd_get_caps_lock()
{
	return _capslock;
}

bool kybrd_get_alt()
{
	return _alt;
}

bool kybrd_get_ctrl()
{
	return _ctrl;
}

bool kybrd_get_shift()
{
	return _shift;
}

void kybrd_ignore_resend()
{
	_kybrd_resend_res = false;
}

bool kybrd_check_resend()
{
	return _kybrd_resend_res;
}

bool kybrd_get_diagnostic_res()
{
	return _kybrd_diag_res;
}

bool kybrd_get_bat_res()
{
	return _kybrd_BAT_res;
}

bool kybrd_self_test()
{
	kybrd_ctrl_send_cmd(KYBRD_CTRL_CMD_SELF_TEST);

	while (kybrd_ctrl_output_ready() == false);

	return (kybrd_enc_read_status() & KYBRD_SELF_TEST);
}

uint8 kybrd_get_last_scan()
{
	return _scancode;
}

KEYCODE kybrd_get_last_key()
{
	if (_scancode == INVALID_SCAN_CODE)
		return KEY_UNKNOWN;

	return (KEYCODE)_kybrd_scancode_std[_scancode];
}

void kybrd_discard_last_key()
{
	_scancode = INVALID_SCAN_CODE;
}

void kybrd_set_leds(bool num, bool caps, bool scroll)
{
	uint8 data = 0;

	data = (scroll) ? (data | 1) : data;
	data = (num) ? (data | 2) : data;
	data = (caps) ? (data | 4) : data;

	kybrd_enc_send_cmd(KYBRD_ENC_CMD_SET_LED);
	kybrd_enc_send_cmd(data);
}

char kybrd_key_to_ascii(KEYCODE code)
{
	uint8 key = code;

	// TODO: Check if code is actually ascii character

	if (isalpha(key))
	{
		// only when either of shift or caps is pressed (but not simultaneously) do uppercase
		if (!_shift && _capslock || _shift && !_capslock)
			key = toupper(key);
	}

	if (_shift)
	{
		if (isdigit(key))
		{
			// map numbers keys (not at numpad) to their upper key counterparts (see keyboard layout)
			switch (key)
			{
			case '0':	key = KEY_RIGHTPARENTHESIS; break;
			case '1':	key = KEY_EXCLAMATION; break;
			case '2':	key = KEY_AT; break;
			case '3':	key = KEY_HASH; break;
			case '4':	key = KEY_DOLLAR; break;
			case '5':	key = KEY_PERCENT; break;
			case '6':	key = KEY_CARRET; break;
			case '7':	key = KEY_AMPERSAND; break;
			case '8':	key = KEY_ASTERISK; break;
			case '9':	key = KEY_LEFTPARENTHESIS; break;
			}
		}
		else
		{
			// map every other key to its shift coutnerpart
			switch (key)
			{
			case KEY_COMMA:			key = KEY_LESS; break;
			case KEY_DOT:			key = KEY_GREATER; break;
			case KEY_SLASH:			key = KEY_QUESTION; break;
			case KEY_SEMICOLON:		key = KEY_COLON; break;
			case KEY_QUOTE:			key = KEY_QUOTEDOUBLE; break;
			case KEY_LEFTBRACKET:	key = KEY_LEFTCURL; break;
			case KEY_RIGHTBRACKET:	key = KEY_RIGHTCURL; break;
			case KEY_GRAVE:			key = KEY_TILDE; break;
			case KEY_MINUS:			key = KEY_UNDERSCORE; break;
			case KEY_PLUS:			key = KEY_EQUAL; break;
			case KEY_BACKSLASH:		key = KEY_BAR; break;
			}
		}
	}

	return key;
}

void kybrd_disable()
{
	kybrd_ctrl_send_cmd(KYBRD_CTRL_CMD_DISABLE);
	_kybrd_disable = true;
}

void kybrd_enable()
{
	kybrd_ctrl_send_cmd(KYBRD_CTRL_CMD_ENABLE);
	_kybrd_disable = false;
}

void kybrd_reset_system()
{
	kybrd_ctrl_send_cmd(KYBRD_CTRL_CMD_SYSTEM_RESET);
}

bool kybrd_is_disabled()
{
	return _kybrd_disable;
}

void keyboard_callback(registers_t* regs)
{
	// ensure keyboard controller buffer has data
	if (kybrd_ctrl_input_ready())
	{
		// read the code into the buffer
		queue_lf_insert(&keycode_buffer, kybrd_enc_read_status());

		// notify the daemon of the incoming code
		if (keyboard_daemon != 0)
			thread_notify(keyboard_daemon);
	}
}

void keyboard_irq()
{
	// run forever
	while (true)
	{
		while (queue_lf_is_empty(&keycode_buffer) == false)
		{
			code = queue_lf_peek(&keycode_buffer);
			queue_lf_remove(&keycode_buffer);

			// TODO : ADD EXTENDED CODE CAPABILITY
			if (code == 0xE0 || code == 0xE1)
			{
				DEBUG("Hit keyboard extended capability");
			}

			//break code specific to Original Scan Set(test bit 7)
			if (code & 0x80)
			{
				code -= 0x80;	// retrieve make code

				int key = _kybrd_scancode_std[code];	// retrieve textual character based on Original Scan Set

														// handle special key release
				switch (key)
				{
				case KEY_LSHIFT:
				case KEY_RSHIFT:
					_shift = false;
					break;

				case KEY_LCTRL:
				case KEY_RCTRL:
					_ctrl = false;
					break;

				case KEY_LALT:
				case KEY_RALT:
					_alt = false;
					break;
				default:
					break;
				}
			}
			// this is a make code
			else
			{
				_scancode = code;
				uint32 key = _kybrd_scancode_std[code];
				// handle special key press
				switch (key)
				{
				case KEY_LSHIFT:
				case KEY_RSHIFT:
					_shift = true;
					break;

				case KEY_LCTRL:
				case KEY_RCTRL:
					_ctrl = true;
					break;

				case KEY_LALT:
				case KEY_RALT:
					_alt = true;
					break;

				case KEY_CAPSLOCK:
					_capslock = !_capslock;
					kybrd_set_leds(_numlock, _capslock, _scrolllock);
					break;

				case KEY_KP_NUMLOCK:
					_numlock = !_numlock;
					kybrd_set_leds(_numlock, _capslock, _scrolllock);
					break;

				case KEY_SCROLLLOCK:
					_scrolllock = !_scrolllock;
					kybrd_set_leds(_numlock, _capslock, _scrolllock);
					break;
				default:
					break;
				}

				// if there is an active thread, write the data to it.
				if (active != 0)
				{
					queue_lf_insert(&user_buffer, key);
					thread_notify(active);
				}
			}

			// watch for any errors
			switch (code)
			{
			case KYBRD_ERR_BAT_FAILED:
				DEBUG("BAT Error");
				_kybrd_BAT_res = false;
				break;
			case KYBRD_ERR_DIAG_FAILED:
				DEBUG("Diagnostic Error");
				_kybrd_diag_res = false;
				break;
			case KYBRD_ERR_RESEND_CMD:
				DEBUG("Resend requested");
				_kybrd_resend_res = true;
				break;
			default:
				break;
			}
		}

		thread_block(thread_get_current());
	}
}

void init_keyboard()
{
	register_interrupt_handler(33, keyboard_callback);
	vfs_create_device("keyboard", DEVICE_DEFAULT_CAPS, 0, 0, &kybd_operations);

	queue_lf_init(&keycode_buffer, 10);
	queue_lf_init(&user_buffer, 10);

	_kybrd_BAT_res = true;							// assume true.. will be checked at the irq handler right above
	_scancode = INVALID_SCAN_CODE;
	_numlock = _capslock = _scrolllock = false;
	_shift = _alt = _ctrl = false;

	kybrd_set_leds(false, false, false);

	virtual_addr krnl_stack = kernel_stack_reserve();
	if (krnl_stack == 0)
		PANIC("keyboard stack allocation failed");

	// parent is the kernel init thread
	keyboard_daemon = thread_create(thread_get_current()->parent, (uint32)keyboard_irq, krnl_stack, 4 KB, 1, 0);
	thread_insert(keyboard_daemon);
	thread_block(keyboard_daemon);
}