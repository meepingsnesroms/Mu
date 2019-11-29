
	.align 4
	.globl  disableInts
	.globl  enableInts
	
//uint32_t disableInts(void)
disableInts:
   MRS R0, CPSR
   ORR R1, R0, #0xC0
   MSR CPSR, R1
   BX LR

//void enableInts(uint32_t ints)
enableInts:
   MSR CPSR, R0
   BX LR

.pool
