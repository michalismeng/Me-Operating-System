#ifndef CRITLOCK_27092017
#define CRITLOCK_27092017

#include "types.h"

// critical section lock.

// When a thread acquires this lock, it cannot be interrupted.
// If the scheduler interrupts a thread inside the critical lock section, then execution of thread is continued until the lock is released and thread yields.
// This lock is only for kernel use!

void critlock_acquire();
void critlock_release();

#endif