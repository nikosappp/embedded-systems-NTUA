.text
.align 4
.global strcmp
.type strcmp, %function

/*
int strcmp(const char *str1, const char *str2)
    r0: str1 pointer
    r1: str2 pointer
    returns (r0): <0 if first char that doesn't match has lower val in ptr1 than ptr2
                   0 if str1 == str2
                  >0 if first char that doesn't match has greater val in ptr1 than ptr2
*/
strcmp:
strcmp_loop:
    ldrb    r2, [r0], #1            /* load unsigned byte *str1 into r2, then str1++ */
    ldrb    r3, [r1], #1            /* load unsigned byte *str2 into r3, then str2++ */
    cmp     r2, r3
    bne     strcmp_done             /* if first mismatch found, then compute difference */
    cmp     r2, #0
    bne     strcmp_loop             /* if not at end of string, then compare next byte */
strcmp_done:
    sub     r0, r2, r3              /* return (unsigned char)*str1 - (unsigned char)*str2 */
    bx      lr                      /* return signed result in r0 */