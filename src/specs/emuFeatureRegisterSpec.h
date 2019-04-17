#ifndef EMU_FEATURE_REGISTER_SPEC_H
#define EMU_FEATURE_REGISTER_SPEC_H

/*
All emu feature registers are 32 bit accessing them in any other way will be undefined behavior.
These registers will do nothing if their corresponding feature bit is not set on launch.
*/

#define EMU_REG_ADDR(x) (0xFFFC0000 | x)

/*features*/
#define FEATURE_ACCURATE   0x00000000/*no hacks/addons*/
/*FEATURE_UNUSED           0x00000001*/
#define FEATURE_FAST_CPU   0x00000002/*allows the emulator to set its CPU speed*/
/*FEATURE_UNUSED           0x00000004*/
/*FEATURE_UNUSED           0x00000008*/
#define FEATURE_SYNCED_RTC 0x00000010/*RTC always equals host system time*/
#define FEATURE_HLE_APIS   0x00000020/*memcpy, memcmp, wait on timer will be replaced with the hosts function*/
/*FEATURE_UNUSED           0x00000040*/
/*FEATURE_UNUSED           0x00000080*/
#define FEATURE_DEBUG      0x00000100/*enables the debug commands, used to call Palm OS functions like native C functions*/
#define FEATURE_DURABLE    0x00000200/*ignore behavior that would crash a real device*/
/*FEATURE_UNUSED           0x00000400*/
/*FEATURE_UNUSED           0x00000800*/
/*new features go here*/

/*registers*/
#define EMU_INFO    0x000/*gets the feature bits, read only*/
#define EMU_SRC     0x004/*write only*/
#define EMU_DST     0x008/*write only*/
#define EMU_SIZE    0x00C/*write only*/
#define EMU_VALUE   0x010/*read/write*/
#define EMU_CMD     0x014/*the command actually runs once this register is written with a value, write only*/
/*new registers go here*/

/*commands*/
#define CMD_MEMCPY       0x00000000
#define CMD_MEMSET       0x00000001
#define CMD_MEMCMP       0x00000002
#define CMD_STRCPY       0x00000003
#define CMD_STRNCPY      0x00000004
#define CMD_STRCMP       0x00000005
#define CMD_STRNCMP      0x00000006
/*new HLE API cmds go here*/

/*new system cmds go here*/
#define CMD_SET_CPU_SPEED  0x0000FFF3/*EMU_VALUE = CPU speed percent, 100% = normal*/
#define CMD_IDLE_X_CLK32   0x0000FFF4/*EMU_VALUE = CLK32s to waste, used to remove idle loops*/
#define CMD_SET_CYCLE_COST 0x0000FFF5/*EMU_DST = HLE API number, EMU_VALUE = how many cycles it takes*/
#define CMD_LCD_SET_FB     0x0000FFF6/*EMU_SRC = framebuffer pointer(must be in RAM and word aligned, unused if size is 160x220), EMU_VALUE >> 16 = width, EMU_VALUE & 0xFFFF = height*/
/*CMD_UNUSED               0x0000FFF7*/
#define CMD_DEBUG_PRINT    0x0000FFF8/*EMU_SRC = pointer to string*/
#define CMD_DEBUG_WATCH    0x0000FFF9/*EMU_VALUE = 0/clear watch reference or 1/make code watch reference or 2/make data watch reference, EMU_SRC = address of area(or watch area reference number), EMU_SIZE = size of area(unused if clear operation), EMU_VALUE is set to the watch areas reference number after a set operation*/
/*CMD_UNUSED               0x0000FFFA*/
#define CMD_DEBUG_EXEC_END 0x0000FFFB/*terminates execution, used when a function is called from outside the emulator*/
/*CMD_UNUSED               0x0000FFFC*/
/*CMD_UNUSED               0x0000FFFD*/
/*CMD_UNUSED               0x0000FFFE*/
/*CMD_UNUSED               0x0000FFFF*/

#endif
