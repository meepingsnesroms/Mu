#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <setjmp.h>

#include "cpu.h"
#include "mem.h"
#include "emu.h"

int waitingFiqs;
int waitingIrqs;
/* cycle_count_delta is a (usually negative) number telling what the time is relative
 * to the next scheduled event. See sched.c */
int cycle_count_delta;

bool exiting;
bool do_translate;

jmp_buf restart_after_exception;

uint32_t cpu_events;
