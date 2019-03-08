#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/*compile time vars*/
#define APP_NAME    "MuExpDriver"
#define APP_CREATOR 'GuiC'

#define FUNCTION_BLOB_TYPE          'func'
#define ARM_EXIT_FUNC_ID            0x0000
#define ARM_CALL_68K_FUNC_ID        0x0001
#define M68K_CALL_WITH_BLOB_FUNC_ID 0x0002

/*config vars*/
enum{
   USER_WARNING_GIVEN = 0,
   DRIVER_ENABLED,
   SAFE_MODE,
   ARM_STACK_SIZE,
   LCD_WIDTH,
   LCD_HEIGHT,
   EXTRA_RAM_MB_DYNAMIC_HEAP,
   BOOT_CPU_SPEED,
   /*add new entries above*/
   CONFIG_FILE_ENTRIES
};

void readConfigFile(uint32_t* configFile);
void writeConfigFile(uint32_t* configFile);

#endif
