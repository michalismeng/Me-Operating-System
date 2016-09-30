#ifdef __cplusplus
extern "C" {
#endif

#ifndef PIT_H
#define PIT_H

#include "system.h"
#include "isr.h"
	//#include "timer.h"

#define PIT_CLOCK_CYCLE			1193180	// hz

#define PIT_COMMAND_PORT		0x43
#define PC_SPEAKER_PORT			0x61

#define PIT_COUNT_0_PORT		0x40	// timer
#define PIT_COUNT_1_PORT		0x41	// not used. legacy for dram refresh
#define PIT_COUNT_2_PORT		0x42	// speaker

	// command word for setting the count register

#define BCD_FORMAT				1 << 0

	//operation modes see brokenthorn for more details

#define TERMINAL_COUNT			0 << 1
#define PROG_ONESHOT			1 << 1
#define RATE_GENERATOR			2 << 1	// used for the timer
#define SQR_WAVE_GENERATOR		3 << 1	// used for the speaker
#define SOFT_TRIGGER			4 << 1
#define HARD_TRIGGER			5 << 1

	/////////////////

	// read/write mode (count is 16 bits but the pit has 8 bits data pins so info must be split)

#define LATCH					0 << 4
#define LSB_ONLY				1 << 4		// least significand byte
#define MSB_ONLY				2 << 4		// most significand byte only
#define LSB_THEN_MSB			3 << 4

	/////////////////

	// select channel for counter

#define COUNTER_0				0 << 6
#define COUNTER_1				1 << 6
#define COUNTER_2				2 << 6

	/////////////////

	void init_pit_timer(uint32 frequency, isr_t callback);
	void play_sound(uint32 frequency);
	void stop_sound();
	void beep(uint32 frequency, uint32 time);

	//void play_pattern(uint32 freq[], uint32 time[], uint32 size, uint32 mult);

	extern uint32 frequency;

#endif

#ifdef __cplusplus
}
#endif