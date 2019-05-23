#include "ff.h"
#include "diskio.h"

#include <stdbool.h>
#include <string.h>
#include "../../emulator.h"


#define DEV_RAM		0


static bool cardInitalized = false;


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
   if(pdrv == DEV_RAM)
      return cardInitalized ? 0x00 : STA_NOINIT;

   return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
   if(pdrv == DEV_RAM){
      cardInitalized = true;
      return 0x00;
   }

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
   if(pdrv == DEV_RAM){
      if(!(sector * SD_CARD_BLOCK_SIZE + SD_CARD_BLOCK_SIZE <= palmSdCard.flashChipSize))
         return RES_ERROR;

      memcpy(buff, palmSdCard.flashChipData + sector * SD_CARD_BLOCK_SIZE, count * SD_CARD_BLOCK_SIZE);
      return RES_OK;
   }

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
   if(pdrv == DEV_RAM){
      if(!(sector * SD_CARD_BLOCK_SIZE + SD_CARD_BLOCK_SIZE <= palmSdCard.flashChipSize))
         return RES_ERROR;

      memcpy(palmSdCard.flashChipData + sector * SD_CARD_BLOCK_SIZE, buff, count * SD_CARD_BLOCK_SIZE);
      return RES_OK;
   }

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
   if(pdrv == DEV_RAM){
      switch(cmd){
         case GET_BLOCK_SIZE:
         case GET_SECTOR_SIZE:
            *((uint32_t*)buff) = SD_CARD_BLOCK_SIZE;
            return RES_OK;

         case GET_SECTOR_COUNT:
            *((uint32_t*)buff) = palmSdCard.flashChipSize / SD_CARD_BLOCK_SIZE;
            return RES_OK;

         case CTRL_SYNC:
            //do nothing
            return RES_OK;

         default:
            return RES_PARERR;
      }
   }

	return RES_PARERR;
}

