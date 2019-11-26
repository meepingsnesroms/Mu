#include "libretro.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <compat/strl.h>
#include <retro_miscellaneous.h>
#include <streams/file_stream.h>
#include <string/stdstring.h>

#include "../src/emulator.h"
#include "../src/portability.h"
#include "../src/silkscreen.h"
#include "../src/fileLauncher/launcher.h"
#include "cursors.h"


#define JOYSTICK_DEADZONE 4000
#define JOYSTICK_MULTIPLIER 0.0001
#define SCREEN_HIRES (!(palmFramebufferWidth == 160))


static retro_log_printf_t         log_cb = NULL;
static retro_video_refresh_t      video_cb = NULL;
static retro_audio_sample_batch_t audio_cb = NULL;
static retro_set_led_state_t      led_cb = NULL;
static retro_environment_t        environ_cb = NULL;
static retro_input_poll_t         input_poll_cb = NULL;
static retro_input_state_t        input_state_cb = NULL;

static double      cpuSpeed;
static bool        syncRtc;
static bool        allowInvalidBehavior;
static const char* osVersion;
static uint8_t     deviceModel;
static bool        firstRetroRunCall;
static bool        dontRenderGraffiti;
static bool        useJoystickAsMouse;
static float       touchCursorX;
static float       touchCursorY;
static char        contentPath[PATH_MAX_LENGTH];
static uint16_t    mouseCursorOldArea[32 * 32];
static bool        runningImgFile;
static uint16_t    screenYEnd;


static void frontendGetCurrentTime(uint8_t* writeBack){
   time_t rawTime;
   struct tm* timeInfo;

   time(&rawTime);
   timeInfo = localtime(&rawTime);

   writeBack[0] = timeInfo->tm_hour;
   writeBack[1] = timeInfo->tm_min;
   writeBack[2] = timeInfo->tm_sec;
}

static void renderMouseCursor(int16_t screenX, int16_t screenY){
   if(SCREEN_HIRES){
      int8_t x;
      int8_t y;
      
      //align cursor to side of image
      screenX -= 6;
      
      for(y = 0; y < 32; y++){
         for(x = 6; x < 26; x++){
            if(screenX + x >= 0 && screenY + y >= 0 && screenX + x < palmFramebufferWidth && screenY + y < palmFramebufferHeight){
               mouseCursorOldArea[y * 32 + x] = palmFramebuffer[(screenY + y) * palmFramebufferWidth + screenX + x];
               if(cursor32x32[y * 32 + x] != 0xFFFF)
                  palmFramebuffer[(screenY + y) * palmFramebufferWidth + screenX + x] = cursor32x32[y * 32 + x];
            }
         }
      }
   }
   else{
      int8_t x;
      int8_t y;
      
      //align cursor to side of image
      screenX -= 3;
      
      for(y = 0; y < 16; y++){
         for(x = 3; x < 13; x++){
            if(screenX + x >= 0 && screenY + y >= 0 && screenX + x < palmFramebufferWidth && screenY + y < palmFramebufferHeight){
               mouseCursorOldArea[y * 16 + x] = palmFramebuffer[(screenY + y) * palmFramebufferWidth + screenX + x];
               if(cursor16x16[y * 16 + x] != 0xFFFF)
                  palmFramebuffer[(screenY + y) * palmFramebufferWidth + screenX + x] = cursor16x16[y * 16 + x];
            }
         }
      }
   }
}

static void unrenderMouseCursor(int16_t screenX, int16_t screenY){
   if(SCREEN_HIRES){
      int8_t x;
      int8_t y;
      
      //align cursors missing rectangle to side of image
      screenX -= 6;
      
      for(y = 0; y < 32; y++)
         for(x = 6; x < 26; x++)
            if(screenX + x >= 0 && screenY + y >= 0 && screenX + x < palmFramebufferWidth && screenY + y < palmFramebufferHeight)
                  palmFramebuffer[(screenY + y) * palmFramebufferWidth + screenX + x] = mouseCursorOldArea[y * 32 + x];
   }
   else{
      int8_t x;
      int8_t y;
      
      //align cursors missing rectangle to side of image
      screenX -= 3;
      
      for(y = 0; y < 16; y++)
         for(x = 3; x < 13; x++)
            if(screenX + x >= 0 && screenY + y >= 0 && screenX + x < palmFramebufferWidth && screenY + y < palmFramebufferHeight)
                  palmFramebuffer[(screenY + y) * palmFramebufferWidth + screenX + x] = mouseCursorOldArea[y * 16 + x];
   }
}

