#include <stdint.h>
#include <stdbool.h>

#include "../emulator.h"


typedef struct{
   file_t file;
   void*  nextEntry;//NULL for none
}file_list_entry_t;


file_list_entry_t launcherFirstFile;
uint32_t          launcherTotalFiles;


uint32_t launcherAddFile(file_t file){
}

uint32_t launcherLaunch(void){
   uint8_t* newSdCardData;
   uint32_t newSdCardSize;
   //a pre specified RAM image for the current device(m515 or Tungsten C)needs to be decompressed into palmRam here
   
   
   emulatorSoftReset();
}