//FIFO accessors
static uint16_t sdCardResponseFifoByteEntrys(void){
   //check for wraparound
   if(palmSdCard.responseWritePosition < palmSdCard.responseReadPosition)
      return palmSdCard.responseWritePosition + SD_CARD_RESPONSE_FIFO_SIZE - palmSdCard.responseReadPosition;
   return palmSdCard.responseWritePosition - palmSdCard.responseReadPosition;
}

static bool sdCardResponseFifoReadBit(void){
   if(sdCardResponseFifoByteEntrys() > 0){
      bool bit = !!(palmSdCard.responseFifo[palmSdCard.responseReadPosition] & 1 << palmSdCard.responseReadPositionBit);
      palmSdCard.responseReadPositionBit--;
      if(palmSdCard.responseReadPositionBit < 0){
         palmSdCard.responseReadPositionBit = 7;
         palmSdCard.responseReadPosition = (palmSdCard.responseReadPosition + 1) % SD_CARD_RESPONSE_FIFO_SIZE;
      }
      return bit;
   }
   return true;
}

static uint8_t sdCardResponseFifoReadByteOptimized(void){
   switch(sdCardResponseFifoByteEntrys()){
      case 0:
         //no bytes
         return 0xFF;

      case 1:{
            //unsafe, use slow mode
            uint8_t byte = 0x00;
            uint8_t count;

            for(count = 0; count < 8; count++){
               byte <<= 1;
               byte |= sdCardResponseFifoReadBit();
            }

            return byte;
         }

      default:{
            uint8_t byte;

            if(palmSdCard.responseReadPositionBit == 7){
               //just read the whole byte at once
               byte = palmSdCard.responseFifo[palmSdCard.responseReadPosition];
               palmSdCard.responseReadPosition = (palmSdCard.responseReadPosition + 1) % SD_CARD_RESPONSE_FIFO_SIZE;
               return byte;
            }
            else{
               //have to merge 2 bytes
               byte = palmSdCard.responseFifo[palmSdCard.responseReadPosition] << 7 - palmSdCard.responseReadPositionBit;
               palmSdCard.responseReadPosition = (palmSdCard.responseReadPosition + 1) % SD_CARD_RESPONSE_FIFO_SIZE;
               byte |= palmSdCard.responseFifo[palmSdCard.responseReadPosition] >> palmSdCard.responseReadPositionBit + 1;
               return byte;
            }
         }
   }
}

static void sdCardResponseFifoWriteByte(uint8_t value){
   if(likely(sdCardResponseFifoByteEntrys() < SD_CARD_RESPONSE_FIFO_SIZE - 1)){
      palmSdCard.responseFifo[palmSdCard.responseWritePosition] = value;
      palmSdCard.responseWritePosition = (palmSdCard.responseWritePosition + 1) % SD_CARD_RESPONSE_FIFO_SIZE;
   }
}

static void sdCardResponseFifoFlush(void){
   palmSdCard.responseReadPosition = palmSdCard.responseWritePosition;
   palmSdCard.responseReadPositionBit = 7;
}

//basic responses
static void sdCardDoResponseDelay(uint16_t bytes){
   uint16_t index;
   for(index = 0; index < bytes; index++)
      sdCardResponseFifoWriteByte(0xFF);
}

static void sdCardDoResponseBusy(uint16_t bytes){
   uint16_t index;
   for(index = 0; index < bytes; index++)
      sdCardResponseFifoWriteByte(0x00);
}

static void sdCardDoResponseR1(uint8_t r1){
   sdCardDoResponseDelay(SD_CARD_NCR_BYTES);
   sdCardResponseFifoWriteByte(r1);
}

static void sdCardDoResponseR2(uint8_t r1, uint8_t status){
   sdCardDoResponseR1(r1);
   sdCardResponseFifoWriteByte(status);
}

static void sdCardDoResponseR3R7(uint8_t r1, uint32_t value){
   sdCardDoResponseR1(r1);
   sdCardResponseFifoWriteByte(value >> 24);
   sdCardResponseFifoWriteByte(value >> 16 & 0xFF);
   sdCardResponseFifoWriteByte(value >> 8 & 0xFF);
   sdCardResponseFifoWriteByte(value & 0xFF);
}

static void sdCardDoResponseDataPacket(uint8_t token, const uint8_t* data, uint16_t size){
   uint16_t crc16;
   uint16_t offset;

   if(likely(palmSdCard.allowInvalidCrc))
      crc16 = 0x0000;
   else
      crc16 = sdCardCrc16(data, size);

   sdCardResponseFifoWriteByte(token);
   for(offset = 0; offset < size; offset++)
      sdCardResponseFifoWriteByte(data[offset]);
   sdCardResponseFifoWriteByte(crc16 >> 8);
   sdCardResponseFifoWriteByte(crc16 & 0xFF);
}

static void sdCardDoResponseDataResponse(uint8_t response){
   sdCardResponseFifoWriteByte(response);
   sdCardDoResponseBusy(1);
}

static void sdCardDoResponseErrorToken(uint8_t token){
   sdCardResponseFifoWriteByte(token);
}

//register getters
static void sdCardGetCsd(uint8_t* csd){
   uint16_t deviceSize;

   memcpy(csd, palmSdCard.sdInfo.csd, 16);

   //set device size field(in multiples of 256k right now, the multiplier size also scales based on chip size), needed to get size
   deviceSize = palmSdCard.flashChip.size / SD_CARD_BLOCK_SIZE / 512;//to calculate the card capacity excluding security area ((device size + 1) * device size multiplier * max read data block length) bytes
   csd[6] = csd[6] & 0xFC | deviceSize >> 10 & 0x03;
   csd[7] = deviceSize >> 2 & 0xFF;
   csd[8] = csd[8] & 0x3F | deviceSize << 6 & 0xC0;

   if(unlikely(!palmSdCard.allowInvalidCrc))
      csd[15] = sdCardCrc7(csd, 15);

   //debugLog("CSD bytes: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", csd[0], csd[1], csd[2], csd[3], csd[4], csd[5], csd[6], csd[7], csd[8], csd[9], csd[10], csd[11], csd[12], csd[13], csd[14], csd[15]);
}

static void sdCardGetCid(uint8_t* cid){
   memcpy(cid, palmSdCard.sdInfo.cid, 16);

   if(unlikely(!palmSdCard.allowInvalidCrc))
      cid[15] = sdCardCrc7(cid, 15);
}

static void sdCardGetScr(uint8_t* scr){
   //dont know it this needs a CRC7 or not?
   memcpy(scr, palmSdCard.sdInfo.scr, 8);
}

static uint32_t sdCardGetOcr(void){
   return !palmSdCard.inIdleState << 31/*power up status*/ | 0 << 30/*card capacity status*/ | palmSdCard.sdInfo.ocr/*supported voltages, misc stuff*/;
}
