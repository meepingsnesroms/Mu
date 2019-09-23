#ifndef _PXA260_UART_H_
#define _PXA260_UART_H_

#include "pxa260_CPU.h"
#include "pxa260_IC.h"


/*
	PXA260 UARTs
	
	PXA260 has three. they are identical, but at diff base addresses. this implements one. instanciate more than one of this struct to make all 3 work if needed.
	PURRPOSE: this is how linux talks to us :)


	by default we read nothing and write nowhere (buffer drains fast into nothingness)
	this can be changed by addidng appropriate callbacks

*/

#define PXA260_FFUART_BASE	0x40100000UL
#define PXA260_BTUART_BASE	0x40200000UL
#define PXA260_STUART_BASE	0x40700000UL
#define PXA260_UART_SIZE	0x00010000UL

#define UART_FIFO_DEPTH		64


#define UART_CHAR_BREAK		0x800
#define UART_CHAR_FRAME_ERR	0x400
#define UART_CHAR_PAR_ERR	0x200
#define UART_CHAR_NONE		0x100

typedef UInt16	(*Pxa260UartReadF)(void* userData);
typedef void	(*Pxa260UartWriteF)(UInt16 chr, void* userData);

#define UART_FIFO_EMPTY	0xFF

typedef struct{

	UInt8 read;
	UInt8 write;
	UInt16 buf[UART_FIFO_DEPTH];

}UartFifo;

typedef struct{

   Pxa260ic* ic;
	UInt32 baseAddr;
	
   Pxa260UartReadF readF;
   Pxa260UartWriteF writeF;
	void* accessFuncsData;
	
	UartFifo TX, RX;
	
	UInt16 transmitShift;	//char currently "sending"
	UInt16 transmitHolding;	//holding register for no-fifo mode
	
	UInt16 receiveHolding;	//char just received
	
	UInt8 irq:5;
	UInt8 cyclesSinceRecv:3;
	
	UInt8 IER;		//interrupt enable register
	UInt8 IIR;		//interrupt information register
	UInt8 FCR;		//fifo control register
	UInt8 LCR;		//line control register
	UInt8 LSR;		//line status register
	UInt8 MCR;		//modem control register
	UInt8 MSR;		//modem status register
	UInt8 SPR;		//scratchpad register
	UInt8 DLL;		//divisor latch low
	UInt8 DLH;		//divior latch high;
	UInt8 ISR;		//infrared selection register
	
	
	
}Pxa260uart;

void pxa260uartInit(Pxa260uart* uart, Pxa260ic* ic, UInt32 baseAddr, UInt8 irq);
void pxa260uartProcess(Pxa260uart* uart);		//write out data in TX fifo and read data into RX fifo

void pxa260uartSetFuncs(Pxa260uart* uart, Pxa260UartReadF readF, Pxa260UartWriteF writeF, void* userData);

#endif

