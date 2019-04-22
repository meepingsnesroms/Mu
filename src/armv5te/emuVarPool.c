#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "cpu.h"
#include "mem.h"
#include "emu.h"


/* cycle_count_delta is a (usually negative) number telling what the time is relative
 * to the next scheduled event. See sched.c */
int cycle_count_delta = 0;

int throttle_delay = 10; /* in milliseconds */

bool exiting, debug_on_start, debug_on_warn, print_on_warn;
bool do_translate;

int log_enabled[MAX_LOG];

void *restart_after_exception[32];

uint32_t cpu_events;
