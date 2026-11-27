.text
.align 4
.global strcat
.type strcat, %function

/*
char *strcat(char *destination, const char *source)
    r0: dest pointer (preserved for return value)
    r1: src pointer
    returns: original dest pointer in r0
*/
strcat:
    mov     r2, r0                  /* working pointer r2 = dest */
strcat_find_end:
    ldrb    r3, [r2], #1            /* scan dest byte-by-byte, then r2++ */
    cmp     r3, #0
    bne     strcat_find_end         /* keep scanning until '\0' */
    sub     r2, r2, #1              /* back up 1 byte so r2 points at dest '\0' */
strcat_copy:
    ldrb    r3, [r1], #1            /* load byte *src into r3, then src++ */
    strb    r3, [r2], #1            /* write byte at *r2, then r2++ */
    cmp     r3, #0
    bne     strcat_copy             /* if not '\0', keep appending */
    bx      lr                      /* return original dest in r0 */