#ifndef DBVZ_H
#define DBVZ_H

#include <stdint.h>
#include <stdbool.h>

//interrupt names
#define DBVZ_INT_EMIQ  0x00800000//level 7
#define DBVZ_INT_RTI   0x00400000//level 4
#define DBVZ_INT_SPI1  0x00200000//level 1<->6, configurable, datasheet is contraditory on this one
#define DBVZ_INT_IRQ5  0x00100000//level 5
#define DBVZ_INT_IRQ6  0x00080000//level 6
#define DBVZ_INT_IRQ3  0x00040000//level 3
#define DBVZ_INT_IRQ2  0x00020000//level 2
#define DBVZ_INT_IRQ1  0x00010000//level 1
#define DBVZ_INT_PWM2  0x00002000//level 1<->6, configurable
#define DBVZ_INT_UART2 0x00001000//level 1<->6, configurable
#define DBVZ_INT_INT3  0x00000800//level 4
#define DBVZ_INT_INT2  0x00000400//level 4
#define DBVZ_INT_INT1  0x00000200//level 4
#define DBVZ_INT_INT0  0x00000100//level 4
#define DBVZ_INT_PWM1  0x00000080//level 6
#define DBVZ_INT_KB    0x00000040//level 4
#define DBVZ_INT_TMR2  0x00000020//level 1<->6, configurable
#define DBVZ_INT_RTC   0x00000010//level 4
#define DBVZ_INT_WDT   0x00000008//level 4
#define DBVZ_INT_UART1 0x00000004//level 4
#define DBVZ_INT_TMR1  0x00000002//level 6
#define DBVZ_INT_SPI2  0x00000001//level 4

//reasons a timer is triggered
#define DBVZ_TIMER_REASON_SYSCLK 0x00
#define DBVZ_TIMER_REASON_TIN    0x01
#define DBVZ_TIMER_REASON_CLK32  0x02

//chip names
enum{
   DBVZ_CHIP_BEGIN = 0,
   DBVZ_CHIP_A0_ROM = 0,
   DBVZ_CHIP_A1_USB,
   DBVZ_CHIP_B0_SED,
   DBVZ_CHIP_B1_NIL,
   //DBVZ_CHIP_CX_RAM, //CSC* is owned by CSD during normal operation
   DBVZ_CHIP_DX_RAM,
   DBVZ_CHIP_00_EMU,
   DBVZ_CHIP_REGISTERS,
   DBVZ_CHIP_NONE,
   DBVZ_CHIP_END
};

//types
typedef struct{
   bool     enable;
   uint32_t start;
   uint32_t lineSize;//the size of a single chip select line, multiply by 2 to get the range size for RAM
   uint32_t mask;//the address lines the chip responds to, so 0x10000 on an chip with 16 address lines will return the value at 0x0000

   //attributes
   bool     inBootMode;
   bool     readOnly;
   bool     readOnlyForProtectedMemory;
   bool     supervisorOnlyProtectedMemory;
   uint32_t unprotectedSize;
}dbvz_chip_t;

//variables
extern dbvz_chip_t dbvzChipSelects[];
extern uint8_t     dbvzReg[];//needed for direct execution of the DBVZ regs without a RAM access function
extern uint16_t*   dbvzFramebuffer;
extern uint16_t    dbvzFramebufferWidth;
extern uint16_t    dbvzFramebufferHeight;

//CPU
void dbvzLcdRender(void);
bool dbvzIsPllOn(void);
bool m515BacklightAmplifierState(void);
bool dbvzAreRegistersXXFFMapped(void);
bool sed1376ClockConnected(void);
void ads7846OverridePenState(bool value);
void m5XXRefreshTouchState(void);//just refreshes the touchscreen
void m5XXRefreshInputState(void);//refreshes touchscreen, buttons and docked status
//int32_t interruptAcknowledge(int32_t intLevel);//this is in m68kexternal.h

//memory errors
void dbvzSetBusErrorTimeOut(uint32_t address, bool isWrite);
void dbvzSetPrivilegeViolation(uint32_t address, bool isWrite);
void dbvzSetWriteProtectViolation(uint32_t address);

//memory accessors
uint8_t dbvzGetRegister8(uint32_t address);
uint16_t dbvzGetRegister16(uint32_t address);
uint32_t dbvzGetRegister32(uint32_t address);
void dbvzSetRegister8(uint32_t address, uint8_t value);
void dbvzSetRegister16(uint32_t address, uint16_t value);
void dbvzSetRegister32(uint32_t address, uint32_t value);

//config
void dbvzReset(void);
void dbvzLoadBootloader(uint8_t* data, uint32_t size);
void dbvzSetRtc(uint16_t days, uint8_t hours, uint8_t minutes, uint8_t seconds);
uint32_t dbvzStateSize(void);
void dbvzSaveState(uint8_t* data);
void dbvzLoadState(uint8_t* data);
void dbvzLoadStateFinished(void);

void dbvzExecute(void);

#endif
