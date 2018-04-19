#pragma once
//SED1376 Register Definitions

//Configuration Registers
#define REV_CODE       0x00
#define DISP_BUFF_SIZE 0x01
#define CFG_READBACK   0x02
#define MEM_CLK        0x04
#define PIXEL_CLK      0x05

//Look Up Table Registers
#define LUT_R_WRITE    0x08
#define LUT_G_WRITE    0x09
#define LUT_B_WRITE    0x0A
#define LUT_WRITE_LOC  0x0B
#define LUT_R_READ     0x0C
#define LUT_G_READ     0x0D
#define LUT_B_READ     0x0E
#define LUT_READ_LOC   0x0F

//Panel Configuration Registers

//Display Mode Registers

//Picture-in-Picture Plus (PIP+) Registers

//Miscellaneous Registers
#define PWR_SAVE_CFG   0xA0
#define SCRATCH_0      0xA4
#define SCRATCH_1      0xA5

//General Purpose IO Pins Registers

//PWM Clock and CV Pulse Configuration Registers
