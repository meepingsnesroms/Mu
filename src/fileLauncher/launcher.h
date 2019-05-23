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
   LAUNCHER_FILE_TYPE_RESOURCE_FILE,
   LAUNCHER_FILE_TYPE_IMG
};

typedef struct{
   uint8_t  type;//file type
   uint8_t* fileData;
   uint32_t fileSize;
   uint8_t* infoData;//only used with LAUNCHER_FILE_TYPE_IMG right now
   uint32_t infoSize;
}launcher_file_t;

extern bool launcherSaveSdCardImage;//false if loading a read only SD card image

/*
the launcher is called after emulatorInit when its enabled
the order is:
launcher_file_t* files[...];
uint32_t fileCount;
uint8_t* saveData = NULL;
uint32_t saveSize = 0;
uint8_t* oldSdCardData = NULL;
uint32_t oldSdCardSize = 0;
uint32_t error;

error = emulatorInit(romFileData, romFileSize, bootloaderFileData(if m515), bootloaderFileSize(if m515), features);
if(error)
   return error;
error = launcherLaunch(files, fileCount, saveData, saveSize, oldSdCardData, oldSdCardSize);//this can fail installing apps
if(error)
   return error;
//its now safe to call emulatorFrame for frames
*/
//if first launch NULL should be passed for sramData and sdCardData
uint32_t launcherLaunch(launcher_file_t* files, uint32_t fileCount, uint8_t* sramData, uint32_t sramSize, uint8_t* sdCardData, uint32_t sdCardSize);

//only call after the emu has booted, launcherLaunch() will ensure a full boot has completed otherwise this is up to the frontend to be safe
uint32_t launcherInstallFiles(launcher_file_t* files, uint32_t fileCount);

#ifdef __cplusplus
}
#endif

#endif
