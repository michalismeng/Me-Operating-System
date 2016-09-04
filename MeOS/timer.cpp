#include "timer.h"

volatile uint32 ticks;					// volatile is necessary for the sleep function, where compiler thinks millis is constant valued
extern uint16 cursorX, cursorY;

void timer_callback(registers_t* regs)
{
	ticks++;
	uint16 x = cursorX, y = cursorY;

	SetCursor(0, SCREEN_HEIGHT - 2);
	printf("%u", ticks);
	SetCursor(x, y);
}

uint32 millis()
{
	return (1000 * ticks) / frequency;
}

void sleep(uint32 _time)
{
	uint32 start = millis();
	while (millis() < start + _time);
}

Timer::Timer()
{
	start = millis();
}

uint32 Timer::Restart()
{
	uint32 temp = GetElapsedMillis();
	start = millis();
	return temp;
}

volatile uint32 Timer::GetElapsedMillis()
{
	return millis() - start;
}