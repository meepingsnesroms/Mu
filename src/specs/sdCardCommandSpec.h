#ifndef SD_CARD_COMMAND_SPEC_HEADER
#define SD_CARD_COMMAND_SPEC_HEADER
/*SD Card Commands*/
/*Command Format:01IIIIIIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACCCCCCC1*/
/*48 bits, I = index, A = argument, C = CRC*/
/*see http://elm-chan.org/docs/mmc/mmc_e.html for more command information*/

#define GO_IDLE_STATE        0/*software reset*/
#define SEND_OP_COND         1/*initiate initialization process*/
#define SEND_IF_COND         8/*only for SDC V2, checks voltage range.*/
#define SEND_CSD             9/*read CSD register*/
#define SEND_CID             10/*read CID register*/
#define STOP_TRANSMISSION    12/*stop to read data*/
#define SET_BLOCKLEN         16/*change R/W block size*/
#define READ_SINGLE_BLOCK    17/*read a block*/
#define READ_MULTIPLE_BLOCK  18/*read multiple blocks*/
#define SET_BLOCK_COUNT      23/*only for MMC, defines number of blocks to transfer with next multi-block read/write command*/
#define WRITE_SINGLE_BLOCK   24/*write a block*/
#define WRITE_MULTIPLE_BLOCK 25/*write multiple blocks*/
#define APP_CMD              55/*next command is ACMD<n> command*/
#define READ_OCR             58/*read OCR(operation condtion register)*/

/*Application Commands*/
#define APP_SEND_OP_COND         41/*only for SDC, same as SEND_OP_COND*/
#define SET_WR_BLOCK_ERASE_COUNT 23/*only for SDC, defines number of blocks to pre-erase with next multi-block write command*/

/*R1 Response Bits*/
#define IN_IDLE_STATE        0x01
#define ERASE_RESET          0x02
#define ILLEGAL_COMMAND      0x04
#define COMMAND_CRC_ERROR    0x08
#define ERASE_SEQUENCE_ERROR 0x10
#define ADDRESS_ERROR        0x20
#define PARAMETER_ERROR      0x40

/*Data Transfer Tokens*/
#define DATA_TOKEN_DEFAULT 0xFE
#define DATA_TOKEN_CMD25   0xFC
#define STOP_TRAN          0xFD

#endif
