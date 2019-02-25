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

static void sdCardResponseFifoWriteByte(uint8_t value){
   if(sdCardResponseFifoByteEntrys() < SD_CARD_RESPONSE_FIFO_SIZE - 1){
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

   if(palmSdCard.allowInvalidCrc)
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
   static const uint8_t sdCardCsd[16] = {0x00, 0x2F, 0x00, 0x32, 0x5F, 0x59, 0x83, 0xB8, 0x6D, 0xB7, 0xFF, 0x9F, 0x96, 0x40, 0x00, 0x00};
   uint16_t deviceSize;

   //this will go at some point, I want to build my own CID and CSD
   memcpy(csd, sdCardCsd, 16);

   //set device size field(in multiples of 256k right now, the multiplier size also scales based on chip size), needed to get size
   deviceSize = palmSdCard.flashChip.size / SD_CARD_BLOCK_SIZE / SD_CARD_BLOCK_SIZE;//to calculate the card capacity excluding security area ((device size + 1) * device size multiplier * max read data block length) bytes
   csd[6] = csd[6] & 0xFC | deviceSize >> 10 & 0x03;
   csd[7] = deviceSize >> 2 & 0xFF;
   csd[8] = csd[8] & 0x3F | deviceSize << 6 & 0xC0;

   if(!palmSdCard.allowInvalidCrc)
      csd[15] = sdCardCrc7(csd, 15);

   //debugLog("CSD bytes: 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", csd[0], csd[1], csd[2], csd[3], csd[4], csd[5], csd[6], csd[7], csd[8], csd[9], csd[10], csd[11], csd[12], csd[13], csd[14], csd[15]);
}

static void sdCardGetCid(uint8_t* cid){
   //this will go at some point, I want to build my own CID and CSD
   static const uint8_t sdCardCid[16] = {0x1D, 0x41, 0x44, 0x53, 0x44, 0x20, 0x20, 0x20, 0x10, 0xA0, 0x50, 0x33, 0xA4, 0x00, 0x81, 0x00};

   memcpy(cid, sdCardCid, 16);

   if(!palmSdCard.allowInvalidCrc)
      cid[15] = sdCardCrc7(cid, 15);
}

static void sdCardGetScr(uint8_t* scr){
   static const uint8_t sdCardScr[8] = {0x01, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};//dont know it this needs a CRC7 or not?

   memcpy(scr, sdCardScr, 8);
}

static uint32_t sdCardGetOcr(void){
   return !palmSdCard.inIdleState << 31/*power up status*/ | 0 << 30/*card capacity status*/ | 0x01FF8000/*supported voltages*/;
}
