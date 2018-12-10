#ifndef SD_CARD_COMMAND_SPEC_HEADER
#define SD_CARD_COMMAND_SPEC_HEADER
/*SD Card Commands*/
/*Command Format:01IIIIIIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACCCCCCC1*/
/*48 bits, I = index, A = argument, C = CRC*/
/*see http://elm-chan.org/docs/mmc/mmc_e.html for more command information*/

#define GO_IDLE_STATE        0/*software reset*/
#define SEND_OP_COND         1/*initiate initialization process*/
#define SEND_CSD             9/*read CSD register*/
#define SEND_CID             10/*read CID register*/
#define STOP_TRANSMISSION    12/*stop to read data*/
#define SET_BLOCKLEN         16/*change R/W block size*/
#define READ_SINGLE_BLOCK    17/*read a block*/
#define READ_MULTIPLE_BLOCK  18/*read multiple blocks*/
#define WRITE_SINGLE_BLOCK   24/*write a block*/
#define WRITE_MULTIPLE_BLOCK 25/*write multiple blocks*/
#define READ_OCR             58/*read OCR(operation condtion register)*/

#endif
