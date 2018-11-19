#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"


//command format:01IIIIIIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACCCCCCC1
//I = index, A = argument, C = CRC


enum{
   SD_CARD_ACTION_NOTHING = 0,
   SD_CARD_ACTION_READ_DATA_BLOCK,
   SD_CARD_ACTION_READ_DATA_STREAM,
   SD_CARD_ACTION_WRITE_DATA_BLOCK,
   SD_CARD_ACTION_WRITE_DATA_STREAM,
   SD_CARD_ACTION_GET_ID
};


static void sdCardInvalidFormat(void){
   palmSdCard.command = 0x0000000000000000;
   palmSdCard.commandBitsRemaining = 48;
}

void sdCardReset(void){
   palmSdCard.command = 0x0000000000000000;
   palmSdCard.commandBitsRemaining = 48;
   palmSdCard.index = 0;
   palmSdCard.currentAction = SD_CARD_ACTION_NOTHING;
}

bool sdCardExchangeBit(bool bit){
   bool sdCardOutputValue = true;//default output value is true, if an action is ongoing it will be set to the data provide by that action

   //check validity of incoming bit
   switch(palmSdCard.commandBitsRemaining - 1){
      case 47:
         if(bit)
            sdCardInvalidFormat();
         return true;

      case 46:
      case 0:
         if(!bit)
            sdCardInvalidFormat();
         return true;
   }

   //add the bit
   palmSdCard.command <<= 1;
   palmSdCard.command |= bit;
   palmSdCard.commandBitsRemaining--;

   //process command if all bits are present
   if(palmSdCard.commandBitsRemaining == 0){
      uint8_t command = palmSdCard.command >> 40 & 0x3F;
      uint32_t argument = palmSdCard.command >> 8 & 0xFFFFFFFF;
      uint8_t crc = palmSdCard.command >> 1 & 0x7F;

      switch(command){

         default:
            debugLog("SD command:cmd:0x%02X, arg:0x%08X, CRC:0x%02X\n", command, argument, crc);
            break;
      }

      palmSdCard.commandBitsRemaining = 48;
   }

   //do action associated with newest command
   switch(palmSdCard.currentAction){
      case SD_CARD_ACTION_NOTHING:
         break;

      default:
         debugLog("SD action:%d\n", palmSdCard.currentAction);
         break;
   }

   return sdCardOutputValue;
}
