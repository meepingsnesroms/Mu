#ifndef EMU_FEATURE_REGISTERS_SPEC_HEADER
#define EMU_FEATURE_REGISTERS_SPEC_HEADER
/*
All emu feature registers are 32 bit accessing them in any other way will be undefined behavior.
These registers will do nothing it there corresponding feature bit is not set on launch.
*/

#define EMU_REGISTER_BASE 0xFFFFE000/*just before the m68k hardware registers*/

/*features*/
#define ACCURATE             0x00000000/*no hacks/addons*/
#define FEATURE_RAM_HUGE     0x00000001/*128 mb ram*/
#define FEATURE_FAST_CPU     0x00000002/*doubles cpu speed*/
#define FEATURE_HYBRID_CPU   0x00000004/*allows running arm opcodes in an OS 4 enviroment*/
#define FEATURE_320x320      0x00000008/*creates a 320x320 framebuffer for hires mode, the 320x320 framebuffer is a transparent overlay over the 160x160 framebuffer*/
#define FEATURE_SYNCED_RTC   0x00000010/*rtc always equals host system time*/
#define FEATURE_HLE_APIS     0x00000020/*memcpy, memcmp, wait on timer will be replaced with the hosts function*/
/*new features go here*/

/*registers*/
#define EMU_INFO    0x000/*gets the feature bits, read only*/
#define EMU_HIRESFB 0x004/*gets the address of the 320x320 framebuffer, read only*/
#define EMU_SRC     0x008/*used for hle apis, write only*/
#define EMU_DST     0x00C/*used for hle apis, write only*/
#define EMU_SIZE    0x010/*used for hle apis, write only*/
#define EMU_VALUE   0x014/*used for hle apis, read/write*/
#define EMU_CMD     0x018/*used for hle apis, the hle api is actually run when this register is written with a value, write only*/
/*new registers go here*/


/*commands*/
#define EMU_CMD_KEY 0x12F7/*must be the top 16 bits of command to trigger execution, prevents programs that are write testing this address space from executing commands*/

#define CMD_MEMCPY       0x0000
#define CMD_MEMSET       0x0001
#define CMD_MEMCMP       0x0002
#define CMD_STRCPY       0x0003
#define CMD_STRNCPY      0x0004
#define CMD_STRCMP       0x0005
#define CMD_STRNCMP      0x0006
/*new hle api cmds go here*/

/*new system cmds go here*/
#define CMD_WASTE_CYCLES 0xFFFF

#define MAKE_EMU_CMD(cmd) ((EMU_CMD_KEY << 16) | cmd)
#endif
