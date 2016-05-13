#include "pit.h"

uint32 frequency;

void init_pit_timer(uint32 _frequency, isr_t callback)
{
	register_interrupt_handler(32, callback);

	frequency = _frequency;

	// frequency must be such that count fits in a 16-bit variable.
	uint16 count = PIT_CLOCK_CYCLE / _frequency;
	//initialization command byte

	uint8 cw = RATE_GENERATOR | LSB_THEN_MSB | COUNTER_0;

	outportb(PIT_COMMAND_PORT, cw);

	uint8 l = (uint8)(count & 0x00FF);
	uint8 h = (uint8)((count >> 8) & 0x00FF);

	outportb(PIT_COUNT_0_PORT, l);	// send the least significand byte first
	outportb(PIT_COUNT_0_PORT, h);	// then send the most.
}

// Uses pit channel 2 to play a sound at frequency through the pc built-in speaker
void play_sound(uint32 frequency)
{
	uint16 count = PIT_CLOCK_CYCLE / frequency;

	// prepare initialization byte
	uint8 cw = SQR_WAVE_GENERATOR | LSB_THEN_MSB | COUNTER_2;
	outportb(PIT_COMMAND_PORT, cw);

	outportb(PIT_COUNT_2_PORT, (uint8)count);
	outportb(PIT_COUNT_2_PORT, (uint8)(count >> 8));

	uint8 speaker_status = inportb(PC_SPEAKER_PORT);
	if (speaker_status != (speaker_status | 3))	// make sure the 2 lsb bits are set
	{
		// bit 1: if set the state of the speaker follows bit 2
		// bit 2: if set the speaker connects to the pit, else it is disabled.	We want them both up

		outportb(PC_SPEAKER_PORT, speaker_status | 3);
	}
}

void stop_sound()
{
	uint8 temp = inportb(0x61) & 0xFC;	// FC = 1111 1100b so we unset the last two bits to disable the connection of the speaker with the pit
	outportb(PC_SPEAKER_PORT, temp);
}

/*void beep(uint32 frequency, uint32 time)
{
	Timer t;
	play_sound(frequency);
	while (t.GetElapsedMillis() < time);
	stop_sound();
}*/

/*void play_pattern(uint32 freq[], uint32 time[], uint32 size, uint32 mult)
{
	for (uint32 i = 0; i < size; i++)
	{
		beep(freq[i], time[i] * mult);
		sleep(5);
	}
}*/