#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "specs/sdCardCommandSpec.h"


static const uint8_t sdCardCrc7Table[256] = {
   0x00,0x09,0x12,0x1B,0x24,0x2D,0x36,0x3F,0x48,0x41,0x5A,0x53,0x6C,0x65,0x7E,0x77,
   0x19,0x10,0x0B,0x02,0x3D,0x34,0x2F,0x26,0x51,0x58,0x43,0x4A,0x75,0x7C,0x67,0x6E,
   0x32,0x3B,0x20,0x29,0x16,0x1F,0x04,0x0D,0x7A,0x73,0x68,0x61,0x5E,0x57,0x4C,0x45,
   0x2B,0x22,0x39,0x30,0x0F,0x06,0x1D,0x14,0x63,0x6A,0x71,0x78,0x47,0x4E,0x55,0x5C,
   0x64,0x6D,0x76,0x7F,0x40,0x49,0x52,0x5B,0x2C,0x25,0x3E,0x37,0x08,0x01,0x1A,0x13,
   0x7D,0x74,0x6F,0x66,0x59,0x50,0x4B,0x42,0x35,0x3C,0x27,0x2E,0x11,0x18,0x03,0x0A,
   0x56,0x5F,0x44,0x4D,0x72,0x7B,0x60,0x69,0x1E,0x17,0x0C,0x05,0x3A,0x33,0x28,0x21,
   0x4F,0x46,0x5D,0x54,0x6B,0x62,0x79,0x70,0x07,0x0E,0x15,0x1C,0x23,0x2A,0x31,0x38,
   0x41,0x48,0x53,0x5A,0x65,0x6C,0x77,0x7E,0x09,0x00,0x1B,0x12,0x2D,0x24,0x3F,0x36,
   0x58,0x51,0x4A,0x43,0x7C,0x75,0x6E,0x67,0x10,0x19,0x02,0x0B,0x34,0x3D,0x26,0x2F,
   0x73,0x7A,0x61,0x68,0x57,0x5E,0x45,0x4C,0x3B,0x32,0x29,0x20,0x1F,0x16,0x0D,0x04,
   0x6A,0x63,0x78,0x71,0x4E,0x47,0x5C,0x55,0x22,0x2B,0x30,0x39,0x06,0x0F,0x14,0x1D,
   0x25,0x2C,0x37,0x3E,0x01,0x08,0x13,0x1A,0x6D,0x64,0x7F,0x76,0x49,0x40,0x5B,0x52,
   0x3C,0x35,0x2E,0x27,0x18,0x11,0x0A,0x03,0x74,0x7D,0x66,0x6F,0x50,0x59,0x42,0x4B,
   0x17,0x1E,0x05,0x0C,0x33,0x3A,0x21,0x28,0x5F,0x56,0x4D,0x44,0x7B,0x72,0x69,0x60,
   0x0E,0x07,0x1C,0x15,0x2A,0x23,0x38,0x31,0x46,0x4F,0x54,0x5D,0x62,0x6B,0x70,0x79
};

//register data is from a "ULTRA SD HI-SPEED 1GB", thats actually the name of the card these IDs are from
static const uint8_t sdCardCsd[16] = {0x00, 0x2F, 0x00, 0x32, 0x5F, 0x59, 0x83, 0xB8, 0x6D, 0xB7, 0xFF, 0x9F, 0x96, 0x40, 0x00, 0x00};//CRC7 is invalid here
static const uint8_t sdCardCid[16] = {0x1D, 0x41, 0x44, 0x53, 0x44, 0x20, 0x20, 0x20, 0x10, 0xA0, 0x50, 0x33, 0xA4, 0x00, 0x81, 0x00};//CRC7 is invalid here
static const uint8_t sdCardScr[8] = {0x01, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};//dont know it this needs a CRC7 or not?
//static const uint8_t sdCardDsr[2] = {0x04, 0x04};
//static const uint8_t sdCardRca[2] = {0x??, 0x??};//not available in SPI mode

static uint32_t sdCardGetOcr(void){
   return !palmSdCard.inIdleState << 31/*power up status*/ | 0 << 30/*card capacity status*/ | 0x01FF8000/*supported voltages*/;
}

