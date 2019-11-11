/* Declarations for debug.c */
#ifndef H_DEBUG
#define H_DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

//#include "mmu.h"

#ifdef __cplusplus

#include <string>

extern "C" {
#endif

extern FILE *debugger_input;
#define gdb_connected false
#define in_debugger false
#define rdbg_port 0

enum DBG_REASON {
    DBG_USER,
    DBG_EXCEPTION,
    DBG_EXEC_BREAKPOINT,
    DBG_READ_BREAKPOINT,
    DBG_WRITE_BREAKPOINT,
};

/*
void *virt_mem_ptr(uint32_t addr, uint32_t size);
void backtrace(uint32_t fp);
int process_debug_cmd(char *cmdline);
void debugger(enum DBG_REASON reason, uint32_t addr);
void rdebug_recv(void);
bool rdebug_bind(unsigned int port);
void rdebug_quit();
*/
#define virt_mem_ptr(x, y) NULL
/*
static inline void *virt_mem_ptr(uint32_t addr, uint32_t size) {
   //this is needed by the disasembler
   // Note: this is not guaranteed to be correct when range crosses page boundary
   return (void *)(intptr_t)phys_mem_ptr(mmu_translate(addr, false, NULL, NULL), size);
}
*/
#define backtrace(x)
#define process_debug_cmd(x) 0
#define debugger(x, y)
#define rdebug_recv()
#define rdebug_bind(x) false
#define rdebug_quit()

#ifdef __cplusplus
}
#endif

#endif
