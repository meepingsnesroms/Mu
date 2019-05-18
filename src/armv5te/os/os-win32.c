#include "os.h"

#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <conio.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <share.h>

#include "../emu.h"
#include "../mmu.h"

void *os_reserve(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void os_free(void *ptr, size_t size)
{
    (void) size;
    VirtualFree(ptr, 0, MEM_RELEASE);
}

#if OS_HAS_PAGEFAULT_HANDLER
void *os_commit(void *addr, size_t size)
{
    return VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
}

void *os_sparse_commit(void *page, size_t size)
{
    return VirtualAlloc(page, size, MEM_COMMIT, PAGE_READWRITE);
}

void os_sparse_decommit(void *page, size_t size)
{
    VirtualFree(page, size, MEM_DECOMMIT);
    return;
}
#endif

void *os_alloc_executable(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

#if OS_HAS_PAGEFAULT_HANDLER
static int addr_cache_exception(PEXCEPTION_RECORD er, void *x, void *y, void *z) {
    (void) x; (void) y; (void) z;
    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        if (addr_cache_pagefault((void *)er->ExceptionInformation[1]))
            return 0; // Continue execution
    }
    return 1; // Continue search
}
#endif

void addr_cache_init() {
    // Don't run more than once
    if(addr_cache)
        return;

    DWORD flags = MEM_RESERVE;

#if defined(AC_FLAGS)
    // Commit memory to not trigger segfaults which make debugging a PITA
    flags |= MEM_COMMIT;
#endif

    addr_cache = VirtualAlloc(NULL, AC_NUM_ENTRIES * sizeof(ac_entry), flags, PAGE_READWRITE);
    if(!addr_cache){
       printf("Cant allocate addr_cache!\n");
       exit(1);
    }

#if !defined(AC_FLAGS)
    unsigned int i;
    for(unsigned int i = 0; i < AC_NUM_ENTRIES; ++i)
    {
        AC_SET_ENTRY_INVALID(addr_cache[i], (i >> 1) << 10)
    }
#else
    memset(addr_cache, 0xFF, AC_NUM_ENTRIES * sizeof(ac_entry));
#endif

#if defined(__i386__) && !defined(NO_TRANSLATION)
    // Relocate the assembly code that wants addr_cache at a fixed address
    extern DWORD *ac_reloc_start[] __asm__("ac_reloc_start"), *ac_reloc_end[] __asm__("ac_reloc_end");
    DWORD **reloc;
    for (reloc = ac_reloc_start; reloc != ac_reloc_end; reloc++) {
        DWORD prot;
        VirtualProtect(*reloc, 4, PAGE_EXECUTE_READWRITE, &prot);
        **reloc += (DWORD)addr_cache;
        VirtualProtect(*reloc, 4, prot, &prot);
    }
#endif
}

#if OS_HAS_PAGEFAULT_HANDLER
void os_faulthandler_arm(os_exception_frame_t *frame)
{
    assert(frame->prev == NULL);

    frame->function = (void *)addr_cache_exception;
    asm ("movl %%fs:(%1), %0" : "=r" (frame->prev) : "r" (0));
    asm ("movl %0, %%fs:(%1)" : : "r" (frame), "r" (0));
}

void os_faulthandler_unarm(os_exception_frame_t *frame)
{
    assert(frame->prev != NULL);

    asm ("movl %0, %%fs:(%1)" : : "r" (frame->prev), "r" (0));
    frame->prev = NULL;
}
#endif

void addr_cache_deinit() {
    if(!addr_cache)
        return;

#if defined(__i386__) && !defined(NO_TRANSLATION)
    // Undo the relocations
    extern DWORD *ac_reloc_start[] __asm__("ac_reloc_start"), *ac_reloc_end[] __asm__("ac_reloc_end");
    DWORD **reloc;
    for (reloc = ac_reloc_start; reloc != ac_reloc_end; reloc++)
        **reloc -= (DWORD)addr_cache;
#endif

    VirtualFree(addr_cache, 0, MEM_RELEASE);
    addr_cache = NULL;
}
