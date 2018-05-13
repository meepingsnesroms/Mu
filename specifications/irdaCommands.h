#ifndef IRDA_COMMANDS_HEADER
#define IRDA_COMMANDS_HEADER

#include <stdint.h>

/*commands are 64 bit big endian, 32 command/32 data, the data is usualy a pointer*/
/*all access require 2 transfers, a request and an acknowledge, each device must take turns sending and receiving*/

#define IRDA_COMMAND_SIZE 8

/*the uint64_t casts are required since int is 16 bit on m68k and bit shifting is always performed at int length unless specified otherwise*/
#define IRDA_GET_COMMAND(ptr) ((uint64_t)ptr[0] << 56 | (uint64_t)ptr[1] << 48 | (uint64_t)ptr[2] << 40 | (uint64_t)ptr[3] << 32 | (uint64_t)ptr[4] << 24 | (uint64_t)ptr[5] << 16 | (uint64_t)ptr[6] << 8 | (uint64_t)ptr[7])

enum{
   IRDA_COMMAND_NONE = 0,
   IRDA_COMMAND_GET_BYTE,
   IRDA_COMMAND_GET_WORD,
   IRDA_COMMAND_GET_LONG,
   IRDA_COMMAND_SET_BYTE,
   IRDA_COMMAND_SET_WORD,
   IRDA_COMMAND_SET_LONG,
   IRDA_COMMAND_CALL_TRAP,
   IRDA_COMMAND_GET_REGISTER,
   IRDA_COMMAND_SET_REGISTER,
   IRDA_COMMAND_RETURN_BYTE,
   IRDA_COMMAND_RETURN_WORD,
   IRDA_COMMAND_RETURN_LONG,
   IRDA_COMMAND_ACKNOWLEDGE,/*used if no value needs to be returned*/
   IRDA_COMMAND_CLOSE_CONNECTION
};

#endif
