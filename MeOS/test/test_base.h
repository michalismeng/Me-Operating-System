#ifndef TEST_BASE_H_28102017
#define TEST_BASE_H_28102017

#include "../types.h"
#include "../SerialDebugger.h"

#define FAIL(x) { serial_printf(x, get_last_error()); return false; }
#define SUCCESS(x) { serial_printf(x); return true; }
#define RET_SUCCESS { serial_printf("--------Test is successful---------\n\n"); return true; }

#endif
