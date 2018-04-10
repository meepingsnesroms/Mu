#include <stdint.h>
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
   
   sdCardChunks = malloc(sizeof(uint64_t) * SDCARD_STATE_CHUNKS_VECTOR_SIZE);
   if(!sdCardChunks)
      return EMU_ERROR_OUT_OF_MEMORY;
   
   memset(sdCardData, 0x00, size);
   memset(sdCardOldData, 0x00, size);
   sdCardChunks[0] = 0x0000000000000000;//0 terminated array
   
   sdCardSize = size;
   sdCardChunkIndex = 0;
      
   return EMU_ERROR_NONE;
}

void sdCardSaveState(uint64_t sessionId, uint64_t stateId){
   //make single bps if sdcard data has changed
   if(memcmp(sdCardOldData, sdCardData, sdCardSize) != 0){
      struct mem source;
      struct mem target;
      struct mem metadata;
      struct mem patch;
      
      source.ptr = sdCardOldData;
      source.len = sdCardSize;
      
      target.ptr = sdCardData;
      target.len = sdCardSize;
      
      metadata.ptr = NULL;
      metadata.len = 0;
      
      bps_create_linear(source, target, metadata, &patch);
      emulatorSetSdCardChunk(sessionId, stateId, patch.ptr, patch.len);
      bps_free(patch);
      
      if(sdCardChunkIndex + 2 >= sdCardChunkMaxIndex){
         //resize vector if needed
         sdCardChunks = realloc(sdCardChunks, sizeof(uint64_t) * (sdCardChunkMaxIndex + SDCARD_STATE_CHUNKS_VECTOR_SIZE));
         sdCardChunkMaxIndex += SDCARD_STATE_CHUNKS_VECTOR_SIZE;
      }
      sdCardChunks[sdCardChunkIndex] = stateId;
      sdCardChunkIndex++;
      sdCardChunks[sdCardChunkIndex] = 0x0000000000000000;
      
      emulatorSetSdCardStateChunkList(sessionId, stateId, sdCardChunks);
      
      memcpy(sdCardOldData, sdCardData, sdCardSize);
   }
}

void sdCardLoadState(uint64_t sessionId, uint64_t stateId){
   //rebuild sdcard from bps chain
   free(sdCardChunks);
   sdCardChunks = emulatorGetSdCardStateChunkList(sessionId, stateId);
   updateChunkBufferLength();
   
   memset(sdCardData, 0x00, sdCardSize);
   for(uint64_t index = 0; index < sdCardChunkIndex; index++){
      struct mem patch;
      struct mem input;
      struct mem output;
      
      patch.ptr = emulatorGetSdCardChunk(sessionId, sdCardChunks[index]);
      patch.len = sdCardSize;
      
      input.ptr = sdCardData;
      input.len = sdCardSize;
      
      output.ptr = sdCardData;
      output.len = sdCardSize;
      
      
      //bps_apply(patch, input, output, NULL, true);//bps_apply needs to be modified to patch the existing buffer instead of making a new one
   }
   
   memcpy(sdCardOldData, sdCardData, sdCardSize);
}
