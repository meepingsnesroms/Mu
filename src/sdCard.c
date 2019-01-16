#include <stdint.h>
#include <stdbool.h>

#include "emulator.h"
#include "specs/sdCardCommandSpec.h"


enum{
   SD_CARD_RESPONSE_NOTHING = 0,
   SD_CARD_RESPONSE_SHIFT_OUT,
   SD_CARD_RESPONSE_READ_BLOCK
};


static const uint8_t sdCardCrc7Table[256] = {
   0x00,0x09,0x12,0x1B,0x24,0x2D,0x36,0x3F,0x48,0x41,0x5A,0x53,0x6C,0x65,0x7E,
   0x77,0x19,0x10,0x0B,0x02,0x3D,0x34,0x2F,0x26,0x51,0x58,0x43,0x4A,0x75,0x7C,
   0x67,0x6E,0x32,0x3B,0x20,0x29,0x16,0x1F,0x04,0x0D,0x7A,0x73,0x68,0x61,0x5E,
   0x57,0x4C,0x45,0x2B,0x22,0x39,0x30,0x0F,0x06,0x1D,0x14,0x63,0x6A,0x71,0x78,
   0x47,0x4E,0x55,0x5C,0x64,0x6D,0x76,0x7F,0x40,0x49,0x52,0x5B,0x2C,0x25,0x3E,
   0x37,0x08,0x01,0x1A,0x13,0x7D,0x74,0x6F,0x66,0x59,0x50,0x4B,0x42,0x35,0x3C,
   0x27,0x2E,0x11,0x18,0x03,0x0A,0x56,0x5F,0x44,0x4D,0x72,0x7B,0x60,0x69,0x1E,
   0x17,0x0C,0x05,0x3A,0x33,0x28,0x21,0x4F,0x46,0x5D,0x54,0x6B,0x62,0x79,0x70,
   0x07,0x0E,0x15,0x1C,0x23,0x2A,0x31,0x38,0x41,0x48,0x53,0x5A,0x65,0x6C,0x77,
   0x7E,0x09,0x00,0x1B,0x12,0x2D,0x24,0x3F,0x36,0x58,0x51,0x4A,0x43,0x7C,0x75,
   0x6E,0x67,0x10,0x19,0x02,0x0B,0x34,0x3D,0x26,0x2F,0x73,0x7A,0x61,0x68,0x57,
   0x5E,0x45,0x4C,0x3B,0x32,0x29,0x20,0x1F,0x16,0x0D,0x04,0x6A,0x63,0x78,0x71,
   0x4E,0x47,0x5C,0x55,0x22,0x2B,0x30,0x39,0x06,0x0F,0x14,0x1D,0x25,0x2C,0x37,
   0x3E,0x01,0x08,0x13,0x1A,0x6D,0x64,0x7F,0x76,0x49,0x40,0x5B,0x52,0x3C,0x35,
   0x2E,0x27,0x18,0x11,0x0A,0x03,0x74,0x7D,0x66,0x6F,0x50,0x59,0x42,0x4B,0x17,
   0x1E,0x05,0x0C,0x33,0x3A,0x21,0x28,0x5F,0x56,0x4D,0x44,0x7B,0x72,0x69,0x60,
   0x0E,0x07,0x1C,0x15,0x2A,0x23,0x38,0x31,0x46,0x4F,0x54,0x5D,0x62,0x6B,0x70,
   0x79
};
static const uint64_t sdCardCsd[2] = {0x0000000000000000, 0x0000000000000000};
static const uint64_t sdCardCid[2] = {0x0000000000000000, 0x0000000000000000};
static const uint32_t sdCardOcr = 0x00000000;


static void sdCardCmdStart(void){
   palmSdCard.command = UINT64_C(0x0000000000000000);
   palmSdCard.commandBitsRemaining = 48;
}

static bool sdCardCmdIsCrcValid(uint8_t command, uint32_t argument, uint8_t crc){
#if !defined(EMU_NO_SAFETY)
   uint8_t commandCrc = 0;

   command |= 0x40;//add the 01 command starting sequence, not actually part of the command but its counted in the checksum

   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ command];
   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ (argument >> 24 & 0xFF)];
   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ (argument >> 16 & 0xFF)];
   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ (argument >> 8 & 0xFF)];
   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ (argument & 0xFF)];

   if(commandCrc == crc)
      return true;

   return false;
