#ifndef IRDA_COMMANDS_HEADER
#define IRDA_COMMANDS_HEADER

#include <stdint.h>

/*commands are 64 bit big endian, 1 byte command/7 bytes data, the data is usualy a pointer*/
/*all access require 2 transfers, a request and an acknowledge, each device must take turns sending and receiving*/

#define IRDA_COMMAND_SIZE 8

/*the uint64_t casts are required since int is 16 bit on m68k and bit shifting is always performed at int length unless specified otherwise*/
#define IRDA_GET_COMMAND(ptr) ((uint64_t)((ptr)[0]) << 56 | (uint64_t)((ptr)[1]) << 48 | (uint64_t)((ptr)[2]) << 40 | (uint64_t)((ptr)[3]) << 32 | (uint64_t)((ptr)[4]) << 24 | (uint64_t)((ptr)[5]) << 16 | (uint64_t)((ptr)[6]) << 8 | (uint64_t)((ptr)[7]))
#define IRDA_SET_COMMAND(ptr, cmd) ((ptr)[0] = (cmd) >> 56, (ptr)[1] = (cmd) >> 48 & 0xFF, (ptr)[2] = (cmd) >> 40 & 0xFF, (ptr)[3] = (cmd) >> 32 & 0xFF, (ptr)[4] = (cmd) >> 24 & 0xFF, (ptr)[5] = (cmd) >> 16 & 0xFF, (ptr)[6] = (cmd) >> 8 & 0xFF, (ptr)[7] = (cmd) & 0xFF)
#define IRDA_MAKE_COMMAND(operation, data) ((uint64_t)(operation) << 56 | ((uint64_t)(data) & 0x00FFFFFFFFFFFFFFULL))
#define IRDA_GET_COMMAND_OPERATION(command) ((command) >> 56)
#define IRDA_GET_COMMAND_DATA(command) ((command) & 0x00FFFFFFFFFFFFFFULL)


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
   IRDA_COMMAND_SET_REGISTER,/*data is regNum << 32 | regValue */
   IRDA_COMMAND_RETURN,/*used to acknowledge the end of a command, returns a value if the command returns data*/
   IRDA_COMMAND_ALLOCATE_BUFFER,/*returns a pointer to the buffer or NULL on failure*/
   IRDA_COMMAND_SET_BUFFER,/*data is size << 32 | pointer*/
   IRDA_COMMAND_BUFFER_CHUNK,/*contains 7 bytes of the buffer data*/
   IRDA_COMMAND_CLOSE_CONNECTION
};

#endif
