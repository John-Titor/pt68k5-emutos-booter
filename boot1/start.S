/*
 * Boot1 startup.
 */

    .set    hd_unit,0xfff           // MONK5 current fixed disk unit number

    .text
    .align  2

    .global start
start:
    // move the user stack away from the potential load area
    lea     start-8(%pc),%sp        // nothing just below us in RAM
    moveq   #9,%d0                  // invalidate and enable
    movec   %d0,%cacr               // ... the instruction cache

    // call the disk loader
    clr.l   %d0
    move.b  hd_unit,%d0
    move.l  %d0,-(%sp)
    move.l  %a2,-(%sp)              // MONK5 current fixed disk base address
    bsr     boot_main               // go boot something
    trap    #0                      // failed, back to monitor