static void fallback_log(enum retro_log_level level, const char *fmt, ...){
   va_list va;

   (void)level;

   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

static void check_variables(bool booting){
   struct retro_variable var = {0};

   if(booting){
      var.key = "palm_emu_cpu_speed";
      if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         cpuSpeed = atoi(var.value);
      
      var.key = "palm_emu_feature_synced_rtc";
      if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         syncRtc = !strcmp(var.value, "enabled");
      
      var.key = "palm_emu_feature_durable";
      if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         allowInvalidBehavior = !strcmp(var.value, "enabled");
   }

   var.key = "palm_emu_use_joystick_as_mouse";
   if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      useJoystickAsMouse = !strcmp(var.value, "enabled");
   
   var.key = "palm_emu_disable_graffiti";
   if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
      dontRenderGraffiti = !strcmp(var.value, "enabled");
   
   var.key = "palm_emu_os_version";
   if(environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value){
      //Palm m515/Palm OS 4.1|Tungsten T3/Palm OS 5.2.1|Tungsten T3/Palm OS 6.0|Palm m500/Palm OS 4.0
      if(!strcmp(var.value, "Palm m500/Palm OS 4.0")){
         deviceModel = EMU_DEVICE_PALM_M500;
         osVersion = "palmos40-en-m500";
      }
      else if(!strcmp(var.value, "Palm m515/Palm OS 4.1")){
         deviceModel = EMU_DEVICE_PALM_M515;
         osVersion = "palmos41-en-m515";
      }
#if defined(EMU_SUPPORT_PALM_OS5)
      else if(!strcmp(var.value, "Tungsten T3/Palm OS 5.2.1")){
         deviceModel = EMU_DEVICE_TUNGSTEN_T3;
         osVersion = "palmos52-en-t3";
      }
      else if(!strcmp(var.value, "Tungsten T3/Palm OS 6.0")){
         deviceModel = EMU_DEVICE_TUNGSTEN_T3;
         osVersion = "palmos60-en-t3";
      }
#endif
      else{
         deviceModel = EMU_DEVICE_PALM_M515;
         osVersion = "palmos41-en-m515";
      }
   }
}

void retro_init(void){
   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   
   if(environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565))
      log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");
}

void retro_deinit(void){
   
}

unsigned retro_api_version(void){
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device){
   (void)port;
   (void)device;
}

void retro_get_system_info(struct retro_system_info *info){
   memset(info, 0, sizeof(*info));
   info->library_name     = "Mu";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v1.1.0" GIT_VERSION;
   info->need_fullpath    = true;
   info->valid_extensions = "prc|pqa|img";
}

void retro_get_system_av_info(struct retro_system_av_info *info){
   info->timing.fps = EMU_FPS;
   info->timing.sample_rate = AUDIO_SAMPLE_RATE;

   info->geometry.base_width   = 160;
   info->geometry.base_height  = 220;
#if defined(EMU_SUPPORT_PALM_OS5)
   info->geometry.max_width    = 320;
   info->geometry.max_height   = 480;
#else
   info->geometry.max_width    = 160;
   info->geometry.max_height   = 220;
#endif
   info->geometry.aspect_ratio = 160.0 / 220.0;
}

