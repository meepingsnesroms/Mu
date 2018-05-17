static void irdaTransmitUint8(uint8_t data){
   if(irdaRunning)
      SrmSend(irdaPortId, &data, 1/*bytes*/, &irdaError);
}

static uint8_t irdaReceiveUint8(){
   if(irdaRunning){
      uint8_t data;
      
      SrmReceive(irdaPortId, &data, 1/*bytes*/, IRDA_WAIT_FOREVER, &irdaError);
      return data;
   }
   
   return 0x00;
}

static void irdaTransmitUint16(uint16_t data){
   if(irdaRunning)
      SrmSend(irdaPortId, &data, 2/*bytes*/, &irdaError);
}

static uint16_t irdaReceiveUint16(){
   if(irdaRunning){
      uint16_t data;
      
      SrmReceive(irdaPortId, &data, 2/*bytes*/, IRDA_WAIT_FOREVER, &irdaError);
      return data;
   }
   
   return 0x0000;
}

static void irdaTransmitUint32(uint32_t data){
   if(irdaRunning)
      SrmSend(irdaPortId, &data, 4/*bytes*/, &irdaError);
}

static uint32_t irdaReceiveUint32(){
   if(irdaRunning){
      uint32_t data;
      
      SrmReceive(irdaPortId, &data, 4/*bytes*/, IRDA_WAIT_FOREVER, &irdaError);
      return data;
   }
   
   return 0x00000000;
}

static void irdaTransmitBuffer(uint8_t* data, uint32_t size){
   if(irdaRunning)
      SrmSend(irdaPortId, data, size, &irdaError);
}

static void irdaReceiveBuffer(uint8_t* data, uint32_t size){
   if(irdaRunning)
      SrmReceive(irdaPortId, data, size/*bytes*/, IRDA_WAIT_FOREVER, &irdaError);
}
