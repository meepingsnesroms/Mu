#ifndef LAUNCHER_H
#define LAUNCHER_H
//this is the only header a frontend needs to include from the launcher

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "../emulator.h"

void launcherBootInstantly(bool hasSram);//fastforwards through the boot sequence
uint32_t launcherInstallFile(uint8_t* data, uint32_t size);//only call after the emu has booted, launcherBootInstantly() will ensure a full boot has completed otherwise this is up to the frontend to be safe
bool launcherIsExecutable(uint8_t* data, uint32_t size);
uint32_t launcherGetAppId(uint8_t* data, uint32_t size);
uint32_t launcherExecute(uint32_t appId);//must first be installed with launcherInstallFile

void launcherGetSdCardInfoFromInfoFile(uint8_t* data, uint32_t size, sd_card_info_t* returnValue);

#ifdef __cplusplus
}
#endif

#endif
