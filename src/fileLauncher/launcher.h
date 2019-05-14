#ifndef LAUNCHER_H
#define LAUNCHER_H
//this is the only header a frontend needs to include from the launcher

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "../emulator.h"

enum{
   LAUNCHER_FILE_TYPE_NONE = 0,
   LAUNCHER_FILE_TYPE_PRC,
   LAUNCHER_FILE_TYPE_PDB,
   LAUNCHER_FILE_TYPE_PQA,
   LAUNCHER_FILE_TYPE_ZIP,
   LAUNCHER_FILE_TYPE_IMG,
   LAUNCHER_FILE_TYPE_INFO_IMG
}

typedef struct{
   uint8_t  type;//file type
   bool     boot;//if set will be the application that is launched
   uint8_t* file;
   uint32_t fileSize;
   uint8_t* info;//only used with LAUNCHER_FILE_TYPE_INFO_IMG
   uint32_t infoSize;//only used with LAUNCHER_FILE_TYPE_INFO_IMG
}file_t;

/*
the launcher is called after emulatorInit when its enabled
the order is:
emulatorInit(romFileData, romFileSize, bootloaderFileData(if m515), bootloaderFileSize(if m515), features);
for(count = 0; count < totalFiles; count++)
   launcherAddFile(file[count]);//this hands the file data buffers off to the launcher, dont attempt to free them after this, this could fail!
launcherLaunch();//this can also fail
//its now safe to call emulatorFrame for frames
*/

uint32_t launcherAddFile(file_t file);
uint32_t launcherLaunch(void);
   
#ifdef __cplusplus
}
#endif

#endif