static void sdCardCmdStart(void){
   palmSdCard.command = UINT64_C(0x0000000000000000);
   palmSdCard.commandBitsRemaining = 48;
   palmSdCard.receivingCommand = true;
}

static bool sdCardCmdIsCrcValid(uint8_t command, uint32_t argument, uint8_t crc){
#if !defined(EMU_NO_SAFETY)
   uint8_t commandCrc = 0x00;

   //add the 01 command starting sequence, not actually part of the command but its counted in the checksum
   command |= 0x40;

   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ command];
   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ (argument >> 24 & 0xFF)];
   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ (argument >> 16 & 0xFF)];
   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ (argument >> 8 & 0xFF)];
   commandCrc = sdCardCrc7Table[(commandCrc << 1) ^ (argument & 0xFF)];

   if(commandCrc != crc)
      return false;
#endif

   return true;
}

static bool sdCardVerifyCrc16(uint8_t* data, uint16_t size, uint16_t crc){
#if !defined(EMU_NO_SAFETY)
   uint16_t dataCrc = 0x0000;

   //HACK, need to actually check this

   if(dataCrc != crc)
      return false;
#endif

   return true;
}

#include "sdCardAccessors.c.h"

void sdCardReset(void){
   palmSdCard.command = UINT64_C(0x0000000000000000);
   palmSdCard.commandBitsRemaining = 48;
   palmSdCard.runningCommand = 0x00;
   memset(palmSdCard.runningCommandVars, 0x00, sizeof(palmSdCard.runningCommandVars));
   memset(palmSdCard.runningCommandPacket, 0x00, SD_CARD_BLOCK_DATA_PACKET_SIZE);
   memset(palmSdCard.responseFifo, 0x00, SD_CARD_RESPONSE_FIFO_SIZE);
   palmSdCard.responseReadPosition = 0;
   palmSdCard.responseWritePosition = 0;
   palmSdCard.responseReadPositionBit = 7;
   palmSdCard.commandIsAcmd = false;
   palmSdCard.allowInvalidCrc = false;
   palmSdCard.chipSelect = false;
   palmSdCard.receivingCommand = false;
   palmSdCard.inIdleState = true;
   //palmSdCard.writeProtectSwitch is not written on reset because it is a physical property not an electronic one
}

void sdCardSetChipSelect(bool value){
   if(value != palmSdCard.chipSelect){
      //debugLog("SD card chip select set to:%s\n", value ? "true" : "false");

      //commands start when chip select goes from high to low
      if(value == false)
         sdCardCmdStart();

      palmSdCard.chipSelect = value;
   }
}

