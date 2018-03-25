#include "libretro.h"
#include "libretro_params.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include <retro_miscellaneous.h>

#include "emulator.h"


#ifdef _WIN32
#define path_default_slash() "\\"
#else
#define path_default_slash() "/"
#endif


retro_log_printf_t log_cb;
retro_video_refresh_t video_cb;

#if 0
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
#endif

retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
   va_list va;

   (void)level;

   va_start(va, fmt);
   vfprintf(stderr, fmt, va);
   va_end(va);
}

void retro_init(void)
{
   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   
   if(environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565))
      log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");
}

void retro_deinit(void)
{
   
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = LR_CORENAME;
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = LR_LIBVERSION GIT_VERSION;
   info->need_fullpath    = LR_NEEDFILEPATH;
   info->valid_extensions = LR_VALIDFILEEXT; /* Anything is fine, we don't care. */
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps = LR_SCREENFPS;
   info->timing.sample_rate = LR_SOUNDRATE;

   info->geometry.base_width   = LR_SCREENWIDTH;
   info->geometry.base_height  = LR_SCREENHEIGHT;
   info->geometry.max_width    = LR_SCREENWIDTH;
   info->geometry.max_height   = LR_SCREENHEIGHT;
   info->geometry.aspect_ratio = LR_SCREENASPECT;
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_log_callback logging;
   bool no_rom = !LR_REQUIRESROM;

   environ_cb = cb;

   cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_rom);

   if (cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
      log_cb = logging.log;
   else
      log_cb = fallback_log;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
#if 0
   audio_cb = cb;
#endif
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
#if 0
   audio_batch_cb = cb;
#endif
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

void retro_reset(void)
{
   emulatorReset();
}

void retro_run(void)
{
   /*
   key_state_t ks;

   input_poll_cb();

   ks.up = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
   ks.right = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);
   ks.down = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
   ks.left = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
   ks.start = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START);
   ks.select = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

   */
   
   emulateFrame();
   video_cb(palmFramebuffer, LR_SCREENWIDTH, LR_SCREENHEIGHT, LR_SCREENWIDTH * sizeof(uint16_t));
}

bool retro_load_game(const struct retro_game_info *info)
{
   const char* systemDirectory;
   char palmRomPath[PATH_MAX_LENGTH];
   uint8_t palmRom[ROM_SIZE];
   FILE* romFile;
   size_t bytesRead;
   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "OS Menu" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Calender" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Address Book" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "ToDo" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Notes" },
      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
   
   environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &systemDirectory);
   strncpy(palmRomPath, systemDirectory, PATH_MAX_LENGTH);
   strncat(palmRomPath, path_default_slash(), PATH_MAX_LENGTH - strlen(palmRomPath));
   strncat(palmRomPath, "palmos41-en-m515.rom", PATH_MAX_LENGTH - strlen(palmRomPath));
   
   romFile = fopen(palmRomPath, "rb");
   
   if(romFile == NULL)
      return false;
   
   bytesRead = fread(palmRom, 1, ROM_SIZE, romFile);
   fclose(romFile);

   if(bytesRead == ROM_SIZE)
      emulatorInit(palmRom);
   else
      return false;
   
   time_t rawTime;
   struct tm* timeInfo;
   time(&rawTime);
   timeInfo = localtime(&rawTime);
   emulatorSetRtc(timeInfo->tm_yday, timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
   
   if(info != NULL){
      uint32_t prcSuccess = emulatorInstallPrcPdb(info->data, info->size);
      //pretend to pass for now
      /*
      if(prcSuccess != EMU_ERROR_NONE)
         return false;
      */
   }

   return true;
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return emulatorGetStateSize();
}

bool retro_serialize(void *data, size_t size)
{
   emulatorSaveState(data);
   return true;
}

bool retro_unserialize(const void *data, size_t size)
{
   emulatorLoadState(data);
   return true;
}

void *retro_get_memory_data(unsigned id)
{
   /*
   if (id != RETRO_MEMORY_SAVE_RAM)
      return NULL;

   return game_data();
   */
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   /*
   if (id != RETRO_MEMORY_SAVE_RAM)
      return 0;

   return game_data_size();
   */
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

