/*
 * boot1.c
 *
 * Find a bootable partition with a usable filesystem and load either
 * BOOTK5.SYS (preferred) or EMUTOSK5.SYS and chain to them.
 */

#include <stdint.h>
#include <lib.h>
#include <fatfs/ff.h>
#include <fatfs/diskio.h>

#define DEBUG 0

typedef struct {
    uint16_t    bra;
    uint16_t    tos_version;
    uint32_t    main;
    uint32_t    os_beg;
    uint32_t    os_end;
    uint32_t    pad0[7];
    uint8_t     etos_id[4];
} emutos_hdr;

/*
 * Do video init, return base address of display memory.
 *
 * VGA mode 12 yields a 640x480x4 display with plane 0
 * at 0x080a_0000. By configuring the palette appropriately,
 * we can use the first plane (LSB) to select one of two 
 * colours, effectively yielding a linear monochrome framebuffer.
 */
static void
video_init(void)
{
    /* standard Atari black-on-white palette */
    static const uint8_t palette[16] = {
        0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    trap12(0x80, 0, 0, 0);              /* screen off */
    trap12(0x12, 0, 0, 0);              /* VGA mode */
    trap12(0x85, 0, 0, &palette[0]);    /* load palette */
    trap12(0x89, 0, 0, 0);              /* clear video memory */
    trap12(0x81, 0, 0, 0);              /* screen on */
    trap12(0x90, 5, 5, 0);              /* position cursor */
    trap12(0x91, 15, 0, "EmuTOS loading...");
}

/*
 * Validate the EmuTOS header on the currently open file, return the load address.
 */
static uint8_t *
emutos_header_check(FIL *fil)
{
    emutos_hdr hdr;
    UINT len = sizeof(hdr);

    if ((f_size(fil) < (64 * 1024)) ||
        (f_size(fil) > (1024 * 1024)) ||
        (f_lseek(fil, 0) != RES_OK) ||
        (f_read(fil, &hdr, len, &len) != RES_OK) ||
        (len != sizeof(hdr)) ||
        (hdr.etos_id[0] != 'E') ||
        (hdr.etos_id[1] != 'T') ||
        (hdr.etos_id[2] != 'O') ||
        (hdr.etos_id[3] != 'S') ||
        (hdr.os_beg < 0x800) ||
        (hdr.os_beg > hdr.os_end) ||
        (hdr.main < hdr.os_beg) ||
        (hdr.main >= hdr.os_end) ||
        (hdr.os_end >= 0x00100000)) {
        fmt("Bad EmuTOS image\n");
        return 0;
    }
    return (uint8_t *)hdr.os_beg;
}

/*
 * Load an opened second-stage image file to its runtime address.
 */
static uint32_t
boot2_load(FIL *fil)
{
    BYTE *load_buffer = (BYTE *)0x2000;
    UINT len = f_size(fil);

    if (len < 1024) {
        fmt("Second-stage bootloader suspiciously small.\n");
        return 0;
    }
    if ((f_read(fil, load_buffer, len, &len) != RES_OK) ||
        len != f_size(fil)) {
        return 0;
    }
    return (uint32_t)load_buffer;
}


/*
 * Load an opened EmuTOS image file to its runtime address.
 */
static uint32_t
emutos_load(FIL *fil)
{
    uint8_t *load_buffer = emutos_header_check(fil);
    const emutos_hdr *eh = (emutos_hdr *)load_buffer;
    UINT len = f_size(fil);

    if (load_buffer == 0) {
        return 0;
    }
    fmt("loading @ %p\n", load_buffer);
    f_lseek(fil, 0);
    if ((f_read(fil, load_buffer, len, &len) != RES_OK) ||
        (len != f_size(fil))) {
        return 0;
    }

    return eh->main;
}

/*
 * Try to boot from the given drive.
 */
static void
try_boot(const char *drive)
{
    uint32_t entry;
    FATFS fs;
    FIL fil;

    if (f_mount(&fs, drive, 1) != RES_OK) {
        fmt("Failed to find a partition on %s\n", drive);
        return;
    }
    if (f_chdrive(drive) != RES_OK) {
        fmt("Failed to set default drive to %s\n", drive);
    } else if (f_open(&fil, "/BOOTK5.SYS", FA_READ) == RES_OK) {
        if ((entry = boot2_load(&fil)) == 0) {
            fmt("Error loading %s/BOOTK5.SYS");
        } else {
            jump_to_loaded_os(entry);
        }
    } else if (f_open(&fil, "/EMUTOSK5.SYS", FA_READ) == RES_OK) {
        if ((entry = emutos_load(&fil)) == 0) {
            fmt("Error loading %s/EMUTOSK5.SYS\n");
        } else {
            uint32_t * const _memctrl       = (uint32_t *)0x424;
            uint32_t * const _resvalid      = (uint32_t *)0x426;
            uint32_t * const _phystop       = (uint32_t *)0x42e;
            uint32_t * const _ramtop        = (uint32_t *)0x5a4;
            uint32_t * const _memvalid      = (uint32_t *)0x420;
            uint32_t * const _memval2       = (uint32_t *)0x43a;
            uint32_t * const _memval3       = (uint32_t *)0x51a;
            uint32_t * const _ramvalid      = (uint32_t *)0x5a8;
            uint32_t * const _warm_magic    = (uint32_t *)0x6fc;
            uint32_t ram_size;

            /* simple RAM probing */
            for (ram_size = 0x00400000; ram_size <= 0x08000000; ram_size += 0x00100000) {
                fmt("\rRAM size %uMiB", (unsigned int)(ram_size >> 20));
                if (!probe(ram_size)) {
                    break;
                }
            }
            puts("");

            fmt("Booting @ 0x%lx\n\n", entry);
            *_memctrl = 0;              /* zero memctrl since we don't conform */
            *_resvalid = 0;             /* prevent reset vector being called */
            *_phystop = ram_size;       /* ST memory size */
            *_ramtop = 0;               /* no TT memory */
            *_memvalid = 0x752019f3;    /* set magic numbers to validate memory config */
            *_memval2 = 0x237698aa;
            *_memval3 = 0x5555aaaa;
            *_ramvalid = 0x1357bd13;
            *_warm_magic = 0;           /* this is a first/cold boot */

            video_init();               /* assume we have a video card */
            jump_to_loaded_os(entry);
        }
    }

    /* no good, unmount */
    f_unmount(drive);
}

void
boot_main(uint32_t base_addr, uint32_t slave)
{
    fmt("\nPT68K5 EmuTOS boot1\n");

    switch (base_addr) {
    case 0x10000300:
        if (slave) {
            try_boot("2:");
        } else {
            try_boot("1:");
        }
        break;
    case 0x20004180:
        if (slave) {
            try_boot("4:");
        } else {
            try_boot("3:");
        }
        break;
    default:
        fmt("XTIDE master:\n");
        try_boot("1:");
        fmt("XTIDE slave:\n");
        try_boot("2:");
        fmt("IDE master:\n");
        try_boot("3:");
        fmt("IDE slave:\n");
        try_boot("4:");
        fmt("nothing bootable\n");
    }
}