#else
   return true;
#endif
}

static void sdCardDoResponseR1(uint8_t r1){
   palmSdCard.response = SD_CARD_RESPONSE_SHIFT_OUT;
   palmSdCard.responseState = (uint64_t)r1 << 56;
   palmSdCard.responseState |= UINT64_C(1) << 55;//add shift termination bit
}

static void sdCardDoResponseR3(uint8_t r1){
   palmSdCard.response = SD_CARD_RESPONSE_SHIFT_OUT;
   palmSdCard.responseState = (uint64_t)r1 << 56;
   palmSdCard.responseState |= (uint64_t)sdCardOcr << 24;
   palmSdCard.responseState |= UINT64_C(1) << 23;//add shift termination bit
}

void sdCardReset(void){
   palmSdCard.command = UINT64_C(0x0000000000000000);
   palmSdCard.commandBitsRemaining = 48;
   palmSdCard.responseState = 0;
   palmSdCard.response = SD_CARD_RESPONSE_NOTHING;
   palmSdCard.allowInvalidCrc = false;
   palmSdCard.chipSelect = false;
}

void sdCardSetChipSelect(bool value){
   if(value != palmSdCard.chipSelect){
      //may need to perform other actions on chip select toggle too
      debugLog("SD card chip select set to:%s\n", value ? "true" : "false");

      palmSdCard.chipSelect = value;
   }
}

bool sdCardExchangeBit(bool bit){
   bool sdCardOutputValue = true;//default output value is true, if an action is ongoing it will be set to the data provided by that action

   //make sure SD is actually plugged in and selected
   if(palmSdCard.flashChip.data && !palmSdCard.chipSelect){
      bool bitValid = true;

#if defined(EMU_DEBUG)
      //since logs go to the debug window I can use the console to dump the raw SPI bitstream
      //printf("%d", bit);
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
         //printf("V");
#endif
      }
      else{
         sdCardCmdStart();
      }

      //process current exchange, needs to happen before command is proccessed because command response starts the next bit after the command is processed
      switch(palmSdCard.response){
         case SD_CARD_RESPONSE_NOTHING:
            break;

         case SD_CARD_RESPONSE_SHIFT_OUT:
            //debugLog("SD shift out state:0x%016lX\n", palmSdCard.responseState);
            sdCardOutputValue = !!(palmSdCard.responseState & UINT64_C(0x8000000000000000));
            palmSdCard.responseState <<= 1;
            if(palmSdCard.responseState == UINT64_C(0x8000000000000000))
               palmSdCard.response = SD_CARD_RESPONSE_NOTHING;
            break;

         default:
            debugLog("SD exchange:%d\n", palmSdCard.response);
            break;
      }

      //process command if all bits are present
      if(palmSdCard.commandBitsRemaining == 0){
         uint8_t command = palmSdCard.command >> 40 & 0x3F;
         uint32_t argument = palmSdCard.command >> 8 & 0xFFFFFFFF;
         uint8_t crc = palmSdCard.command >> 1 & 0x7F;

#if defined(EMU_DEBUG)
         //acknowledge the end of a command
         printf("CMD");
#endif
         debugLog("SD command:cmd:0x%02X, arg:0x%08X, CRC:0x%02X\n", command, argument, crc);

         if(palmSdCard.allowInvalidCrc || sdCardCmdIsCrcValid(command, argument, crc)){
            //respond with command value
            debugLog("SD valid CRC\n");

            switch(command){
               case GO_IDLE_STATE:
                  palmSdCard.allowInvalidCrc = true;
                  sdCardDoResponseR1(0x01);//"idle state" bit set should be set
                  break;

               default:
                  debugLog("SD unknown command:cmd:0x%02X, arg:0x%08X, CRC:0x%02X\n", command, argument, crc);
                  break;
            }
         }
         else{
            //send back R1 response with CRC error set
            debugLog("SD invalid CRC\n");
            sdCardDoResponseR1(0x08);//"command CRC error" bit set
         }

         //start next command
         sdCardCmdStart();
      }
   }

#if defined(EMU_DEBUG)
   //print response
   printf("%d", sdCardOutputValue);
#endif

   return sdCardOutputValue;
}
