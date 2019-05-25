#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emu.h"
#include "os/os.h"
#include "mem.h"
#include "translate.h"

uint8_t   (*read_byte_map[64])(uint32_t addr);
uint16_t  (*read_half_map[64])(uint32_t addr);
uint32_t  (*read_word_map[64])(uint32_t addr);
void (*write_byte_map[64])(uint32_t addr, uint8_t value);
void (*write_half_map[64])(uint32_t addr, uint16_t value);
void (*write_word_map[64])(uint32_t addr, uint32_t value);

/* For invalid/unknown physical addresses */
uint8_t bad_read_byte(uint32_t addr)               { warn("Bad read_byte: %08x", addr); return 0; }
uint16_t bad_read_half(uint32_t addr)              { warn("Bad read_half: %08x", addr); return 0; }
uint32_t bad_read_word(uint32_t addr)              { warn("Bad read_word: %08x", addr); return 0; }
void bad_write_byte(uint32_t addr, uint8_t value)  { warn("Bad write_byte: %08x %02x", addr, value); }
void bad_write_half(uint32_t addr, uint16_t value) { warn("Bad write_half: %08x %04x", addr, value); }
void bad_write_word(uint32_t addr, uint32_t value) { warn("Bad write_word: %08x %08x", addr, value); }

uint8_t *mem_and_flags = NULL;
struct mem_area_desc mem_areas[2];

void *phys_mem_ptr(uint32_t addr, uint32_t size) {
    unsigned int i;
    for (i = 0; i < sizeof(mem_areas)/sizeof(*mem_areas); i++) {
        uint32_t offset = addr - mem_areas[i].base;
        if (offset < mem_areas[i].size && size <= mem_areas[i].size - offset)
            return mem_areas[i].ptr + offset;
    }
    return NULL;
}

uint32_t phys_mem_addr(void *ptr) {
    int i;
    for (i = 0; i < 3; i++) {
        uint32_t offset = (uint8_t *)ptr - mem_areas[i].ptr;
        if (offset < mem_areas[i].size)
            return mem_areas[i].base + offset;
    }
    return -1; // should never happen
}

void read_action(void *ptr) {
    // this is just debugging stuff
    /*
    uint32_t addr = phys_mem_addr(ptr);
    if (!gdb_connected)
        emuprintf("Hit read breakpoint at %08x. Entering debugger.\n", addr);
    debugger(DBG_READ_BREAKPOINT, addr);
    */
}

void write_action(void *ptr) {
    // this is just debugging stuff
    uint32_t addr = phys_mem_addr(ptr);
    uint32_t *flags = &RAM_FLAGS((size_t)ptr & ~3);
    /*
    // this is just debugging stuff
    if (*flags & RF_WRITE_BREAKPOINT) {
        if (!gdb_connected)
            emuprintf("Hit write breakpoint at %08x. Entering debugger.\n", addr);
        debugger(DBG_WRITE_BREAKPOINT, addr);
    }
    */
#ifndef NO_TRANSLATION
    if (*flags & RF_CODE_TRANSLATED) {
        logprintf(LOG_CPU, "Wrote to translated code at %08x. Deleting translations.\n", addr);
        invalidate_translation(*flags >> RFS_TRANSLATION_INDEX);
    } else {
        *flags &= ~RF_CODE_NO_TRANSLATE;
    }
    *flags &= ~RF_CODE_EXECUTED;
#endif
}

/* 00000000, 10000000, A4000000: ROM and RAM */
uint8_t memory_read_byte(uint32_t addr) {
    uint8_t *ptr = phys_mem_ptr(addr, 1);
    if (!ptr) return bad_read_byte(addr);
    if (RAM_FLAGS((size_t)ptr & ~3) & DO_READ_ACTION) read_action(ptr);
    return *ptr;
}
uint16_t memory_read_half(uint32_t addr) {
    uint16_t *ptr = phys_mem_ptr(addr, 2);
    if (!ptr) return bad_read_half(addr);
    if (RAM_FLAGS((size_t)ptr & ~3) & DO_READ_ACTION) read_action(ptr);
    return *ptr;
}
uint32_t memory_read_word(uint32_t addr) {
    uint32_t *ptr = phys_mem_ptr(addr, 4);
    if (!ptr) return bad_read_word(addr);
    if (RAM_FLAGS(ptr) & DO_READ_ACTION) read_action(ptr);
    return *ptr;
}
void memory_write_byte(uint32_t addr, uint8_t value) {
    uint8_t *ptr = phys_mem_ptr(addr, 1);
    if (!ptr) { bad_write_byte(addr, value); return; }
    uint32_t flags = RAM_FLAGS((size_t)ptr & ~3);
    if (flags & RF_READ_ONLY) { bad_write_byte(addr, value); return; }
    if (flags & DO_WRITE_ACTION) write_action(ptr);
    *ptr = value;
}
void memory_write_half(uint32_t addr, uint16_t value) {
    uint16_t *ptr = phys_mem_ptr(addr, 2);
    if (!ptr) { bad_write_half(addr, value); return; }
    uint32_t flags = RAM_FLAGS((size_t)ptr & ~3);
    if (flags & RF_READ_ONLY) { bad_write_half(addr, value); return; }
    if (flags & DO_WRITE_ACTION) write_action(ptr);
    *ptr = value;
}
void memory_write_word(uint32_t addr, uint32_t value) {
    uint32_t *ptr = phys_mem_ptr(addr, 4);
    if (!ptr) { bad_write_word(addr, value); return; }
    uint32_t flags = RAM_FLAGS(ptr);
    if (flags & RF_READ_ONLY) { bad_write_word(addr, value); return; }
    if (flags & DO_WRITE_ACTION) write_action(ptr);
    *ptr = value;
}

uint32_t FASTCALL mmio_read_byte(uint32_t addr) {
    return read_byte_map[addr >> 26](addr);
}
uint32_t FASTCALL mmio_read_half(uint32_t addr) {
    return read_half_map[addr >> 26](addr);
}
uint32_t FASTCALL mmio_read_word(uint32_t addr) {
    return read_word_map[addr >> 26](addr);
}
void FASTCALL mmio_write_byte(uint32_t addr, uint32_t value) {
    write_byte_map[addr >> 26](addr, value);
}
void FASTCALL mmio_write_half(uint32_t addr, uint32_t value) {
    write_half_map[addr >> 26](addr, value);
}
void FASTCALL mmio_write_word(uint32_t addr, uint32_t value) {
    write_word_map[addr >> 26](addr, value);
}
