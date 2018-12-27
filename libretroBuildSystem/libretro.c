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

#include "../src/emulator.h"
#include "../src/portability.h"
#include "cursors.h"


#define JOYSTICK_DEADZONE 4000
#define JOYSTICK_MULTIPLIER 0.0001


static retro_log_printf_t          log_cb;
static retro_video_refresh_t       video_cb;
static retro_audio_sample_batch_t  audio_cb;
static retro_environment_t         environ_cb;
static retro_input_poll_t          input_poll_cb;
static retro_input_state_t         input_state_cb;

static bool     screenHires;
static uint16_t screenWidth;
static uint16_t screenHeight;
static uint16_t screenData[480 * 480];
static uint32_t emuFeatures;
static bool     useJoystickAsMouse;
static float    touchCursorX;
static float    touchCursorY;


static void renderMouseCursor(int16_t screenX, int16_t screenY){
   if(screenHires){
      int8_t x;
      int8_t y;
      
      //align cursor to side of image
      screenX -= 6;
      
      for(y = 0; y < 32; y++)
         for(x = 6; x < 26; x++)
            if(screenX + x >= 0 && screenY + y >= 0 && screenX + x < screenWidth && screenY + y < screenHeight)
               if(cursor32x32[y * 32 + x] != 0xFFFF)
                  screenData[(screenY + y) * screenWidth + screenX + x] = cursor32x32[y * 32 + x];
   }
   else{
      int8_t x;
      int8_t y;
      
      //align cursor to side of image
      screenX -= 3;
      
      for(y = 0; y < 16; y++)
         for(x = 3; x < 13; x++)
            if(screenX + x >= 0 && screenY + y >= 0 && screenX + x < screenWidth && screenY + y < screenHeight)
               if(cursor16x16[y * 16 + x] != 0xFFFF)
                  screenData[(screenY + y) * screenWidth + screenX + x] = cursor16x16[y * 16 + x];
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
      emuFeatures = FEATURE_ACCURATE;
      
      var.key = "palm_emu_feature_ram_huge";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "enabled"))
            emuFeatures |= FEATURE_RAM_HUGE;
      
      var.key = "palm_emu_feature_fast_cpu";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "enabled"))
            emuFeatures |= FEATURE_FAST_CPU;
      
      var.key = "palm_emu_feature_hybrid_cpu";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "enabled"))
            emuFeatures |= FEATURE_HYBRID_CPU;
      
      var.key = "palm_emu_feature_custom_fb";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "enabled"))
            emuFeatures |= FEATURE_CUSTOM_FB;
      
      var.key = "palm_emu_feature_synced_rtc";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "enabled"))
            emuFeatures |= FEATURE_SYNCED_RTC;
      
      var.key = "palm_emu_feature_hle_apis";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "enabled"))
            emuFeatures |= FEATURE_HLE_APIS;
      
      var.key = "palm_emu_feature_emu_honest";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "enabled"))
            emuFeatures |= FEATURE_EMU_HONEST;
      
      var.key = "palm_emu_feature_ext_keys";
      if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
         if (!strcmp(var.value, "enabled"))
            emuFeatures |= FEATURE_EXT_KEYS;
   }

   var.key = "palm_emu_use_joystick_as_mouse";
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value){
      if (!strcmp(var.value, "enabled"))
         useJoystickAsMouse = true;
      else
         useJoystickAsMouse = false;
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
   info->library_version  = "v1/3*3" GIT_VERSION;
   info->need_fullpath    = false;
   info->valid_extensions = "rom";
}

void retro_get_system_av_info(struct retro_system_av_info *info){
   info->timing.fps = EMU_FPS;
   info->timing.sample_rate = AUDIO_SAMPLE_RATE;

   info->geometry.base_width   = 160;
   info->geometry.base_height  = 220;
   info->geometry.max_width    = 480;
   info->geometry.max_height   = 480;
   info->geometry.aspect_ratio = 160.0 / 220.0;
}

