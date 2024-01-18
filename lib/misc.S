/*
 * Misc assembly functions.
 */

    .text
    .align  2

// void jump_to_loaded_os(uint32_t entrypoint)
//
    .globl  jump_to_loaded_os
    .type   jump_to_loaded_os,@function

jump_to_loaded_os:
    move.l  4(%sp),%a1              // get entrypoint
    lea     am_super(%pc),%a0       // continuation address
    move.l  %a0,0xbc                // trap #15 vector
    trap    #15                     // go to supervisor mode
am_super:
    moveq   #8,%d0                  // invalidate and disable the instruction cache
    movec   %d0,%cacr
    jmp     (%a1)                   // call entrypoint

    .size   jump_to_loaded_os, .-jump_to_loaded_os


// int probe(uint32_t address)
//
    .globl  probe
    .type   probe,@function
probe:
    move.l  4(%sp),%d0
    move.l  %d0,%a0
    move.l  %d0,(%a0)
    not     %d0
    move.l  %d0,4(%a0)
    not     %d0
    cmp.l   (%a0),%d0
    bne     1f
    not     %d0
    cmp.l   4(%a0),%d0
    bne     1f
    move.w  1,%d0
    rts
1:
    clr.w   %d0
    rts

    .size   probe, .-probe
