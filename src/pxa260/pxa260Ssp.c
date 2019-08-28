#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pxa260.h"
#include "pxa260_IC.h"
#include "pxa260Timing.h"
#include "../tsc2101.h"
#include "../emulator.h"


#define PXA260_SSP_TRANSFER_DURATION 10

#define SSCR0 0x0000
#define SSCR1 0x0004
#define SSSR 0x0008
#define SSDR 0x0010


uint32_t pxa260SspSscr0;
uint32_t pxa260SspSscr1;
uint16_t pxa260SspRxFifo[17];
uint16_t pxa260SspTxFifo[17];
uint8_t  pxa260SspRxReadPosition;
uint8_t  pxa260SspRxWritePosition;
bool     pxa260SspRxOverflowed;
uint8_t  pxa260SspTxReadPosition;
uint8_t  pxa260SspTxWritePosition;
bool     pxa260SspTransfering;


static uint8_t pxa260SspRxFifoEntrys(void){
   //check for wraparound
   if(pxa260SspRxWritePosition < pxa260SspRxReadPosition)
      return pxa260SspRxWritePosition + 17 - pxa260SspRxReadPosition;
   return pxa260SspRxWritePosition - pxa260SspRxReadPosition;
}

static uint16_t pxa260SspRxFifoRead(void){
   if(pxa260SspRxFifoEntrys() > 0)
      pxa260SspRxReadPosition = (pxa260SspRxReadPosition + 1) % 17;
   pxa260SspRxOverflowed = false;
   return pxa260SspRxFifo[pxa260SspRxReadPosition];
}

static void pxa260SspRxFifoWrite(uint16_t value){
   if(pxa260SspRxFifoEntrys() < 16){
      pxa260SspRxWritePosition = (pxa260SspRxWritePosition + 1) % 17;
   }
   else{
      pxa260SspRxOverflowed = true;
      debugLog("pxa260Ssp RX FIFO overflowed\n");
   }
   pxa260SspRxFifo[pxa260SspRxWritePosition] = value;
}

static void pxa260SspRxFifoFlush(void){
   pxa260SspRxReadPosition = pxa260SspRxWritePosition;
}

static uint8_t pxa260SspTxFifoEntrys(void){
   //check for wraparound
   if(pxa260SspTxWritePosition < pxa260SspTxReadPosition)
      return pxa260SspTxWritePosition + 17 - pxa260SspTxReadPosition;
   return pxa260SspTxWritePosition - pxa260SspTxReadPosition;
}

static uint16_t pxa260SspTxFifoRead(void){
   if(pxa260SspTxFifoEntrys() > 0)
      pxa260SspTxReadPosition = (pxa260SspTxReadPosition + 1) % 17;
   return pxa260SspTxFifo[pxa260SspTxReadPosition];
}

static void pxa260SspTxFifoWrite(uint16_t value){
   if(pxa260SspTxFifoEntrys() < 16){
      pxa260SspTxWritePosition = (pxa260SspTxWritePosition + 1) % 17;
      pxa260SspTxFifo[pxa260SspTxWritePosition] = value;
   }
}

static void pxa260SspTxFifoFlush(void){
   pxa260SspTxReadPosition = pxa260SspTxWritePosition;
}

static void pxa260SspUpdateInterrupt(void){
   //SYNCHRONOUS SERIAL PORT ENABLE
   if(pxa260SspSscr0 & 0x0080){
      //RECEIVE FIFO INTERRUPT
      if(pxa260SspSscr1 & 0x0001){
         uint8_t rft = ((pxa260SspSscr1 >> 10) & 0x000F) + 1;

         if(pxa260SspRxFifoEntrys() >= rft)
            goto trigger;
      }

      //TRANSMIT FIFO INTERRUPT
      if(pxa260SspSscr1 & 0x0002){
         uint8_t tft = ((pxa260SspSscr1 >> 6) & 0x000F) + 1;

         if(pxa260SspTxFifoEntrys() <= tft)
            goto trigger;
      }
   }

   pxa260icInt(&pxa260Ic, PXA260_I_SSP, false);
   return;

   trigger:
   pxa260icInt(&pxa260Ic, PXA260_I_SSP, true);
   return;
}

void pxa260SspReset(void){
   pxa260SspSscr0 = 0x0000;
   pxa260SspSscr1 = 0x0000;
   memset(pxa260SspRxFifo, 0x00, sizeof(pxa260SspRxFifo));
   memset(pxa260SspTxFifo, 0x00, sizeof(pxa260SspTxFifo));
   pxa260SspRxReadPosition = 0;
   pxa260SspRxWritePosition = 0;
   pxa260SspRxOverflowed = false;
   pxa260SspTxReadPosition = 0;
   pxa260SspTxWritePosition = 0;
   pxa260SspTransfering = false;
}

