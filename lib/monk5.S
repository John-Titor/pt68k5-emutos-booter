/*
 * MONK5 functions.
 */

    .text
    .align  2

// void putc(int c)
//
    .globl  putc
    .type   putc,@function

putc:
    move.w  4(%sp),%d0
    cmp.b   #'\n',%d0
    bne     1f
    move.b  #'\r',%d0
    trap    #2
    move.w  4(%sp),%d0
1:
    trap    #2
    rts

    .size   putc, .-putc


// Video functions
//
// uint32_t trap12(uint8_t op, uint8_t arg1, uint8_t arg2, void *addr)
//
    .globl  trap12
    .type   trap12,@function

trap12:
    move.l  %d7,-(%sp)              // save d7
    move.l  8(%sp),%d7              // get opcode
    move.l  12(%sp),%d0             // get args
    move.l  16(%sp),%d1
    move.l  20(%sp),%a1
    trap    #12                     // call MONK5
    move.l  %d7,%d0                 // return value in d7
    move.l  (%sp)+,%d7              // restore d7
    rts

    .size trap12, .-trap12
