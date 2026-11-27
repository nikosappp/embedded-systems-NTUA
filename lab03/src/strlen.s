.text
.align 4
.global strlen
.type strlen, %function

/*
size_t strlen(const char *str)
    r0: pointer to '\0'-terminated string
    returns: number of characters before '\0' in r0
*/
strlen:
    add     r1, r0, #1              /* r1 = start + 1 (accounts for final post-increment) */
strlen_loop:
    ldrb    r2, [r0], #1            /* load unsigned byte *r0 into r2, then r0++ */
    cmp     r2, #0                  /* reached NUL terminator '\0'? */
    bne     strlen_loop             /* if not '\0', keep scanning */
    sub     r0, r0, r1              /* length = (ptr_past_NUL) - (start + 1) */
    bx      lr                      /* return length in r0 */