#ifndef SPINLOCK_H_02012017 // Happy New Year
#define SPINLOCK_H_02012017

#include "types.h"
#include "utility.h"
#include "thread_sched.h"

// UP spinlock. Very lightweight. No blocking queues.
// If resource is locked => thread will sleep and retry
typedef uint32 spinlock;

void spinlock_init(spinlock* lock);
void spinlock_acquire(spinlock* lock);
bool spinlock_try_acquire(spinlock* lock);
void spinlock_release(spinlock* lock);

#endif