void retro_set_environment(retro_environment_t cb){
   struct retro_log_callback logging;
   struct retro_vfs_interface_info vfs_getter = { 1, NULL };
   struct retro_led_interface led_getter;
   struct retro_variable vars[] = {
      { "palm_emu_cpu_speed", "CPU Speed; 1.0|1.5|2.0|2.5|3.0|0.5" },
      { "palm_emu_feature_synced_rtc", "Force Match System Clock; disabled|enabled" },
      { "palm_emu_feature_durable", "Ignore Invalid Behavior; disabled|enabled" },
      { "palm_emu_use_joystick_as_mouse", "Use Left Joystick As Mouse; disabled|enabled" },
      { "palm_emu_disable_graffiti", "Disable Graffiti Area; disabled|enabled" },
#if defined(EMU_SUPPORT_PALM_OS5)
      { "palm_emu_os_version", "OS Version; Palm m515/Palm OS 4.1|Tungsten T3/Palm OS 5.2.1|Tungsten T3/Palm OS 6.0|Palm m500/Palm OS 4.0" },
#else
      { "palm_emu_os_version", "OS Version; Palm m515/Palm OS 4.1|Palm m500/Palm OS 4.0" },
#endif
      { 0 }
   };
   struct retro_input_descriptor input_desc[] = {
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Touchscreen Mouse X" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Touchscreen Mouse Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,                              "Touchscreen Mouse Click" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,                             "Dpad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,                           "Dpad Down" },
#if defined(EMU_SUPPORT_PALM_OS5)
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,                           "Dpad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,                          "Dpad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,                         "Dpad Center" },
#endif
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,                          "Power" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,                              "Date Book" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,                              "Address Book" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,                              "To Do List" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,                              "Note Pad" },
      { 0 }
   };
   bool no_rom = true;

   environ_cb = cb;
   
   environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

   if(environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging) && logging.log)
      log_cb = logging.log;
   
   if(!log_cb)
      log_cb = fallback_log;
   
   led_getter.set_led_state = NULL;
   if(environ_cb(RETRO_ENVIRONMENT_GET_LED_INTERFACE, &led_getter) && led_getter.set_led_state)
      led_cb = led_getter.set_led_state;
   
   if(environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_getter) && vfs_getter.iface)
      filestream_vfs_init(&vfs_getter);

   environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, vars);
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, input_desc);
}

void retro_set_audio_sample(retro_audio_sample_t cb){
   
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb){
   audio_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb){
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb){
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb){
   video_cb = cb;
}

void retro_reset(void){
   emulatorSoftReset();
}

void retro_run(void){
   input_poll_cb();
   
   //some RetroArch functions can only be called from this function so call those if needed
   if(unlikely(firstRetroRunCall)){
      struct retro_game_geometry geometry;
#if defined(EMU_SUPPORT_PALM_OS5)
      if(deviceModel == EMU_DEVICE_TUNGSTEN_T3){
         if(dontRenderGraffiti){
            geometry.base_width   = 320;
            geometry.base_height  = 320;
            geometry.max_width    = 320;
            geometry.max_height   = 480;
         }
         else{
            geometry.base_width   = 320;
            geometry.base_height  = 480;
         }
      }
      else{
#endif
         if(dontRenderGraffiti){
            geometry.base_width   = 160;
            geometry.base_height  = 160;
         }
         else{
            geometry.base_width   = 160;
            geometry.base_height  = 220;
         }
#if defined(EMU_SUPPORT_PALM_OS5)
      }
#endif
      geometry.aspect_ratio = (float)geometry.base_width / (float)geometry.base_height;
      environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &geometry);
      firstRetroRunCall = false;
   }
   
   //touchscreen
   if(useJoystickAsMouse){
      int16_t x = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
      int16_t y = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
      
      if(x < -JOYSTICK_DEADZONE || x > JOYSTICK_DEADZONE)
         touchCursorX += x * JOYSTICK_MULTIPLIER * (SCREEN_HIRES ? 2.0 : 1.0);
      
      if(y < -JOYSTICK_DEADZONE || y > JOYSTICK_DEADZONE)
         touchCursorY += y * JOYSTICK_MULTIPLIER * (SCREEN_HIRES ? 2.0 : 1.0);
      
      if(touchCursorX < 0)
         touchCursorX = 0;
      else if(touchCursorX > palmFramebufferWidth - 1)
         touchCursorX = palmFramebufferWidth - 1;
      
      if(touchCursorY < 0)
         touchCursorY = 0;
      else if(touchCursorY > palmFramebufferHeight - 1)
         touchCursorY = palmFramebufferHeight - 1;
      
      palmInput.touchscreenX = touchCursorX / (palmFramebufferWidth - 1);
      palmInput.touchscreenY = touchCursorY / (palmFramebufferHeight - 1);
      palmInput.touchscreenTouched = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
   }
   else{
      //use RetroArch internal pointer
      palmInput.touchscreenX = ((float)input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X) / 0x7FFF + 1.0) / 2.0;
      palmInput.touchscreenY = ((float)input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y) / 0x7FFF + 1.0) / 2.0 * ((float)screenYEnd / palmFramebufferHeight);
      palmInput.touchscreenTouched = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
   }

   //dpad
   palmInput.buttonUp = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
   palmInput.buttonDown = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