void retro_set_environment(retro_environment_t cb){
   struct retro_log_callback logging;
   struct retro_vfs_interface_info vfs_getter = { 1, NULL };
   struct retro_variable vars[] = {
      { "palm_emu_feature_ram_huge", "Extra RAM Hack; disabled|enabled" },
      { "palm_emu_feature_fast_cpu", "Overclock 2x; disabled|enabled" },
      { "palm_emu_feature_hybrid_cpu", "Extra RAM Hack; disabled|enabled" },
      { "palm_emu_feature_custom_fb", "Custom Resolution; disabled|enabled" },
      { "palm_emu_feature_synced_rtc", "Force Match System Clock; disabled|enabled" },
      { "palm_emu_feature_hle_apis", "HLE API Implementations; disabled|enabled" },
      { "palm_emu_feature_emu_honest", "Tell Programs They're In An Emulator(for test programs); disabled|enabled" },
      { "palm_emu_feature_ext_keys", "Enable OS5 Keys; disabled|enabled" },
      { "palm_emu_use_joystick_as_mouse", "Use Left Joystick As Mouse; disabled|enabled" },
      { 0 }
   };
   struct retro_input_descriptor input_desc[] = {
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, "Touchscreen Mouse X" },
      { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, "Touchscreen Mouse Y" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,     "Touchscreen Mouse Click" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Dpad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Dpad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Dpad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Dpad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "Dpad Middle" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Power" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Calender" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Address Book" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "Todo" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "Notes" },
      { 0 }
   };

   environ_cb = cb;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
   
   if(environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_getter))
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
   
   //touchscreen
   if(useJoystickAsMouse){
      int16_t x = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
      int16_t y = input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
      
      if(x < -JOYSTICK_DEADZONE || x > JOYSTICK_DEADZONE)
         touchCursorX += x * JOYSTICK_MULTIPLIER * (screenHires ? 2.0 : 1.0);
      
      if(y < -JOYSTICK_DEADZONE || y > JOYSTICK_DEADZONE)
         touchCursorY += y * JOYSTICK_MULTIPLIER * (screenHires ? 2.0 : 1.0);
      
      if(touchCursorX < 0)
         touchCursorX = 0;
      else if(touchCursorX > screenWidth - 1)
         touchCursorX = screenWidth - 1;
      
      if(touchCursorY < 0)
         touchCursorY = 0;
      else if(touchCursorY > screenHeight - 1)
         touchCursorY = screenHeight - 1;
      
      palmInput.touchscreenX = touchCursorX / (screenWidth - 1);
      palmInput.touchscreenY = touchCursorY / (screenHeight - 1);
      palmInput.touchscreenTouched = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2);
   }
   else{
      //use RetroArch internal pointer
      palmInput.touchscreenX = ((float)input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_X) / 0x7FFF + 1.0) / 2.0;
      palmInput.touchscreenY = ((float)input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_Y) / 0x7FFF + 1.0) / 2.0;
      palmInput.touchscreenTouched = input_state_cb(0, RETRO_DEVICE_POINTER, 0, RETRO_DEVICE_ID_POINTER_PRESSED);
   }

   //dpad
   palmInput.buttonUp = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
   palmInput.buttonRight = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
   palmInput.buttonDown = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
   palmInput.buttonLeft = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
   palmInput.buttonSelect = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
   
   //app buttons
   palmInput.buttonCalendar = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
   palmInput.buttonAddress = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
   palmInput.buttonTodo = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y);
   palmInput.buttonNotes = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
   
   //special buttons
   palmInput.buttonPower = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
   
   //run emulator
   emulatorRunFrame();
   memcpy(screenData, screenHires ? palmExtendedFramebuffer : palmFramebuffer, screenWidth * screenHeight * sizeof(uint16_t));
   
   //draw mouse
   if(useJoystickAsMouse)
      renderMouseCursor(touchCursorX, touchCursorY);
   
   video_cb(screenData, screenWidth, screenHeight, screenWidth * sizeof(uint16_t));
   audio_cb(palmAudio, AUDIO_SAMPLES_PER_FRAME);
}

