#include "timer.h"
#include "thread_sched.h"
#include "print_utility.h"

volatile uint32 ticks;					// volatile is necessary for the sleep function, where compiler thinks millis is constant valued
extern uint16 cursorX, cursorY;

// this function is never called - see kmain first lines
void timer_callback(registers_t* regs)
{
	ticks++;
	uint16 x = cursorX, y = cursorY;

	SetCursor(0, SCREEN_HEIGHT - 2);
	printf("t=%u m=%u", ticks, millis());
	SetCursor(x, y);
}

uint32 millis()
{
	if (frequency == 0)
		PANIC("Zero freq");
	return (1000 * ticks) / frequency;
}

uint32 get_ticks()
{
	return ticks;
}

void sleep(uint32 _time)
{
	thread_sleep(thread_get_current_node(), _time);
	//uint32 start = millis();
	//while (millis() < start + _time);
	//thread_sleep(thread_get_current(), _time);
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