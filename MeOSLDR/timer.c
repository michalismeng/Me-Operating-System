#include "timer.h"

volatile uint32 ticks;					// volatile is necessary for the sleep function, where compiler thinks millis is constant valued

void timer_callback(registers_t* regs)
{
	ticks++;
}

uint32 millis()
{
	if (frequency == 0)
		PANIC("Zero freq");
	return (1000 * ticks) / frequency;
}

void sleep(uint32 _time)
{
	uint32 start = millis();
	while (millis() < start + _time);
}