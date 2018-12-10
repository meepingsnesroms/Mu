#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "specs/sdCardCommandSpec.h"


enum{
   SD_CARD_RESPONSE_NOTHING = 0,
   SD_CARD_RESPONSE_SHIFT_OUT,
   SD_CARD_RESPONSE_WAIT_RETURN_0,
   SD_CARD_RESPONSE_WAIT_RETURN_1,
   SD_CARD_RESPONSE_R1_RESPONCE,
   SD_CARD_RESPONSE_R3_RESPONCE,
   SD_CARD_RESPONSE_DATA_PACKET,
   SD_CARD_RESPONSE_READ_DATA_AT_INDEX
};


static const uint64_t sdCardCsd[2] = {0x0000000000000000, 0x0000000000000000};
static const uint64_t sdCardCid[2] = {0x0000000000000000, 0x0000000000000000};
static const uint32_t sdCardOcr = 0x00000000;


static void sdCardCmdStart(void){
   palmSdCard.command = 0x0000000000000000;
   palmSdCard.commandBitsRemaining = 48;
}

static bool sdCardCmdIsCrcValid(uint64_t command){
#if !defined(EMU_NO_SAFETY)
   uint64_t data = palmSdCard.command >> 8 & 0x3FFFFFFFFF;
   uint8_t crc = palmSdCard.command >> 1 & 0x7F;

   //add real check
   if(true)
      return true;

   return false;
#else
   return true;
#endif
}

static void sdCardDoResponseR1(bool addressError, bool eraseError, bool badCrc, bool illegalCommand, bool idle){

}

void sdCardReset(void){
   palmSdCard.command = 0x0000000000000000;
   palmSdCard.commandBitsRemaining = 48;
   palmSdCard.responseState = 0;
   palmSdCard.response = SD_CARD_RESPONSE_NOTHING;
}

bool sdCardExchangeBit(bool bit){
   bool sdCardOutputValue = true;//default output value is true, if an action is ongoing it will be set to the data provide by that action

   //make sure SD is actually plugged in
   if(palmSdCard.flashChip.data){
      bool bitValid = true;

#if defined(EMU_DEBUG)
      //since logs go to the debug window I can use the console to dump the raw SPI bitstream
      printf("%d", bit);
#endif

      //check validity of incoming bit, needed even when safety checks are disabled to determine command start
      switch(palmSdCard.commandBitsRemaining - 1){
         case 47:
            if(bit)
               bitValid = false;
            break;


         case 46:
         case 0:
            if(!bit)
               bitValid = false;
            break;
      }

      //add the bit or start new command if invalid
      if(bitValid){
         palmSdCard.command <<= 1;
         palmSdCard.command |= bit;
         palmSdCard.commandBitsRemaining--;

#if defined(EMU_DEBUG)
         //mark this bit as valid
         printf("V");
#endif
      }
      else{
         sdCardCmdStart();
      }

      //process command if all bits are present
      if(palmSdCard.commandBitsRemaining == 0){
         //check command validity
         if(sdCardCmdIsCrcValid(palmSdCard.command)){
            uint8_t command = palmSdCard.command >> 40 & 0x3F;
            uint32_t argument = palmSdCard.command >> 8 & 0xFFFFFFFF;
            uint8_t crc = palmSdCard.command >> 1 & 0x7F;

#if defined(EMU_DEBUG)
            //acknowledge the end of a command
            printf("CMD");
#endif

            debugLog("SD command:cmd:0x%02X, arg:0x%08X, CRC:0x%02X\n", command, argument, crc);

            switch(command){
               case GO_IDLE_STATE:
                  sdCardReset();
                  palmSdCard.response = SD_CARD_RESPONSE_R1_RESPONCE;
                  break;

               default:
                  debugLog("SD unknown command:cmd:0x%02X, arg:0x%08X, CRC:0x%02X\n", command, argument, crc);
                  break;
            }
         }

         //start next command
         sdCardCmdStart();
      }

      //process current exchange
      switch(palmSdCard.response){
         case SD_CARD_RESPONSE_NOTHING:
            break;

         case SD_CARD_RESPONSE_SHIFT_OUT:
            sdCardOutputValue = !!(palmSdCard.responseState & 0x8000000000000000);
            palmSdCard.responseState <<= 1;
            if(palmSdCard.responseState == 0x8000000000000000)
               palmSdCard.response = SD_CARD_RESPONSE_NOTHING;
            break;

         default:
            debugLog("SD exchange:%d\n", palmSdCard.response);
            break;
      }
   }

   return sdCardOutputValue;
}
