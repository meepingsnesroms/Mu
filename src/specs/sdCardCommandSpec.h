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
#define SEND_WRITE_PROT      30/*if the card has write protection features this command asks the card to send the status of the write protection bits*/
#define APP_CMD              55/*next command is ACMD<n> command*/
#define READ_OCR             58/*read OCR(Operation Condtion Register)*/

/*Application Commands*/
#define SET_WR_BLOCK_ERASE_COUNT 23/*only for SDC, defines number of blocks to pre-erase with next multi-block write command*/
#define APP_SEND_OP_COND         41/*only for SDC, same as SEND_OP_COND*/
#define SEND_SCR                 51/*reads the SCR(SD Configuration Register)*/

/*R1 Response Bits*/
#define IN_IDLE_STATE        0x01
#define ERASE_RESET          0x02
#define ILLEGAL_COMMAND      0x04
#define COMMAND_CRC_ERROR    0x08
#define ERASE_SEQUENCE_ERROR 0x10
#define ADDRESS_ERROR        0x20
#define PARAMETER_ERROR      0x40

/*Error Token Bits*/
#define ET_ERROR           0x01
#define ET_CC_ERROR        0x02
#define ET_CARD_ECC_FAILED 0x04
#define ET_OUT_OF_RANGE    0x08
#define ET_CARD_IS_LOCKED  0x10

/*Data Transfer Tokens*/
#define DATA_TOKEN_DEFAULT 0xFE
#define DATA_TOKEN_CMD25   0xFC
#define STOP_TRAN          0xFD

/*Data Response Bits*/
#define DR_ACCEPTED    0x05
#define DR_CRC_ERROR   0x0B
#define DR_WRITE_ERROR 0x0D

#endif
