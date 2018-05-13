#ifndef EMU_FEATURE_REGISTERS_SPEC_HEADER
#define EMU_FEATURE_REGISTERS_SPEC_HEADER

/*
All emu feature registers are 32 bit accessing them in any other way will be undefined behavior.
These registers will do nothing it there corresponding feature bit is not set on launch.
*/

#define EMU_REGISTER_BASE 0xFFFFE000/*just before the m68k hardware registers, in the same bank*/
#define EMU_REGISTER_SIZE 0x1000
#define EMU_REG_ADDR(x) (EMU_REGISTER_BASE | x)

/*features*/
#define FEATURE_ACCURATE     0x00000000/*no hacks/addons*/
#define FEATURE_RAM_HUGE     0x00000001/*128mb RAM*/
#define FEATURE_FAST_CPU     0x00000002/*doubles CPU speed*/
#define FEATURE_HYBRID_CPU   0x00000004/*allows running ARM opcodes in an OS 4 enviroment*/
#define FEATURE_320x320      0x00000008/*creates a 320x320 framebuffer for hires mode, the 160x160 framebuffer is a transparent overlay over the 320x320 framebuffer*/
#define FEATURE_SYNCED_RTC   0x00000010/*RTC always equals host system time*/
#define FEATURE_HLE_APIS     0x00000020/*memcpy, memcmp, wait on timer will be replaced with the hosts function*/
#define FEATURE_EMU_HONEST   0x00000040/*tell the OS that its running in an emu, does nothing else*/
#define FEATURE_EMU_EXT_KEYS 0x00000080/*enables the OS 5 buttons, left, right and select*/
/*new features go here*/

/*registers*/
#define EMU_INFO    0x000/*gets the feature bits, read only*/
#define EMU_HIRESFB 0x004/*sets the address of the 320x320 framebuffer, read/write*/
#define EMU_SRC     0x008/*write only*/
#define EMU_DST     0x00C/*write only*/
#define EMU_SIZE    0x010/*write only*/
#define EMU_VALUE   0x014/*read/write*/
#define EMU_CMD     0x018/*the command actually runs once this register is written with a value, write only*/
#define EMU_KEYS    0x01C/*read only, stores the extra left/right/select keys that OS 4 palms lack*/
/*new registers go here*/


/*commands*/
#define EMU_CMD_KEY 0xF1EA/*must be the top 16 bits of command to trigger execution, prevents programs that are write testing this address space from executing commands*/

#define CMD_MEMCPY       0x0000
#define CMD_MEMSET       0x0001
#define CMD_MEMCMP       0x0002
#define CMD_STRCPY       0x0003
#define CMD_STRNCPY      0x0004
#define CMD_STRCMP       0x0005
#define CMD_STRNCMP      0x0006
/*new HLE API cmds go here*/

/*new system cmds go here*/
#define CMD_IDLE_X_CLK32   0xFFFC/*used to remove idle loops*/
#define CMD_RUN_AS_M68K    0xFFFD/*emulStateP is ignored, EMU_SRC = argsOnStackP, EMU_SIZE = argsSizeAndwantA0, EMU_VALUE = trapOrFunction, on exit EMU_VALUE = Call68KFuncType() return value*/
#define CMD_RUN_AS_ARM     0xFFFE/*EMU_SRC = nativeFuncP, EMU_DST = userDataP, on exit EMU_VALUE = PceNativeCall() return value*/
#define CMD_SET_CYCLE_COST 0xFFFF/*EMU_DST = HLE API number, EMU_VALUE = how many cycles it takes*/

#define MAKE_EMU_CMD(cmd) ((EMU_CMD_KEY << 16) | cmd)


/*buttons*/
#define EXT_BUTTON_LEFT   0x01000000
#define EXT_BUTTON_RIGHT  0x02000000
#define EXT_BUTTON_SELECT 0x04000000

#endif
