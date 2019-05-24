#ifndef _H_EMU
#define _H_EMU

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <setjmp.h>

#include "cpu.h"
#include "mem.h"

#include "../emulator.h"
#include "../portability.h"

#ifdef __cplusplus
extern "C" {
#endif

// Can also be set manually
#if !defined(__i386__) && !defined(__x86_64__) && !(defined(__arm__) && !defined(__thumb__)) && !(defined(__aarch64__))
#define NO_TRANSLATION
#endif

static inline uint16_t BSWAP16(uint16_t x) { return x << 8 | x >> 8; }
#define BSWAP32(x) __builtin_bswap32(x)

extern int cycle_count_delta __asm__("cycle_count_delta");
extern uint32_t cpu_events __asm__("cpu_events");
#define EVENT_IRQ 1
#define EVENT_FIQ 2
#define EVENT_RESET 4
#define EVENT_DEBUG_STEP 8
#define EVENT_WAITING 16

// Settings
extern bool exiting;
extern bool do_translate;

enum { LOG_CPU, LOG_IO, LOG_FLASH, LOG_INTS, LOG_ICOUNT, LOG_USB, LOG_GDB, MAX_LOG };
#define LOG_TYPE_TBL "CIFQ#UG"
//void logprintf(int type, const char *str, ...);
//void emuprintf(const char *format, ...);
#define logprintf(type, ...) debugLog(__VA_ARGS__)
#define emuprintf(...) debugLog(__VA_ARGS__)

//void warn(const char *fmt, ...);
#define warn(...) debugLog(__VA_ARGS__)
//__attribute__((noreturn)) void error(const char *fmt, ...);
#define error(...) abort()

// Is actually a jmp_buf, but __builtin_*jmp is used instead
// as the MinGW variant is buggy
extern jmp_buf restart_after_exception;

// GUI callbacks
#define gui_debug_printf(...) debugLog(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
