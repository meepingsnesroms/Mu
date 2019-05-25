/* Declarations for memory.c */

#ifndef _H_MEM
#define _H_MEM

#include <stdint.h>

#include "cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_MAXSIZE (70*1024*1024) // also defined as RAM_FLAGS in asmcode.S

extern uint8_t   (*read_byte_map[64])(uint32_t addr);
extern uint16_t  (*read_half_map[64])(uint32_t addr);
extern uint32_t  (*read_word_map[64])(uint32_t addr);
extern void (*write_byte_map[64])(uint32_t addr, uint8_t value);
extern void (*write_half_map[64])(uint32_t addr, uint16_t value);
extern void (*write_word_map[64])(uint32_t addr, uint32_t value);

// Must be allocated below 2GB (see comments for mmu.c)
extern uint8_t *mem_and_flags;
struct mem_area_desc {
    uint32_t base, size;
    uint8_t *ptr;
};
extern struct mem_area_desc mem_areas[2];
void *phys_mem_ptr(uint32_t addr, uint32_t size);
uint32_t phys_mem_addr(void *ptr);

/* Each word of memory has a flag word associated with it. For fast access,
 * flags are located at a constant offset from the memory data itself.
 *
 * These can't be per-byte because a translation index wouldn't fit then.
 * This does mean byte/halfword accesses have to mask off the low bits to
 * check flags, but the alternative would be another 32MB of memory overhead. */
#define RAM_FLAGS(memptr) (*(uint32_t *)((uint8_t *)(memptr) + MEM_MAXSIZE))

#define RF_READ_BREAKPOINT   1
#define RF_WRITE_BREAKPOINT  2
#define RF_EXEC_BREAKPOINT   4
#define RF_EXEC_DEBUG_NEXT   8
#define RF_CODE_EXECUTED     16
#define RF_CODE_TRANSLATED   32
#define RF_CODE_NO_TRANSLATE 64
#define RF_READ_ONLY         128
#define RF_ARMLOADER_CB      256
#define RFS_TRANSLATION_INDEX 9

#define DO_READ_ACTION (RF_READ_BREAKPOINT)
#define DO_WRITE_ACTION (RF_WRITE_BREAKPOINT | RF_CODE_TRANSLATED | RF_CODE_NO_TRANSLATE | RF_CODE_EXECUTED)
#define DONT_TRANSLATE (RF_EXEC_BREAKPOINT | RF_EXEC_DEBUG_NEXT | RF_CODE_TRANSLATED | RF_CODE_NO_TRANSLATE)

uint8_t bad_read_byte(uint32_t addr);
uint16_t bad_read_half(uint32_t addr);
uint32_t bad_read_word(uint32_t addr);
void bad_write_byte(uint32_t addr, uint8_t value);
void bad_write_half(uint32_t addr, uint16_t value);
void bad_write_word(uint32_t addr, uint32_t value);

void write_action(void *ptr) __asm__("write_action");
void read_action(void *ptr) __asm__("read_action");

uint8_t memory_read_byte(uint32_t addr);
uint16_t memory_read_half(uint32_t addr);
uint32_t memory_read_word(uint32_t addr);
void memory_write_byte(uint32_t addr, uint8_t value);
void memory_write_half(uint32_t addr, uint16_t value);
void memory_write_word(uint32_t addr, uint32_t value);

uint32_t FASTCALL mmio_read_byte(uint32_t addr) __asm__("mmio_read_byte");
uint32_t FASTCALL mmio_read_half(uint32_t addr) __asm__("mmio_read_half");
uint32_t FASTCALL mmio_read_word(uint32_t addr) __asm__("mmio_read_word");
void FASTCALL mmio_write_byte(uint32_t addr, uint32_t value) __asm__("mmio_write_byte");
void FASTCALL mmio_write_half(uint32_t addr, uint32_t value) __asm__("mmio_write_half");
void FASTCALL mmio_write_word(uint32_t addr, uint32_t value) __asm__("mmio_write_word");

#ifdef __cplusplus
}
#endif

#endif