uint32_t pxa260SspReadWord(uint32_t address){
   address &= 0xFFFF;

   switch(address){
      case SSCR0:
         return pxa260SspSscr0;

      case SSCR1:
         return pxa260SspSscr1;

      case SSSR:{
            uint8_t rft = ((pxa260SspSscr1 >> 10) & 0x000F) + 1;
            uint8_t tft = ((pxa260SspSscr1 >> 6) & 0x000F) + 1;
            uint8_t rfl = pxa260SspRxFifoEntrys();
            uint8_t tfl = pxa260SspTxFifoEntrys();
            uint16_t value = 0x0000;

            if(rfl == 0)
               rfl = 0xF;
            else
               rfl--;

            if(tfl == 16)
               tfl = 0x0;

            value |= rfl << 12;
            value |= tfl << 8;
            value |= pxa260SspRxOverflowed << 7;
            value |= (pxa260SspRxFifoEntrys() >= rft) << 6;
            value |= (pxa260SspTxFifoEntrys() <= tft) << 5;
            value |= pxa260SspTransfering << 4;
            value |= (pxa260SspRxFifoEntrys() > 0) << 3;
            value |= (pxa260SspTxFifoEntrys() < 16) << 2;

            return value;
         }

      case SSDR:{
            uint16_t fifoVal = pxa260SspRxFifoRead();

            pxa260SspUpdateInterrupt();
            return fifoVal;
         }

      default:
         debugLog("Unimplimented 32 bit PXA260 SSP register read:0x%04X\n", address);
         return 0x00000000;
   }
}

void pxa260SspWriteWord(uint32_t address, uint32_t value){
   address &= 0xFFFF;

   switch(address){
      case SSCR0:
         pxa260SspSscr0 = value & 0xFFFF;

         if(!(value & 0x0080)){
            //disable SSP
            pxa260SspRxFifoFlush();
            pxa260SspTxFifoFlush();
            pxa260SspTransfering = false;
         }

         if((value & 0x0080) != (pxa260SspSscr0 & 0x0080))
            pxa260SspUpdateInterrupt();
         return;

      case SSCR1:
         pxa260SspSscr1 = value & 0x3FFF;
         pxa260SspUpdateInterrupt();
         return;

      case SSSR:
         //clear RECEIVE FIFO OVERRUN
         if(value & 0x0080)
            pxa260SspRxOverflowed = false;

         pxa260SspUpdateInterrupt();
         return;

      case SSDR:
         debugLog("PXA260 SSP write:0x%08X\n", value);
         pxa260SspTxFifoWrite(value);
         if(!pxa260SspTransfering){
            pxa260SspTransfering = true;
            pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_SSP_TRANSFER_COMPLETE, PXA260_SSP_TRANSFER_DURATION);
         }
         pxa260SspUpdateInterrupt();
         return;

      default:
         debugLog("Unimplimented 32 bit PXA260 SSP register write:0x%04X, value:0x%08X\n", address, value);
         return;
   }
}

void pxa260SspTransferComplete(void){
   //check if still transfering, if SSP is disabled mid transfer FIFOs get cleared and weird behavior will occur here
   if(pxa260SspTransfering){
      uint8_t index;
      uint8_t bitCount = (pxa260SspSscr0 & 0x000F);
      uint16_t output = pxa260SspTxFifoRead();
      uint16_t input = 0x0000;

      /*
      NOTE: The serial clock (SSPSCLK), if driven by the SSP port, toggles only while an active data transfer is
      underway, unless receive-without-transmit mode is enabled by setting SSCR1[RWOT] and the
      frame format is not Microwire*, in which case the SSPSCLK toggles regardless of whether
      transmit data exist within the transmit FIFO. At other times, SSPSCLK holds in an inactive or idle
      state as defined by the protocol.
      */

      for(index = 0; index <= bitCount; index++){
         input <<= 1;
         input |= tsc2101ExchangeBit(output >> bitCount - index & 0x0001);
      }

      pxa260SspRxFifoWrite(input);

      //if still transmitting, need to enqueue next event
      if(pxa260SspTxFifoEntrys() > 0)
         pxa260TimingTriggerEvent(PXA260_TIMING_CALLBACK_SSP_TRANSFER_COMPLETE, PXA260_SSP_TRANSFER_DURATION);
      else
         pxa260SspTransfering = false;

      pxa260SspUpdateInterrupt();
   }
}