bool sdCardExchangeBit(bool bit){
   bool outputValue = true;//SPI1 pins are on port j which has pull up resistors so default output value is true

   //make sure SD is actually plugged in and chip select is low
   if(palmSdCard.flashChip.data && !palmSdCard.chipSelect){
      //get output value first
      outputValue = sdCardResponseFifoReadBit();

      //if doing a multiblock read add data when running low
      if(palmSdCard.runningCommand == READ_MULTIPLE_BLOCK){
         if(sdCardResponseFifoByteEntrys() < SD_CARD_BLOCK_SIZE){
            sdCardDoResponseDelay(1);
            if(palmSdCard.runningCommandVars[0] * SD_CARD_BLOCK_SIZE < palmSdCard.flashChip.size){
               sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, palmSdCard.flashChip.data + palmSdCard.runningCommandVars[0] * SD_CARD_BLOCK_SIZE, SD_CARD_BLOCK_SIZE);
               palmSdCard.runningCommandVars[0]++;
            }
            else{
               sdCardDoResponseErrorToken(ET_OUT_OF_RANGE);
               palmSdCard.runningCommand = 0x00;
            }
         }
      }

      //route received bit as command or data
      if(palmSdCard.receivingCommand){
         //receiving a command
         bool bitValid = true;

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
         }
         else{
            sdCardCmdStart();
         }

         //process command if all bits are present
         if(palmSdCard.commandBitsRemaining == 0){
            uint8_t command = palmSdCard.command >> 40 & 0x3F;
            uint32_t argument = palmSdCard.command >> 8 & 0xFFFFFFFF;
            uint8_t crc = palmSdCard.command >> 1 & 0x7F;
            bool commandWantsData = false;
            bool doInIdleState = false;

            debugLog("SD command: isAcmd:%d, cmd:%d, arg:0x%08X, CRC:0x%02X\n", palmSdCard.commandIsAcmd, command, argument, crc);

            //in idle state, the card accepts only CMD0, CMD1, ACMD41, CMD58 and CMD59, any other commands will be rejected
            if(palmSdCard.inIdleState){
               if(!palmSdCard.commandIsAcmd){
                  switch(command){
                     case GO_IDLE_STATE:
                     case SEND_OP_COND:
                     case APP_CMD:
                     case READ_OCR:
                     case CRC_ON_OFF:
                        doInIdleState = true;
                        break;

                     default:
                        break;
                  }
               }
               else{
                  switch(command){
                     case APP_SEND_OP_COND:
                        doInIdleState = true;
                        break;

                     default:
                        break;
                  }
               }
            }

            //log blocked commands
            if(palmSdCard.inIdleState && !doInIdleState)
               debugLog("SD command blocked by idle state: isAcmd:%d, cmd:%d, arg:0x%08X, CRC:0x%02X\n", palmSdCard.commandIsAcmd, command, argument, crc);

            if(!palmSdCard.inIdleState || doInIdleState){
               //run command
               if(palmSdCard.allowInvalidCrc || sdCardCmdIsCrcValid(command, argument, crc)){
                  if(!palmSdCard.commandIsAcmd){
                     //normal command
                     switch(command){
                        case GO_IDLE_STATE:
                           palmSdCard.inIdleState = true;
                           palmSdCard.allowInvalidCrc = true;
                           palmSdCard.runningCommand = 0x00;
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           break;

                        case SEND_OP_COND:
                           //after this is run the SD card is initialized
                           palmSdCard.inIdleState = false;
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           break;

                        case READ_OCR:
                           sdCardDoResponseR3R7(palmSdCard.inIdleState, sdCardGetOcr());
                           break;

                        case SEND_CSD:
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           sdCardDoResponseDelay(1);
                           sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, sdCardCsd, sizeof(sdCardCsd));
                           break;

                        case SEND_CID:
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           sdCardDoResponseDelay(1);
                           sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, sdCardCid, sizeof(sdCardCid));
                           break;

                        case SEND_STATUS:
                           //HACK, need to add real write protection, this command is also how the host reads the value of the little switch on the side
                           sdCardDoResponseR2(palmSdCard.inIdleState, palmSdCard.writeProtectSwitch);
                           break;

                        case SEND_WRITE_PROT:{
                              const uint8_t writeProtBits[4] = {0x00, 0x00, 0x00, 0x00};

                              //HACK, need to add real write protection
                              sdCardDoResponseR1(palmSdCard.inIdleState);
                              sdCardDoResponseDelay(1);
                              sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, writeProtBits, sizeof(writeProtBits));
                           }
                           break;

                        case SET_BLOCKLEN:
                           sdCardDoResponseR1((argument != SD_CARD_BLOCK_SIZE ? R1_PARAMETER_ERROR : 0x00) | palmSdCard.inIdleState);
                           break;

                        case APP_CMD:
                           palmSdCard.commandIsAcmd = true;
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           break;

                        case STOP_TRANSMISSION:
                           if(palmSdCard.runningCommand == READ_MULTIPLE_BLOCK){
                              palmSdCard.runningCommand = 0x00;
                              sdCardResponseFifoFlush();
                              sdCardDoResponseDelay(1);
                              sdCardDoResponseR1(palmSdCard.inIdleState);
                              sdCardDoResponseBusy(1);
                           }
                           else{
                              sdCardDoResponseR1(palmSdCard.inIdleState);
                           }
                           break;

                        case READ_SINGLE_BLOCK:
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           sdCardDoResponseDelay(1);
                           if(argument < palmSdCard.flashChip.size)
                              sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, palmSdCard.flashChip.data + argument / SD_CARD_BLOCK_SIZE * SD_CARD_BLOCK_SIZE, SD_CARD_BLOCK_SIZE);
                           else
                              sdCardDoResponseErrorToken(ET_OUT_OF_RANGE);
                           break;

                        case READ_MULTIPLE_BLOCK:
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           sdCardDoResponseDelay(1);
                           if(argument < palmSdCard.flashChip.size){
                              palmSdCard.runningCommand = READ_MULTIPLE_BLOCK;
                              palmSdCard.runningCommandVars[0] = argument / SD_CARD_BLOCK_SIZE;
                              sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, palmSdCard.flashChip.data + palmSdCard.runningCommandVars[0] * SD_CARD_BLOCK_SIZE, SD_CARD_BLOCK_SIZE);
                              palmSdCard.runningCommandVars[0]++;
                           }
                           else{
                              sdCardDoResponseErrorToken(ET_OUT_OF_RANGE);
                           }
                           break;

                        case WRITE_SINGLE_BLOCK:
                        case WRITE_MULTIPLE_BLOCK:
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           if(argument < palmSdCard.flashChip.size){
                              palmSdCard.runningCommand = command;
                              palmSdCard.runningCommandVars[0] = argument / SD_CARD_BLOCK_SIZE;
                              palmSdCard.runningCommandVars[1] = 0x00;//last 8 received bits, used to see if a data token has been received
                              palmSdCard.runningCommandVars[2] = 0;//data packet bit index
                              memset(palmSdCard.runningCommandPacket, 0x00, SD_CARD_BLOCK_DATA_PACKET_SIZE);
                              commandWantsData = true;
                           }
                           else{
                              sdCardDoResponseErrorToken(ET_OUT_OF_RANGE);
                           }
                           break;

                        default:
                           debugLog("SD unknown command: cmd:%d, arg:0x%08X, CRC:0x%02X\n", command, argument, crc);
                           sdCardDoResponseR1(R1_ILLEGAL_COMMAND | palmSdCard.inIdleState);
                           break;
                     }
                  }
                  else{
                     //ACMD command
                     switch(command){
                        case APP_SEND_OP_COND:
                           //after this is run the SD card is initialized
                           palmSdCard.inIdleState = false;
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           break;

                        case SEND_SCR:
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           sdCardDoResponseDelay(1);
                           sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, sdCardScr, sizeof(sdCardScr));
                           break;

                        default:
                           debugLog("SD unknown ACMD command: cmd:%d, arg:0x%08X, CRC:0x%02X\n", command, argument, crc);
                           sdCardDoResponseR1(R1_ILLEGAL_COMMAND | palmSdCard.inIdleState);
                           break;
                     }

                     //ACMD finished, go back to normal command format
                     palmSdCard.commandIsAcmd = false;
                  }
               }
               else{
                  //send back R1 response with CRC error set
                  debugLog("SD invalid CRC\n");
                  sdCardDoResponseR1(R1_COMMAND_CRC_ERROR | palmSdCard.inIdleState);
               }
            }

            //start next command if previous doesnt take any data
            if(commandWantsData)
               palmSdCard.receivingCommand = false;
            else
               sdCardCmdStart();
         }
      }
      else{
         //receiving data
         switch(palmSdCard.runningCommand){
            case WRITE_SINGLE_BLOCK:
            case WRITE_MULTIPLE_BLOCK:
               if(palmSdCard.runningCommandVars[2] >= SD_CARD_BLOCK_DATA_PACKET_SIZE * 8){
                  //packet finished, verify and write block to chip
                  if(palmSdCard.allowInvalidCrc || sdCardVerifyCrc16(palmSdCard.runningCommandPacket + 1, SD_CARD_BLOCK_SIZE, palmSdCard.runningCommandPacket[SD_CARD_BLOCK_DATA_PACKET_SIZE - 2] << 8 | palmSdCard.runningCommandPacket[SD_CARD_BLOCK_DATA_PACKET_SIZE - 1])){
                     //HACK, also need to check if block is write protected, not just the card as a whole
                     if(!palmSdCard.writeProtectSwitch){
                        memcpy(palmSdCard.flashChip.data + palmSdCard.runningCommandVars[0] * SD_CARD_BLOCK_SIZE, palmSdCard.runningCommandPacket + 1, SD_CARD_BLOCK_SIZE);
                        sdCardDoResponseDataResponse(DR_ACCEPTED);
                     }
                     else{
                        sdCardDoResponseDataResponse(DR_WRITE_ERROR);
                     }
                  }
                  else{
                     sdCardDoResponseDataResponse(DR_CRC_ERROR);
                  }

                  //this fixes broken write mode???
                  //this may not be a hack, elm-chan says:
                  //The card responds a Data Response immediataly following the data packet from the host.
                  //The Data Response trails a busy flag and host controller must suspend the next command or data transmission until the card goes ready.
                  //that could mean the response is returned starting on the final bit of the data packet instead of the bit after, violating byte boundrys
                  //the return data is 1 bit misaligned, but I dont know how it gets this way
                  //Currently:
                  //SPI1 transfer, bitCount:8, PC:0x100A7D98(printed 1 times)
                  //SPIRXD read, FIFO value:0x0082, SPIINTCS:0x0001(printed 1 times)
                  //SPI1 transfer, bitCount:8, PC:0x100A5B32(printed 1 times)
                  //SPIRXD read, FIFO value:0x0080, SPIINTCS:0x0001(printed 1 times)
                  //Should be:
                  //SPIRXD read, FIFO value:0x0005, SPIINTCS:0x0001(printed 1 times)
                  //SPI1 transfer, bitCount:8, PC:0x100A5B32(printed 1 times)
                  //SPIRXD read, FIFO value:0x0000, SPIINTCS:0x0001(printed 1 times)
                  outputValue = sdCardResponseFifoReadBit();

                  if(palmSdCard.runningCommand == WRITE_SINGLE_BLOCK){
                     //end transfer
                     palmSdCard.runningCommand = 0x00;
                     sdCardCmdStart();
                  }
                  else{
                     //prepare to write next block
                     palmSdCard.runningCommandVars[0]++;
                     palmSdCard.runningCommandVars[1] = 0x00;//last 8 received bits, used to see if a data token has been received
                     palmSdCard.runningCommandVars[2] = 0;//data packet bit index
                     memset(palmSdCard.runningCommandPacket, 0x00, SD_CARD_BLOCK_DATA_PACKET_SIZE);
                  }
               }
               else if(palmSdCard.runningCommandVars[2] > 0){
                  //add bit to data packet
                  palmSdCard.runningCommandPacket[palmSdCard.runningCommandVars[2] / 8] |= bit << 7 - palmSdCard.runningCommandVars[2] % 8;
                  palmSdCard.runningCommandVars[2]++;
               }
               else{
                  //check if data packet should start
                  palmSdCard.runningCommandVars[1] <<= 1;
                  palmSdCard.runningCommandVars[1] |= bit;
                  palmSdCard.runningCommandVars[1] &= 0xFF;

                  if(palmSdCard.runningCommand == WRITE_SINGLE_BLOCK){
                     //writing 1 block
                     if(palmSdCard.runningCommandVars[1] == DATA_TOKEN_DEFAULT){
                        //accept block
                        palmSdCard.runningCommandPacket[0] = DATA_TOKEN_DEFAULT;
                        palmSdCard.runningCommandVars[2] = 8;
                     }
                  }
                  else{
                     //writing an undefined number of blocks
                     if(palmSdCard.runningCommandVars[1] == DATA_TOKEN_CMD25){
                        //accept block
                        palmSdCard.runningCommandPacket[0] = DATA_TOKEN_CMD25;
                        palmSdCard.runningCommandVars[2] = 8;
                     }
                     else if(palmSdCard.runningCommandVars[1] == STOP_TRAN){
                        //end multiblock transfer
                        sdCardDoResponseDelay(1);
                        sdCardDoResponseBusy(1);
                        palmSdCard.runningCommand = 0x00;
                        sdCardCmdStart();
                     }
                  }
               }
               break;

            default:
               debugLog("SD orphan data bit:%d\n", bit);
               break;
         }
      }
   }

   return outputValue;
}
