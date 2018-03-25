#include <stdint.h>

#include "emulator.h"

//the sed1376 has only 16 address lines(17 if you count the line that switches between registers and framebuffer) and 16 data lines, the most you can read is 16 bits, registers are 8 bits

//the actions described below are just my best guesses after reading the datasheet, I have not tested with actual hardware
//you read and write the register on the address lines set
//8 bit reg access works normal
//16 bit reg reads will result in you getting (0x00 << 8 | register).
//16 bit reg writes will result in you writing the lower 8 bits.
//32 bit register reads will result in you getting (randomUint16 << 16 | 0x00 << 8 | register), upper 16 bits are floating because sed1376 only has 16 address lines.
//32 bit register writes will result in you writing the lower 8 bits.

uint8_t sed1376Framebuffer[0x14000];

void sed1376SetRegister(unsigned int address, unsigned int value){
   
}

unsigned int sed1376GetRegister(unsigned int address){
   
}
