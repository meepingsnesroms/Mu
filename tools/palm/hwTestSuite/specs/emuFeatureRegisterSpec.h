#ifndef EMU_FEATURE_REGISTER_SPEC_H
#define EMU_FEATURE_REGISTER_SPEC_H

/*
All emu feature registers are 32 bit accessing them in any other way will be undefined behavior.
These registers will do nothing if their corresponding feature bit is not set on launch.
*/

#define EMU_REG_ADDR(x) (0xFFFC0000 | x)

/*features*/
#define FEATURE_ACCURATE   0x00000000/*no hacks/addons*/
#define FEATURE_RAM_HUGE   0x00000001/*128mb RAM*/
#define FEATURE_FAST_CPU   0x00000002/*doubles CPU speed*/
#define FEATURE_HYBRID_CPU 0x00000004/*allows running ARM opcodes in an OS 4 enviroment*/
#define FEATURE_CUSTOM_FB  0x00000008/*creates a dynamicly sized framebuffer for hires mode, the 160x160 framebuffer is a transparent overlay over the extended framebuffer*/
#define FEATURE_SYNCED_RTC 0x00000010/*RTC always equals host system time*/
#define FEATURE_HLE_APIS   0x00000020/*memcpy, memcmp, wait on timer will be replaced with the hosts function*/
#define FEATURE_EMU_HONEST 0x00000040/*tell the OS that its running in an emu, does nothing else*/
#define FEATURE_EXT_KEYS   0x00000080/*enables the OS 5 buttons, left, right and select*/
#define FEATURE_DEBUG      0x00000100/*enables the debug commands, used to call Palm OS functions like native C functions*/
#define FEATURE_INVALID    0x00000200/*if this bit is set the returned data is invalid*/
#define FEATURE_SHELL      0x00000400/*allows executing code on the host machine, is a huge securty hole and is compiled out in releases*/
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
#define CMD_SET_ARM_STACK  0x0000FFF5/*EMU_VALUE = address of ARM stack, must be set before running ARM code*/
#define CMD_SET_RESOLUTION 0x0000FFF6/*EMU_VALUE >> 16 = width, EMU_VALUE & 0x0000FFFF = height*/
#define CMD_GET_KEYS       0x0000FFF7/*EMU_VALUE = OS 5 keys*/
#define CMD_PRINTF         0x0000FFF8/*EMU_SRC = pointer to string*/
#define CMD_SHELL_EXECUTE  0x0000FFF9/*execute shell commands from inside the emulator, will be used for a cool web project*/
#define CMD_SOUND          0x0000FFFA/*needed for OS 5 advanced sound*/
#define CMD_EXECUTION_DONE 0x0000FFFB/*terminates execution, used when a function is called from outside the emulator*/
#define CMD_IDLE_X_CLK32   0x0000FFFC/*used to remove idle loops*/
#define CMD_RUN_AS_M68K    0x0000FFFD/*emulStateP is ignored, EMU_SRC = argsOnStackP, EMU_SIZE = argsSizeAndwantA0, EMU_VALUE = trapOrFunction, on exit EMU_VALUE = Call68KFuncType() return value*/
#define CMD_RUN_AS_ARM     0x0000FFFE/*EMU_SRC = nativeFuncP, EMU_DST = userDataP, on exit EMU_VALUE = PceNativeCall() return value*/
#define CMD_SET_CYCLE_COST 0x0000FFFF/*EMU_DST = HLE API number, EMU_VALUE = how many cycles it takes*/

/*buttons*/
#define EXT_BUTTON_LEFT   0x01000000
#define EXT_BUTTON_RIGHT  0x02000000
#define EXT_BUTTON_SELECT 0x04000000

#endif
