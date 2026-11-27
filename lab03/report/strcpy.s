.text
.align 4
.global strcpy
.type strcpy, %function

/*
char *strcpy(char *destination, const char *source)
    r0: dest pointer (preserved for return value)
    r1: src pointer
    returns: original dest pointer in r0
*/
strcpy:
    mov     r2, r0                  /* working write pointer r2 = dest */
strcpy_loop:
    ldrb    r3, [r1], #1            /* load byte *src into r3, then src++ */
    strb    r3, [r2], #1            /* store byte into *r2, then r2++ */
    cmp     r3, #0
    bne     strcpy_loop             /* if not '\0', copy next byte */
    bx      lr                      /* return original dest in r0 */