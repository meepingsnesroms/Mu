#ifndef M68KOPS_HEADER
#define M68KOPS_HEADER

#include <stdint.h>

/* ======================================================================== */
/* ============================ OPCODE HANDLERS =========================== */
/* ======================================================================== */


/* Build the opcode handler table */
void m68ki_build_opcode_table(void);

extern void (*m68ki_instruction_jump_table[0x10000])(void); /* opcode handler jump table */
extern uint8_t m68ki_cycles[][0x10000];


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68KOPS__HEADER */