#if defined(EMU_SUPPORT_PALM_OS5)
   if(deviceModel == EMU_DEVICE_TUNGSTEN_T3){
      palmInput.buttonLeft = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
      palmInput.buttonRight = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
      palmInput.buttonCenter = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);
   }
#endif
   
   //app buttons
   palmInput.buttonCalendar = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);
   palmInput.buttonAddress = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
   palmInput.buttonTodo = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
   palmInput.buttonNotes = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
   
   //special buttons
   palmInput.buttonPower = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
   
   //run emulator
   emulatorRunFrame();
   
   //draw mouse
   if(useJoystickAsMouse)
      renderMouseCursor(touchCursorX, touchCursorY);
   
   video_cb(palmFramebuffer, palmFramebufferWidth, screenYEnd, palmFramebufferWidth * sizeof(uint16_t));
   audio_cb(palmAudio, AUDIO_SAMPLES_PER_FRAME);
   if(led_cb){
      led_cb(0, palmMisc.greenLed);
#if defined(EMU_SUPPORT_PALM_OS5)
      led_cb(1, palmMisc.redLed);
#endif
   }
   
   //repair damage done to the framebuffer by the mouse cursor
   if(useJoystickAsMouse)
      unrenderMouseCursor(touchCursorX, touchCursorY);
}

