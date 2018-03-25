#pragma once

// Dragonball VZ Hrdware Register Definitions
//"verified" means that the definitions above it have been checked against the dragonball vz datasheet, this file was for the original dragonball and some things arent in the same location anymore

// SIM - System Integration Module
#define SCR       0x000 // System Control Register
#define PCR       0x003 // Peripheral control register
#define IDR       0x004 // Silicon ID register
#define IODCR     0x008 // I/O drive control register
//verified

// CS - Chip Select
#define CSGBA     0x100 // Chip Select Group A Base Register
#define CSGBB     0x102 // Chip Select Group B Base Register
#define CSGBC     0x104 // Chip Select Group C Base Register
#define CSGBD     0x106 // Chip Select Group D Base Register
#define CSUGBA    0x108 // Chip Select Upper Group Address Register
#define CSA       0x110 // Group A Chip Select Register
#define CSC       0x114 // Group C Chip Select Register
#define CSD       0x116 // Group D Chip Select Register
#define EMUCS     0x118 // Emulation chip-select register
//verified

// PLL - Phase Locked Loop
#define PLLCR     0x200 // PLL Control Register
#define PLLFSR    0x202 // PLL Frequency Select Register
#define PCTLR     0x207 // Power Control Register
//verified

// INTR - Interrupt controller
#define IVR       0x300 // Interrupt Vector Register
#define ICR       0x302 // Interrupt Control Register
#define IMR       0x304 // Interrupt Mask Register
// Reserved       0x308 // Interrupt Wakeup Enable Register On Original Dragonball
#define ISR       0x30C // Interrupt Status Register
#define IPR       0x310 // Interrupt Pending Register
#define ILCR      0x314 // Interrupt Level Control Register
//verified

// PIO - Parallel IO
#define PADIR     0x400 // Port A Direction Register
#define PADATA    0x401 // Port A Data Register
#define PAPUEN    0x402 // Port A Pull-Down Enable Register

#define PBDIR     0x408 // Port B Direction Register
#define PBDATA    0x409 // Port B Data Register
#define PBPUEN    0x40A // Port B Pull-Down Enable Register
#define PBSEL     0x40B // Port B Select Register

#define PCDIR     0x410 // Port C Direction Register
#define PCDATA    0x411 // Port C Data Register
#define PCPUEN    0x412 // Port C Pull-Down Enable Register
#define PCSEL     0x413 // Port C Select Register

#define PDDIR     0x418 // Port D Direction Register
#define PDDATA    0x419 // Port D Data Register
#define PDPUEN    0x41A // Port D Pullup Enable Register
#define PDSEL     0x41B // Port D Select Register
#define PDPOL     0x41C // Port D Polarity Register
#define PDIRQEN   0x41D // Port D IRQ Enable Register
#define PDKBEN    0x41E // Port D Keyboard Enable Register
#define PDIRQEG   0x41F // Port D IRQ Edge Register

#define PEDIR     0x420 // Port E Direction Register
#define PEDATA    0x421 // Port E Data Register
#define PEPUEN    0x422 // Port E Pullup Enable Register
#define PESEL     0x423 // Port E Select Register

#define PFDIR     0x428 // Port F Direction Register
#define PFDATA    0x429 // Port F Data Register
#define PFPUEN    0x42A // Port F Pullup Enable Register
#define PFSEL     0x42B // Port F Select Register

#define PGDIR     0x430 // Port G Direction Register
#define PGDATA    0x431 // Port G Data Register
#define PGPUEN    0x432 // Port G Pullup Enable Register
#define PGSEL     0x433 // Port G Select Register

#define PJDIR     0x438 // Port J Direction Register
#define PJDATA    0x439 // Port J Data Register
#define PJPUEN    0x43A // Port J Pullup Enable Register
#define PJSEL     0x43B // Port J Select Register

#define PKDIR     0x440 // Port K Direction Register
#define PKDATA    0x441 // Port K Data Register
#define PKPUEN    0x442 // Port K Pullup Enable Register
#define PKSEL     0x443 // Port K Select Register

#define PMDIR     0x448 // Port M Direction Register
#define PMDATA    0x449 // Port M Data Register
#define PMPUEN    0x44A // Port M Pullup Enable Register
#define PMSEL     0x44B // Port M Select Register
//verified

// PWM - Pulse Width Modulator
#define PWMC1      0x500 // PWM Unit 1 Control Register
#define PWMS1      0x502 // PWM Unit 1 Sample Register
#define PWMP1      0x504 // PWM Unit 1 Period Register
#define PWMCNT1    0x505 // PWM Unit 1 Counter Register
//verified

// Timer
#define TCTL1     0x600 // Timer Unit 1 Control Register
#define TPRER1    0x602 // Timer Unit 1 Prescaler Register
#define TCMP1     0x604 // Timer Unit 1 Compare Register
#define TCN1      0x608 // Timer Unit 1 Counter
#define TSTAT1    0x60A // Timer Unit 1 Status Register
#define TCTL2     0x610 // Timer Unit 2 Control Register
#define TPRER2    0x612 // Timer Unit 2 Prescaler Register
#define TCMP2     0x614 // Timer Unit 2 Compare Register
#define TCN2      0x618 // Timer Unit 2 Counter
#define TSTAT2    0x61A // Timer Unit 2 Status Register
//verified

// WD - Watchdog
#define WATCHDOG    0xB0A // Watchdog Timer Register
//verified

// SPI - Serial Peripheral Interface
#define SPIDATA2  0x800 // SPI Unit 2 Data Register
#define SPICONT2  0x802 // SPI Unit 2 Control/Status Register
//verified

// UART - Universal Asynchronous Receiver/Transmitter
#define USTCNT1    0x900 // UART Unit 1 Status/Control Register
#define UBAUD1     0x902 // UART Unit 1 Baud Control Register
#define URX1       0x904 // UART Unit 1 RX Register
#define UTX1       0x906 // UART Unit 1 TX Register
#define UMISC1     0x908 // UART Unit 1 Misc Register
//verified

// LCDC - LCD Controller
#define LSSA      0xA00 // Screen Starting Address Register
#define LVPW      0xA05 // Virtual Page Width Register
#define LXMAX     0xA08 // Screen Width Register
#define LYMAX     0xA0A // Screen Height Register
#define LCXP      0xA18 // Cursor X Position
#define LCYP      0xA1A // Cursor Y Position
#define LCWCH     0xA1C // Cursor Width & Height Register
#define LBLKC     0xA1F // Blink Control Register
#define LPICF     0xA20 // Panel Interface Config Register
#define LPXCD     0xA25 // Pixel Clock Divider Register
#define LCKCON    0xA27 // Clocking Control Register
#define LRRA      0xA29 // LCD Refresh Rate Adjustment Register
// Reserved       0xA2B // Octet Terminal Count Register On Original Dragonball
#define LFRCM     0xA31 // Frame Rate Control Modulation Register
#define LGPMR     0xA33 // Gray Palette Mapping Register
//verified

// RTC - Real Time Clock
#define RTCTIME   0xB00 // RTC Time Of Day Register
#define RTCALRM   0xB04 // RTC Alarm Register
#define RTCCTL    0xB0C // RTC Control Register
#define RTCISR    0xB0E // RTC Interrupt Status Register
#define RTCIENR   0xB10 // RTC Interrupt Enable Register
#define DAYR      0xB1A // RTC Day Count Register
//verified
