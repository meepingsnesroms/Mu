#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "specs/sdCardCommandSpec.h"


enum{
   SD_CARD_EXCHANGE_NOTHING = 0,
   SD_CARD_EXCHANGE_WAIT_RETURN_0,
   SD_CARD_EXCHANGE_WAIT_RETURN_1,
   SD_CARD_EXCHANGE_R1_RESPONCE,
   SD_CARD_EXCHANGE_R3_RESPONCE,
   SD_CARD_EXCHANGE_DATA_PACKET,
   SD_CARD_EXCHANGE_READ_DATA_AT_INDEX,
   SD_CARD_EXCHANGE_WRITE_DATA_AT_INDEX
};


static const uint64_t sdCardCsd[2] = {0x0000000000000000, 0x0000000000000000};
static const uint64_t sdCardCid[2] = {0x0000000000000000, 0x0000000000000000};
static const uint32_t sdCardOcr = 0x00000000;


static void sdCardCmdInvalidFormat(void){
   debugLog("SD command invalid:0x%012X, bits remaining:%d\n", palmSdCard.command, palmSdCard.commandBitsRemaining);
   palmSdCard.command = 0x0000000000000000;
   palmSdCard.commandBitsRemaining = 48;
}

static bool sdCardCmdIsCrcValid(uint64_t command){
#if !defined(EMU_NO_SAFETY)
   uint64_t data = palmSdCard.command >> 8 & 0x3FFFFFFFFF;
   uint8_t crc = palmSdCard.command >> 1 & 0x7F;

   //add real check

   return true;
#else
   return true;
#endif
}

void sdCardReset(void){
   palmSdCard.command = 0x0000000000000000;
   palmSdCard.commandBitsRemaining = 48;
   palmSdCard.index = 0;
   palmSdCard.currentExchange = SD_CARD_EXCHANGE_NOTHING;
}

bool sdCardExchangeBit(bool bit){
   bool sdCardOutputValue = true;//default output value is true, if an action is ongoing it will be set to the data provide by that action

   //make sure SD is actually plugged in
   if(palmSdCard.flashChip.data){
#if !defined(EMU_NO_SAFETY)
      //check validity of incoming bit
      switch(palmSdCard.commandBitsRemaining - 1){
         case 47:
            if(bit)
               sdCardCmdInvalidFormat();
            return true;

         case 46:
         case 0:
            if(!bit)
               sdCardCmdInvalidFormat();
            return true;
      }
#endif

      //add the bit
      palmSdCard.command <<= 1;
      palmSdCard.command |= bit;
      palmSdCard.commandBitsRemaining--;

      //process command if all bits are present
      if(palmSdCard.commandBitsRemaining == 0){
         //check command validity
         if(sdCardCmdIsCrcValid(palmSdCard.command)){
            uint8_t command = palmSdCard.command >> 40 & 0x3F;
            uint32_t argument = palmSdCard.command >> 8 & 0xFFFFFFFF;
            uint8_t crc = palmSdCard.command >> 1 & 0x7F;

            switch(command){
               case GO_IDLE_STATE:
                  sdCardReset();
                  break;

               default:
                  debugLog("SD command:cmd:0x%02X, arg:0x%08X, CRC:0x%02X\n", command, argument, crc);
                  break;
            }

            palmSdCard.commandBitsRemaining = 48;
         }
         else{
            sdCardCmdInvalidFormat();
         }
      }

      //process current exchange
      switch(palmSdCard.currentExchange){
         case SD_CARD_EXCHANGE_NOTHING:
            break;

         default:
            debugLog("SD exchange:%d\n", palmSdCard.currentExchange);
            break;
      }
   }

   return sdCardOutputValue;
}