bool retro_load_game(const struct retro_game_info *info){
   uint8_t* romData;
   uint32_t romSize;
   uint8_t* bootloaderData;
   uint32_t bootloaderSize;
   char romPath[PATH_MAX_LENGTH];
   char bootloaderPath[PATH_MAX_LENGTH];
   char saveRamPath[PATH_MAX_LENGTH];
   char sdImgPath[PATH_MAX_LENGTH];
   struct RFILE* romFile;
   struct RFILE* bootloaderFile;
   struct RFILE* saveRamFile;
   struct RFILE* sdImgFile;
   const char* systemDir;
   time_t rawTime;
   struct tm* timeInfo;
   bool hasSram = false;
   uint32_t error;
   
   //updates the emulator configuration
   check_variables(true);
   
   environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &systemDir);
   
   if(info && !string_is_empty(info->path)){
      //boot application
      strlcpy(contentPath, info->path, PATH_MAX_LENGTH);
      runningImgFile = string_is_equal_case_insensitive(contentPath + strlen(contentPath) - 4, ".img");
   }
   else{
      //boot standard device image
      strlcpy(contentPath, systemDir, PATH_MAX_LENGTH);
      strlcat(contentPath, "/default", PATH_MAX_LENGTH);
      runningImgFile = false;
   }
   
   //ROM
   strlcpy(romPath, systemDir, PATH_MAX_LENGTH);
   strlcat(romPath, osVersion, PATH_MAX_LENGTH);
   strlcat(romPath, ".rom", PATH_MAX_LENGTH);
   romFile = filestream_open(romPath, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if(romFile){
      romSize = filestream_get_size(romFile);
      romData = malloc(romSize);
      
      if(romData)
         filestream_read(romFile, romData, romSize);
      else
         romSize = 0;
      filestream_close(romFile);
   }
   else{
      //cant load without ROM
      return false;
   }
   
   //bootloader, will simply be ignored for Tungsten T3
   strlcpy(bootloaderPath, systemDir, PATH_MAX_LENGTH);
   strlcat(bootloaderPath, "/bootloader-dbvz.rom", PATH_MAX_LENGTH);
   bootloaderFile = filestream_open(bootloaderPath, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if(bootloaderFile){
      bootloaderSize = filestream_get_size(bootloaderFile);
      bootloaderData = malloc(bootloaderSize);
      
      if(bootloaderData)
         filestream_read(bootloaderFile, bootloaderData, bootloaderSize);
      else
         bootloaderSize = 0;
      filestream_close(bootloaderFile);
   }
   else{
      bootloaderData = NULL;
      bootloaderSize = 0;
   }
   
   error = emulatorInit(deviceModel, romData, romSize, bootloaderData, bootloaderSize, syncRtc, allowInvalidBehavior);
   free(romData);
   if(bootloaderData)
      free(bootloaderData);
   
   if(error != EMU_ERROR_NONE)
      return false;
   
   //save RAM
   strlcpy(saveRamPath, contentPath, PATH_MAX_LENGTH);
   strlcat(saveRamPath, "-", PATH_MAX_LENGTH);
   strlcat(saveRamPath, osVersion, PATH_MAX_LENGTH);
   strlcat(saveRamPath, ".ram", PATH_MAX_LENGTH);
   saveRamFile = filestream_open(saveRamPath, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if(saveRamFile){
      hasSram = true;
      if(filestream_get_size(saveRamFile) == emulatorGetRamSize()){
         filestream_read(saveRamFile, palmRam, emulatorGetRamSize());
         swap16BufferIfLittle(palmRam, emulatorGetRamSize() / sizeof(uint16_t));
      }
      filestream_close(saveRamFile);
   }
   
   if(!runningImgFile){
      //SD card
      strlcpy(sdImgPath, contentPath, PATH_MAX_LENGTH);
      strlcat(sdImgPath, "-", PATH_MAX_LENGTH);
      strlcat(sdImgPath, osVersion, PATH_MAX_LENGTH);
      strlcat(sdImgPath, ".sd.img", PATH_MAX_LENGTH);
      sdImgFile = filestream_open(sdImgPath, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
      if(sdImgFile){
         uint32_t sdImgSize = filestream_get_size(sdImgFile);
         
         //use the NULL, size, NULL method because it takes less RAM
         
         error = emulatorInsertSdCard(NULL, sdImgSize, NULL);
         if(error == EMU_ERROR_NONE)
            filestream_read(sdImgFile, palmSdCard.flashChipData, sdImgSize);
         
         filestream_close(sdImgFile);
      }
   }
   
   //set RTC
   time(&rawTime);
   timeInfo = localtime(&rawTime);
   emulatorSetRtc(timeInfo->tm_yday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
   
   //see if RetroArch wants something launched
   if(info && !string_is_empty(info->path)){
      struct RFILE* contentFile;
      uint8_t* contentData;
      uint32_t contentSize;
      
      contentFile = filestream_open(contentPath, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
      if(contentFile){
         contentSize = filestream_get_size(contentFile);
         contentData = malloc(contentSize);
         
         if(contentData)
            filestream_read(contentFile, contentData, contentSize);
         else
            return false;
         filestream_close(contentFile);
      }
      else{
         //no content at path, fail time
         return false;
      }
      
      launcherBootInstantly(hasSram);

      if(runningImgFile){
         char infoPath[PATH_MAX_LENGTH];
         struct RFILE* infoFile;
         uint8_t* infoData = NULL;
         uint32_t infoSize;
         sd_card_info_t sdInfo;
         
         memset(&sdInfo, 0x00, sizeof(sdInfo));
         
         strlcpy(infoPath, contentPath, PATH_MAX_LENGTH);
         infoPath[strlen(infoPath) - 4] = '\0';//chop off ".img"
         strlcat(infoPath, ".info", PATH_MAX_LENGTH);
         infoFile = filestream_open(infoPath, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
         if(infoFile){
            infoSize = filestream_get_size(infoFile);
            infoData = malloc(infoSize);
            
            if(infoData)
               filestream_read(infoFile, infoData, infoSize);

            filestream_close(infoFile);
         }
         
         if(infoData)
            launcherGetSdCardInfoFromInfoFile(infoData, infoSize, &sdInfo);
         error = emulatorInsertSdCard(contentData, contentSize, infoData ? &sdInfo : NULL);
         if(infoData)
            free(infoData);
      }
      else{
         if(!hasSram)
            error = launcherInstallFile(contentData, contentSize);
         if(error == EMU_ERROR_NONE)
            error = launcherExecute(launcherGetAppId(contentData, contentSize));
      }
      
      free(contentData);
      if(error != EMU_ERROR_NONE)
         return false;
   }
   
   //set the time callback
   palmGetRtcFromHost = frontendGetCurrentTime;
   
   //set mouse position
   touchCursorX = palmFramebufferWidth / 2;
   touchCursorY = palmFramebufferHeight / 2;
   
   //make touches land on the correct spot and screen render the correct size when the graffiti area is off
   if(dontRenderGraffiti){
#if defined(EMU_SUPPORT_PALM_OS5)
      if(deviceModel == EMU_DEVICE_TUNGSTEN_T3)
         screenYEnd = 320;
      else
#endif
         screenYEnd = 160;
   }
   else{
      screenYEnd = palmFramebufferHeight;
   }
   
   //used to resize things properly
   firstRetroRunCall = true;
   
   //set default CPU speed
   emulatorSetCpuSpeed(cpuSpeed);

   return true;
}

void retro_unload_game(void){
   char saveRamPath[PATH_MAX_LENGTH];
   char sdImgPath[PATH_MAX_LENGTH];
   struct RFILE* saveRamFile;
   struct RFILE* sdImgFile;
   
   //save RAM
   strlcpy(saveRamPath, contentPath, PATH_MAX_LENGTH);
   strlcat(saveRamPath, "-", PATH_MAX_LENGTH);
   strlcat(saveRamPath, osVersion, PATH_MAX_LENGTH);
   strlcat(saveRamPath, ".ram", PATH_MAX_LENGTH);
   saveRamFile = filestream_open(saveRamPath, RETRO_VFS_FILE_ACCESS_WRITE, RETRO_VFS_FILE_ACCESS_HINT_NONE);
   if(saveRamFile){
      swap16BufferIfLittle(palmRam, emulatorGetRamSize() / sizeof(uint16_t));//this will no longer be used, so its ok to destroy it when swapping
      filestream_write(saveRamFile, palmRam, emulatorGetRamSize());
      filestream_close(saveRamFile);
   }
   
   if(!runningImgFile){
      //SD card
      if(palmSdCard.flashChipData){
         strlcpy(sdImgPath, contentPath, PATH_MAX_LENGTH);
         strlcat(sdImgPath, "-", PATH_MAX_LENGTH);
         strlcat(sdImgPath, osVersion, PATH_MAX_LENGTH);
         strlcat(sdImgPath, ".sd.img", PATH_MAX_LENGTH);
         sdImgFile = filestream_open(sdImgPath, RETRO_VFS_FILE_ACCESS_WRITE, RETRO_VFS_FILE_ACCESS_HINT_NONE);
         if(sdImgFile){
            filestream_write(sdImgFile, palmSdCard.flashChipData, palmSdCard.flashChipSize);
            filestream_close(sdImgFile);
         }
      }
   }
   
   emulatorDeinit();
}

unsigned retro_get_region(void){
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num){
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void){
   return emulatorGetStateSize();
}

bool retro_serialize(void *data, size_t size){
   return emulatorSaveState(data, size);
}

bool retro_unserialize(const void *data, size_t size){
   return emulatorLoadState(data, size);
}

void* retro_get_memory_data(unsigned id){
   return NULL;
}

size_t retro_get_memory_size(unsigned id){
   return 0;
}

void retro_cheat_reset(void){
   
}

void retro_cheat_set(unsigned index, bool enabled, const char *code){
   (void)index;
   (void)enabled;
   (void)code;
}

