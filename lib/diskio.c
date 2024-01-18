/*-----------------------------------------------------------------------*/
/* Low level disk I/O module for PT68K5, from:                           */
/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include <lib.h>

#include <fatfs/ff.h>		/* Obtains integer types */
#include <fatfs/diskio.h>	/* Declarations of disk functions */

/* Definitions of physical drive number for each drive */
#define XTIDE_PRIMARY	0
#define XTIDE_SECONDARY	1
#define IDE_PRIMARY		2
#define IDE_SECONDARY	3

/* try the first valid partition on each drive */
PARTITION VolToPart[FF_VOLUMES] = {
	{ XTIDE_PRIMARY, 0},
	{ XTIDE_SECONDARY, 0},
	{ IDE_PRIMARY, 0},
	{ IDE_SECONDARY, 0},
};

static const uint32_t xtide_base = 0x10000300;
static const uint32_t ide_base = 0x20004180;

#define REG8(_x)    *(volatile uint8_t *)(_x)
#define REG16(_x)  *(volatile uint16_t *)(_x)
#define IDE_DATA16              REG16(iobase + 0x00)
#define IDE_DATA8               REG8(iobase + 0x00)
#define IDE_ERROR               REG8(iobase + 0x03)
#define IDE_ERROR_ID_NOT_FOUND      0x10
#define IDE_ERROR_UNCORRECTABLE     0x40
#define IDE_FEATURE             REG8(iobase + 0x03)
#define IDE_SECTOR_COUNT        REG8(iobase + 0x05)
#define IDE_LBA_0               REG8(iobase + 0x07)
#define IDE_LBA_1               REG8(iobase + 0x09)
#define IDE_LBA_2               REG8(iobase + 0x0b)
#define IDE_LBA_3               REG8(iobase + 0x0d)
#define IDE_LBA_3_DEV1              0x10
#define IDE_LBA_3_LBA               0xe0    /* incl. bits 7/5 for compat */
#define IDE_STATUS              REG8(iobase + 0x0f)
#define IDE_STATUS_ERR              0x01
#define IDE_STATUS_DRQ              0x08
#define IDE_STATUS_DF               0x20
#define IDE_STATUS_DRDY             0x40
#define IDE_STATUS_BSY              0x80
#define IDE_COMMAND             REG8(iobase + 0x0f)
#define IDE_CMD_NOP                 0x00
#define IDE_CMD_READ_SECTORS        0x20
#define IDE_CMD_WRITE_SECTORS       0x30
#define IDE_CMD_IDENTIFY_DEVICE     0xec

#define DISK_BLOCK_SIZE				512

bool diskio_swap;

static DRESULT
disk_read_sector(BYTE *buff, LBA_t sector, uint32_t iobase, uint8_t ssel)
{
	uint32_t   timeout = 0x200000;

	IDE_LBA_3 = ((sector >> 24) & 0x3f) | IDE_LBA_3_LBA | ssel;
	IDE_LBA_2 = (sector >> 16) & 0xff;
	IDE_LBA_1 = (sector >> 8) & 0xff;
	IDE_LBA_0 = sector & 0xff;
	IDE_SECTOR_COUNT = 1;
	IDE_COMMAND = IDE_CMD_READ_SECTORS;

	while (--timeout) {
	    uint8_t status = IDE_STATUS;

	    if (status & IDE_STATUS_BSY) {
	        continue;
	    }
	    if (status & IDE_STATUS_ERR) {
	        fmt("error 0x%x reading 0x%lx\n", IDE_ERROR, sector);
	        return -1;
	    }
	    if (status & IDE_STATUS_DRQ) {
	        uint16_t *bp = (uint16_t *)buff;
	        for (unsigned idx = 0; idx < DISK_BLOCK_SIZE; idx += 2) {
	            if (diskio_swap) {
	                *bp++ = __builtin_bswap16(IDE_DATA16);
	            } else {
	                *bp++ = IDE_DATA16;
	            }
	        }
	        return RES_OK;
	    }
	}
	return RES_ERROR;
}


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	/* always re-init */
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	/* init does not require anything special */
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	uint32_t iobase;
	uint8_t  ssel;
	DRESULT res;

	switch (pdrv) {
	case XTIDE_PRIMARY:
		iobase = xtide_base;
		ssel = 0;
		break;
	case XTIDE_SECONDARY:
		iobase = xtide_base;
		ssel = IDE_LBA_3_DEV1;
		break;
	case IDE_PRIMARY:
		iobase = ide_base;
		ssel = 0;
		break;
	case IDE_SECONDARY:
		iobase = ide_base;
		ssel = IDE_LBA_3_DEV1;
		break;
	default:
		return RES_PARERR;
	}

	while (count) {
		res = disk_read_sector(buff, sector, iobase, ssel);
		if (res != RES_OK) {
			return res;
		}
		buff += DISK_BLOCK_SIZE;
		sector += 1;
		count -= 1;
	}

	return RES_OK;
}
