#pragma once

// dragonball register definitions

// SIM - System Integration Module
#define SCR       0x000 // System Control Register
#define PCR       0x003 // Peripheral control register
#define IDR       0x004 // Silicon ID register
#define IODCR     0x008 // I/O drive control register

// CS - Chip Select
#define GRPBASEA  0x100 // Chip Select Group A Base Register
#define GRPBASEC  0x104 // Chip Select Group C Base Register
#define GRPMASKA  0x108 // Chip Select Group A Mask Register
#define GRPMASKC  0x10C // Chip Select Group C Mask Register
#define CSA       0x110 // Group A Chip Select Register
#define CSC       0x114 // Group C Chip Select Register
#define CSD       0x116 // Group D Chip Select Register
#define EMUCS     0x118 // Emulation chip-select register

// PLL - Phase Locked Loop
#define PLLCR     0x200 // PLL Control Register
#define PLLFSR    0x202 // PLL Frequency Select Register
#define PCTLR     0x207 // Power Control Register

// INTR - Interrupt controller
#define IVR       0x300 // Interrupt Vector Register
#define ICR       0x302 // Interrupt Control Register
#define IMR       0x304 // Interrupt Mask Register
#define IWR       0x308 // Interrupt Wakeup Enable Register
#define ISR       0x30C // Interrupt Status Register
#define IPR       0x310 // Interrupt Pending Register

// PIO - Parallel IO
#define PCDIR     0x410 // Port C Direction Register
#define PCDATA    0x411 // Port C Data Register
#define PCSEL     0x413 // Port C Select Register
#define PDDIR     0x418 // Port D Direction Register
#define PDDATA    0x419 // Port D Data Register
#define PDPUEN    0x41A // Port D Pullup Enable Register
#define PDPOL     0x41C // Port D Polarity Register
#define PDIRQEN   0x41D // Port D IRQ Enable Register
#define PDIRQEDGE 0x41F // Port D IRQ Edge Register
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
#define PKDIR     0x440 // Port K Direction Register
#define PKDATA    0x441 // Port K Data Register
#define PKPUEN    0x442 // Port K Pullup Enable Register
#define PKSEL     0x443 // Port K Select Register
#define PMDIR     0x448 // Port M Direction Register
#define PMDATA    0x449 // Port M Data Register
#define PMPUEN    0x44A // Port M Pullup Enable Register
#define PMSEL     0x44B // Port M Select Register

// PWM - Pulse Width Modulator
#define PWMC      0x500 // PWM Control Register
#define PWMP      0x502 // PWM Period Register
#define PWMW      0x504 // PWM Width Register

// Timer
#define TCTL1     0x600 // Timer Unit 1 Control Register
#define TPRER1    0x602 // Timer Unit 1 Prescaler Register
#define TCMP1     0x604 // Timer Unit 1 Compare Register
#define TCN1      0x608 // Timer Unit 1 Counter
#define TSTAT1    0x60A // Timer Unit 1 Status Register
#define TCTL2     0x60C // Timer Unit 2 Control Register
#define TPRER2    0x60E // Timer Unit 2 Prescaler Register
#define TCMP2     0x610 // Timer Unit 2 Compare Register
#define TCN2      0x614 // Timer Unit 2 Counter
#define TSTAT2    0x616 // Timer Unit 2 Status Register

// WD - Watchdog
#define WCR       0x618 // Watchdog Control Register
#define WCN       0x61C // Watchdog Counter

// SPIM - Serial Peripheral Interface Master
#define SPIMDATA  0x800 // SPIM Data Register
#define SPIMCONT  0x802 // SPIM Control/Status Register

// UART - Universal Asynchronous Receiver/Transmitter
#define USTCNT    0x900 // UART Status/Control Register
#define UBAUD     0x902 // UART Baud Control Register
#define URX       0x904 // UART RX Register
#define UTX       0x906 // UART TX Register
#define UMISC     0x908 // UART Misc Register

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
#define LLBAR     0xA29 // Last Buffer Address Register
#define LOTCR     0xA2B // Octet Terminal Count Register
#define LFRCM     0xA31 // Frame Rate Control Modulation Register
#define LGPMR     0xA32 // Gray Palette Mapping Register

// RTC - Real Time Clock
#define RTCHMS    0xB00 // RTC Hours Minutes Seconds Register
#define RTCALARM  0xB04 // RTC Alarm Register
#define RTCCTL    0xB0C // RTC Control Register
#define RTCISR    0xB0E // RTC Interrupt Status Register
#define RTCIENR   0xB10 // RTC Interrupt Enable Register
