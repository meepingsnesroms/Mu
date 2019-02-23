//FIFO accessors
static uint16_t sdCardResponseFifoByteEntrys(void){
   //check for wraparound
   if(palmSdCard.responseWritePosition <palmSdCard.responseReadPosition)
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
static void sdCardDoResponseR1(uint8_t r1){
   uint8_t ncrByte = 0;
   for(ncrByte = 0; ncrByte < SD_CARD_NCR_BYTES; ncrByte++)
      sdCardResponseFifoWriteByte(0xFF);
   sdCardResponseFifoWriteByte(r1);
}

static void sdCardDoResponseR3R7(uint8_t r1, uint32_t value){
   sdCardDoResponseR1(r1);
   sdCardResponseFifoWriteByte(value >> 24);
   sdCardResponseFifoWriteByte(value >> 16 & 0xFF);
   sdCardResponseFifoWriteByte(value >> 8 & 0xFF);
   sdCardResponseFifoWriteByte(value & 0xFF);
}

static void sdCardDoResponseDataPacket(uint8_t token, uint8_t* location, uint16_t size){
   uint16_t crc16 = 0x0000;
   uint16_t offset;

   sdCardResponseFifoWriteByte(token);

   for(offset = 0; offset < size; offset++){
      sdCardResponseFifoWriteByte(token);
      //need to add byte to CRC16 here
   }

   sdCardResponseFifoWriteByte(crc16 >> 8);
   sdCardResponseFifoWriteByte(crc16 & 0xFF);
}

static void sdCardDoResponseDelay(uint16_t bytes){
   uint16_t index;
   for(index = 0; index < bytes; index++)
      sdCardResponseFifoWriteByte(0xFF);
}
