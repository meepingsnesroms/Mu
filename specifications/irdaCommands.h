#ifndef IRDA_COMMANDS_HEADER
#define IRDA_COMMANDS_HEADER

#include <stdint.h>

/*commands are big endian*/
/*all access require 2 transfers, a request and an acknowledge, each device must take turns sending and receiving*/

enum{
   IRDA_COMMAND_NONE = 0,
   IRDA_COMMAND_GET_BYTE,
   IRDA_COMMAND_GET_WORD,
   IRDA_COMMAND_GET_LONG,
   IRDA_COMMAND_SET_BYTE,/*(uint32_t)pointer << 8 | (uint8_t)value*/
   IRDA_COMMAND_SET_WORD,/*(uint32_t)pointer << 16 | (uint16_t)value*/
   IRDA_COMMAND_SET_LONG,/*(uint32_t)pointer << 32 | (uint32_t)value*/
   IRDA_COMMAND_CALL_TRAP,/*(uint16_t)trapNum*/
   IRDA_COMMAND_GET_REGISTER,
   IRDA_COMMAND_SET_REGISTER,/*data is (uint8_t)regNum << 32 | (uint32_t)regValue */
   IRDA_COMMAND_RETURN,/*used to acknowledge the end of a command, returns a value if the command returns data*/
   IRDA_COMMAND_ALLOCATE_BUFFER,/*returns a pointer to the buffer or NULL on failure*/
   IRDA_COMMAND_SET_BUFFER,/*(uint32_t)pointer << 32 | (uint32_t)size then raw data*/
   IRDA_COMMAND_GET_BUFFER,/*(uint32_t)pointer << 32 | (uint32_t)size*/
   IRDA_COMMAND_GET_FRAMEBUFFER,/*all execution is halted until this is finished, returns (uint16_t)width << 24 | (uint16_t)height << 8 | (uint8_t)bpp then raw data*/
   IRDA_COMMAND_CLOSE_CONNECTION
};

#endif
