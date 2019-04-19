#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/*compile time vars*/
#define APP_NAME    "MuExpDriver"
#define APP_ID      'MuDv'

/*config vars*/
enum{
   USER_WARNING_GIVEN = 0,
   DRIVER_ENABLED,
   SAFE_MODE,
   BOOT_CPU_SPEED,
   PATCH_INCONSISTENT_APIS,
   /*add new entries above*/
   CONFIG_FILE_ENTRIES
};

void readConfigFile(uint32_t* configFile);
void writeConfigFile(uint32_t* configFile);

#endif
