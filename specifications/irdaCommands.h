#ifndef IRDA_COMMANDS_HEADER
#define IRDA_COMMANDS_HEADER

#include <stdint.h>

/*commands are 64 bits, 32 command/32 data, the data is usualy a pointer*/
/*all access require 2 transfers, a request and an acknowledge, each device must take turns sending and receiving*/

typedef struct{
   uint32_t command;
   uint32_t data;
}irda_command_t;

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
