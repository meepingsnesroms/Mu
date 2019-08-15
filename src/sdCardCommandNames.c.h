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
#define SEND_STATUS          13/*asks the card to send its status register*/
#define SET_BLOCKLEN         16/*change R/W block size*/
#define READ_SINGLE_BLOCK    17/*read a block*/
#define READ_MULTIPLE_BLOCK  18/*read multiple blocks*/
#define SET_BLOCK_COUNT      23/*only for MMC, defines number of blocks to transfer with next multi-block read/write command*/
#define WRITE_SINGLE_BLOCK   24/*write a block*/
#define WRITE_MULTIPLE_BLOCK 25/*write multiple blocks*/
#define SEND_WRITE_PROT      30/*if the card has write protection features this command asks the card to send the status of the write protection bits*/
#define APP_CMD              55/*next command is ACMD<n> command*/
#define READ_OCR             58/*read OCR(Operation Condtion Register)*/
#define CRC_ON_OFF           59/*turns the CRC option on or off, a 1 in the CRC option bit will turn the option on, a 0 will turn it off*/

/*Application Commands*/
#define SET_WR_BLOCK_ERASE_COUNT 23/*only for SDC, defines number of blocks to pre-erase with next multi-block write command*/
#define APP_SEND_OP_COND         41/*only for SDC, same as SEND_OP_COND*/
#define SEND_SCR                 51/*reads the SCR(SD Configuration Register)*/

/*R1 Response Bits*/
#define R1_IN_IDLE_STATE        0x01
#define R1_ERASE_RESET          0x02
#define R1_ILLEGAL_COMMAND      0x04
#define R1_COMMAND_CRC_ERROR    0x08
#define R1_ERASE_SEQUENCE_ERROR 0x10
#define R1_ADDRESS_ERROR        0x20
#define R1_PARAMETER_ERROR      0x40

/*R2 Response Bits*/
#define R2_CARD_IS_LOCKED           0x01
#define R2_WRITE_PROTECT_ERASE_SKIP 0x02
#define R2_ERROR                    0x04
#define R2_CC_ERROR                 0x08
#define R2_CARD_ECC_FAILED          0x10
#define R2_WRITE_PROTECT_VIOLATION  0x20
#define R2_ERASE_PARAM              0x40
#define R2_OUT_OF_RANGE             0x80/*also called R2_CSD_OVERWRITE*/

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
