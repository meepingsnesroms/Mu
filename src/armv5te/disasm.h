#ifndef H_DISASM
#define H_DISASM

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char disasmReturnBuf[80];

uint32_t disasm_arm_insn(uint32_t pc);
uint32_t disasm_thumb_insn(uint32_t pc);

#ifdef __cplusplus
}
#endif

#endif
