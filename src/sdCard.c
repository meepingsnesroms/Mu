#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "emulator.h"
#include "specs/sdCardCommandSpec.h"


#include "sdCardCrcTables.c.h"


static void sdCardCmdStart(void){
   palmSdCard.command = UINT64_C(0x0000000000000000);
   palmSdCard.commandBitsRemaining = 48;
   palmSdCard.receivingCommand = true;
}

static bool sdCardCmdIsCrc7Valid(uint8_t command, uint32_t argument, uint8_t crc){
#if !defined(EMU_NO_SAFETY)
   uint8_t commandCrc = 0x00;

   //add the 01 command starting sequence, not actually part of the command but its counted in the checksum
   command |= 0x40;

   commandCrc = sdCardCrc7Table[commandCrc << 1 ^ command];
   commandCrc = sdCardCrc7Table[commandCrc << 1 ^ (argument >> 24 & 0xFF)];
   commandCrc = sdCardCrc7Table[commandCrc << 1 ^ (argument >> 16 & 0xFF)];
   commandCrc = sdCardCrc7Table[commandCrc << 1 ^ (argument >> 8 & 0xFF)];
   commandCrc = sdCardCrc7Table[commandCrc << 1 ^ (argument & 0xFF)];

   if(commandCrc != crc)
      return false;
#endif

   return true;
}

static uint8_t sdCardCrc7(uint8_t* data, uint16_t size){
   uint8_t dataCrc = 0x00;
   uint16_t offset;

   //csum = crc7_table[(csum << 1) ^ input];
   for(offset = 0; offset < size; offset++)
      dataCrc = sdCardCrc7Table[dataCrc << 1 ^ data[offset]];

   return dataCrc;
}

static uint16_t sdCardCrc16(uint8_t* data, uint16_t size){
   uint16_t dataCrc = 0x0000;
   uint16_t offset;

   //csum = crc16_table[((csum >> 8) ^ *data) & 0xff] ^ (csum << 8);
   for(offset = 0; offset < size; offset++)
      dataCrc = sdCardCrc16Table[(dataCrc >> 8 ^ data[offset]) & 0xFF] ^ dataCrc << 8;

   return dataCrc;
}

#include "sdCardAccessors.c.h"

void sdCardReset(void){
   if(palmSdCard.flashChip.data){
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
      palmSdCard.receivingCommand = false;
      palmSdCard.inIdleState = true;
      //palmSdCard.chipSelect is a property of the wire to the SD card not the SD card itself
      //palmSdCard.writeProtectSwitch is not written on reset because it is a physical property not an electronic one
   }
}

void sdCardSetChipSelect(bool value){
   if(value != palmSdCard.chipSelect){
      if(palmSdCard.flashChip.data){
         //commands start when chip select goes from high to low
         if(value == false)
            sdCardCmdStart();
      }

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
            if(palmSdCard.runningCommandVars[0] < palmSdCard.flashChip.size){
               sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, palmSdCard.flashChip.data + palmSdCard.runningCommandVars[0], SD_CARD_BLOCK_SIZE);
               palmSdCard.runningCommandVars[0] += SD_CARD_BLOCK_SIZE;
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

            //debugLog("SD command: isAcmd:%d, cmd:%d, arg:0x%08X, CRC:0x%02X\n", palmSdCard.commandIsAcmd, command, argument, crc);

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
               if(palmSdCard.allowInvalidCrc || sdCardCmdIsCrc7Valid(command, argument, crc)){
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

                        case SEND_CSD:{
                              uint8_t csd[16];

                              sdCardGetCsd(csd);
                              sdCardDoResponseR1(palmSdCard.inIdleState);
                              sdCardDoResponseDelay(1);
                              sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, csd, 16);
                           }
                           break;

                        case SEND_CID:{
                              uint8_t cid[16];

                              sdCardGetCid(cid);
                              if(!palmSdCard.allowInvalidCrc)
                                 cid[15] = sdCardCrc7(cid, 15);
                              sdCardDoResponseR1(palmSdCard.inIdleState);
                              sdCardDoResponseDelay(1);
                              sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, cid, 16);
                           }
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
                              sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, palmSdCard.flashChip.data + argument, SD_CARD_BLOCK_SIZE);
                           else
                              sdCardDoResponseErrorToken(ET_OUT_OF_RANGE);
                           break;

                        case READ_MULTIPLE_BLOCK:
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           sdCardDoResponseDelay(1);
                           if(argument < palmSdCard.flashChip.size){
                              palmSdCard.runningCommand = READ_MULTIPLE_BLOCK;
                              palmSdCard.runningCommandVars[0] = argument;
                              sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, palmSdCard.flashChip.data + palmSdCard.runningCommandVars[0], SD_CARD_BLOCK_SIZE);
                              palmSdCard.runningCommandVars[0] += SD_CARD_BLOCK_SIZE;
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
                              palmSdCard.runningCommandVars[0] = argument;
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

                        case SEND_SCR:{
                              uint8_t scr[8];

                              sdCardGetScr(scr);
                              sdCardDoResponseR1(palmSdCard.inIdleState);
                              sdCardDoResponseDelay(1);
                              sdCardDoResponseDataPacket(DATA_TOKEN_DEFAULT, scr, 8);
                           }
                           break;

                        case SET_WR_BLOCK_ERASE_COUNT:
                           sdCardDoResponseR1(palmSdCard.inIdleState);
                           //HACK, this command isnt actually supported yet, called when formmating the SD card
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
                  if(palmSdCard.allowInvalidCrc || sdCardCrc16(palmSdCard.runningCommandPacket + 1, SD_CARD_BLOCK_SIZE) == (palmSdCard.runningCommandPacket[SD_CARD_BLOCK_DATA_PACKET_SIZE - 2] << 8 | palmSdCard.runningCommandPacket[SD_CARD_BLOCK_DATA_PACKET_SIZE - 1])){
                     //HACK, also need to check if block is write protected, not just the card as a whole
                     if(palmSdCard.runningCommandVars[0] < palmSdCard.flashChip.size && !palmSdCard.writeProtectSwitch){
                        memcpy(palmSdCard.flashChip.data + palmSdCard.runningCommandVars[0], palmSdCard.runningCommandPacket + 1, SD_CARD_BLOCK_SIZE);
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
                     palmSdCard.runningCommandVars[0] += SD_CARD_BLOCK_SIZE;
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