bool retro_load_game(const struct retro_game_info *info){
   buffer_t rom;
   buffer_t bootloader;
   char bootloaderPath[PATH_MAX_LENGTH];
   char saveRamPath[PATH_MAX_LENGTH];
   struct RFILE* bootloaderFile;
   struct RFILE* saveRamFile;
   const char* systemDir;
   const char* saveDir;
   time_t rawTime;
   struct tm* timeInfo;
   uint32_t error;
   
   if(info == NULL)
      return false;
   
   environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &systemDir);
   environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &saveDir);
   
   rom.data = info->data;
   rom.size = info->size;
   
   //bootloader
   strlcpy(bootloaderPath, systemDir, PATH_MAX_LENGTH);
   strlcat(bootloaderPath, "/bootloader-en-m515.rom", PATH_MAX_LENGTH);
   bootloaderFile = filestream_open(bootloaderPath, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
   
   if(bootloaderFile){
      bootloader.size = filestream_get_size(bootloaderFile);
      bootloader.data = malloc(bootloader.size);
      
      if(bootloader.data)
         filestream_read(bootloaderFile, bootloader.data, bootloader.size);
      else
         bootloader.size = 0;
      filestream_close(bootloaderFile);
   }
   else{
      bootloader.data = NULL;
      bootloader.size = 0;
   }
   
   //updates the emulator configuration
   check_variables(true);
   
   error = emulatorInit(rom, bootloader, emuFeatures);
   if(error != EMU_ERROR_NONE)
      return false;
   
   if(bootloader.data)
      free(bootloader.data);
   
   //save RAM
   strlcpy(saveRamPath, saveDir, PATH_MAX_LENGTH);
   strlcat(saveRamPath, "/userdata-en-m515.ram", PATH_MAX_LENGTH);
   saveRamFile = filestream_open(saveRamPath, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
   
   if(saveRamFile){
      if(filestream_get_size(saveRamFile) == emulatorGetRamSize()){
         filestream_read(saveRamFile, palmRam, emulatorGetRamSize());
         swap16BufferIfLittle(palmRam, emulatorGetRamSize() / sizeof(uint16_t));
      }
      filestream_close(saveRamFile);
   }
   
   //set RTC
   time(&rawTime);
   timeInfo = localtime(&rawTime);
   emulatorSetRtc(timeInfo->tm_yday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
   
   screenHires = emuFeatures & FEATURE_CUSTOM_FB;
   screenWidth = screenHires ? 320 : 160;
   screenHeight = screenHires ? 440 : 220;
   touchCursorX = screenWidth / 2;
   touchCursorY = screenHeight / 2;

   return true;
}

void retro_unload_game(void){
   const char* saveDir;
   char saveRamPath[PATH_MAX_LENGTH];
   struct RFILE* saveRamFile;
   
   environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &saveDir);
   
   //save RAM
   strlcpy(saveRamPath, saveDir, PATH_MAX_LENGTH);
   strlcat(saveRamPath, "/userdata-en-m515.ram", PATH_MAX_LENGTH);
   saveRamFile = filestream_open(saveRamPath, RETRO_VFS_FILE_ACCESS_WRITE, RETRO_VFS_FILE_ACCESS_HINT_NONE);
   
   if(saveRamFile){
      swap16BufferIfLittle(palmRam, emulatorGetRamSize() / sizeof(uint16_t));//this will no longer be used, so its ok to destroy it when swapping
      filestream_write(saveRamFile, palmRam, emulatorGetRamSize());
      filestream_close(saveRamFile);
   }
   
   emulatorExit();
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
   buffer_t saveBuffer;
   
   saveBuffer.data = (uint8_t*)data;
   saveBuffer.size = size;
   
   return emulatorSaveState(saveBuffer);
}

bool retro_unserialize(const void *data, size_t size){
   buffer_t saveBuffer;
   
   saveBuffer.data = (uint8_t*)data;
   saveBuffer.size = size;
   
   return emulatorLoadState(saveBuffer);
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

