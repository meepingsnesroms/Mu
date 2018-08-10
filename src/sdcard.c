#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "emulator.h"
#include "bps/libbps.h"


static uint8_t*  sdCardData;
static uint8_t*  sdCardOldData;
static uint64_t* sdCardChunks;//0 terminated array
static uint64_t  sdCardChunkIndex;
static uint64_t  sdCardChunkMaxIndex;
static uint64_t  sdCardSize;


static inline void updateChunkBufferLength(){
   uint64_t newChunkLength = 0;

   while(true){
      newChunkLength++;
      if(sdCardChunks[newChunkLength] == 0x0000000000000000)
         break;
   }
   
   sdCardChunkIndex = newChunkLength;
   sdCardChunkMaxIndex = newChunkLength;
}


void sdCardInit(){
   sdCardData = NULL;
   sdCardOldData = NULL;
   sdCardChunks = NULL;
   sdCardChunkIndex = 0;
   sdCardChunkMaxIndex = 0;
   sdCardSize = 0;
}

void sdCardExit(){
   if(sdCardData)
      free(sdCardData);
   if(sdCardOldData)
      free(sdCardOldData);
   sdCardData = NULL;
   sdCardOldData = NULL;
   sdCardChunks = NULL;
   sdCardChunkIndex = 0;
   sdCardChunkMaxIndex = 0;
   sdCardSize = 0;
}

uint32_t sdCardReconfigure(uint64_t size){
   if(sdCardData)
      free(sdCardData);
   if(sdCardOldData)
      free(sdCardOldData);
   if(sdCardChunks)
      free(sdCardChunks);
      
   sdCardData = malloc(size);
   if(!sdCardData)
      return EMU_ERROR_OUT_OF_MEMORY;
   
   sdCardOldData = malloc(size);
   if(!sdCardOldData)
      return EMU_ERROR_OUT_OF_MEMORY;
   
   sdCardChunks = malloc(sizeof(uint64_t) * SD_CARD_CHUNK_VECTOR_SIZE);
   if(!sdCardChunks)
      return EMU_ERROR_OUT_OF_MEMORY;
   
   memset(sdCardData, 0x00, size);
   memset(sdCardOldData, 0x00, size);
   sdCardChunks[0] = 0x0000000000000000;//0 terminated array
   
   sdCardSize = size;
   sdCardChunkIndex = 0;
      
   return EMU_ERROR_NONE;
}

buffer_t sdCardGetImage(){
   buffer_t image;

   image.data = sdCardData;
   image.size = sdCardSize;

   return image;
}

uint32_t sdCardSetFromImage(buffer_t image){
   uint32_t error = sdCardReconfigure(image.size);

   if(error != EMU_ERROR_NONE)
      return error;

   memcpy(sdCardData, image.data, image.size);

   return EMU_ERROR_NONE;
}

void sdCardSaveState(uint64_t sessionId, uint64_t stateId){
   //make single BPS if SD card data has changed
   if(memcmp(sdCardOldData, sdCardData, sdCardSize) != 0){
      struct mem source;
      struct mem target;
      struct mem metadata;
      struct mem patch;
      buffer_t chunk;
      
      source.ptr = sdCardOldData;
      source.len = sdCardSize;
      
      target.ptr = sdCardData;
      target.len = sdCardSize;
      
      metadata.ptr = NULL;
      metadata.len = 0;
      
      bps_create_linear(source, target, metadata, &patch);
      chunk.data = patch.ptr;
      chunk.size = patch.len;
      emulatorSetSdCardChunk(sessionId, stateId, chunk);
      bps_free(patch);
      
      if(sdCardChunkIndex + 2 >= sdCardChunkMaxIndex){
         //resize vector if needed
         sdCardChunks = realloc(sdCardChunks, sizeof(uint64_t) * (sdCardChunkMaxIndex + SD_CARD_CHUNK_VECTOR_SIZE));
         sdCardChunkMaxIndex += SD_CARD_CHUNK_VECTOR_SIZE;
      }
      sdCardChunks[sdCardChunkIndex] = stateId;
      sdCardChunkIndex++;
      sdCardChunks[sdCardChunkIndex] = 0x0000000000000000;
      
      emulatorSetSdCardStateChunkList(sessionId, stateId, sdCardChunks);
      
      memcpy(sdCardOldData, sdCardData, sdCardSize);
   }
}

void sdCardLoadState(uint64_t sessionId, uint64_t stateId){
   //rebuild SD card from BPS chain
   free(sdCardChunks);
   sdCardChunks = emulatorGetSdCardStateChunkList(sessionId, stateId);
   updateChunkBufferLength();
   
   memset(sdCardData, 0x00, sdCardSize);
   for(uint64_t index = 0; index < sdCardChunkIndex; index++){
      struct mem patch;
      struct mem input;
      struct mem output;
      buffer_t chunk = emulatorGetSdCardChunk(sessionId, sdCardChunks[index]);
      
      patch.ptr = chunk.data;
      patch.len = chunk.size;
      
      input.ptr = sdCardData;
      input.len = sdCardSize;
      
      output.ptr = sdCardData;
      output.len = sdCardSize;
      
      
      //bps_apply(patch, input, output, NULL, true);//bps_apply needs to be modified to patch the existing buffer instead of making a new one
   }
   
   memcpy(sdCardOldData, sdCardData, sdCardSize);
}

bool sdCardExchangeBit(bool bit){
   //not done
   return true;//eek
}
