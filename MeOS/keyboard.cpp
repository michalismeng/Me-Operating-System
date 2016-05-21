#include "keyboard.h"

inline volatile bool kybrd_ctrl_input_ready()
{
	return ( (kybrd_ctrl_read_status() & KYBRD_CTRL_STATS_MASK_IN_BUF) == 0 );
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

void kybrd_set_leds(bool num, bool caps, bool scroll)
{
	uint8 data = 0;

	data = (scroll) ? (data | 1) : data;
	data = (num) ? (data | 2) : data;
	data = (caps) ? (data | 4) : data;

	kybrd_enc_send_cmd(KYBRD_ENC_CMD_SET_LED);
	kybrd_enc_send_cmd(data);
}
