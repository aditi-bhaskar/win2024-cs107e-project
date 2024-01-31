/* File: larson.s
 * --------------
 * TODO: add your file header comment here
 */

/*
 * The code below is the blink program from Lab 1.
 * Modify the code to implement the larson scanner for Assignment 1.
 * Be sure to use GPIO pins PB0-PB3 (or PB0-PB7) for your LEDs.
 */

    lui     a0,0x2000       # a0 holds base addr PB group = 0x2000000
    addi    a1,zero,1       # a1 holds constant 1
    sw      a1,0x30(a0)     # config PB0 as output

 loop:
    xori    a1,a1,1         # xor ^ 1 invert a1
    sw      a1,0x40(a0)     # set data value of PB0 to a1

    lui     a2,0x4500       # a2 = init countdown value
 delay:
    addi    a2,a2,-1        # decrement a2
    bne     a2,zero,delay   # keep counting down until a2 is zero

    j       loop            # back to top of outer loop