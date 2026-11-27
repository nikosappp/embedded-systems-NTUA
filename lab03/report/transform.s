.data
prompt_str: .asciz "Input a string of up to 32 chars long: "
fmt_read:   .asciz "%32[^\n]"           /* read up to 32 chars until '\n' */
fmt_flush:  .asciz "%*[^\n]"            /* discard any leftover chars past 32 up to '\n' */
result_str: .asciz "Result is: %s\n\n"
exit_str:   .asciz "Exiting....\n"

.bss
.align 4                                /* align next address to a 16-byte boundary */
buf:        .space 36                   /* 32 chars + '\0' + 3 bytes alignment padding */

.text
.align 4
.global main
.type main, %function

/* main function */
main:
    push    {r4, lr}                    /* save r4 and return address (8-byte stack alignment) */
    ldr     r4, =buf                    /* r4 = &buf[0] - r4 callee-saved, preserved across bl calls */

input_loop:
    mov     r0, #0
    strb    r0, [r4]                    /* buf[0] = '\0' in case user just presses Enter */

    ldr     r0, =prompt_str             /* r0 = "Input a string of up to 32 chars long: " */
    bl      printf

    ldr     r0, =fmt_read               /* r0 = "%32[^\n]" */
    mov     r1, r4                      /* r1 = &buf[0] */
    bl      scanf
    cmp     r0, #-1                     /* did scanf hit EOF / Ctrl+D? */
    beq     do_exit

    /* Flush any extra characters (> 32) and '\n' from stdin */
    ldr     r0, =fmt_flush              /* r0 = "%*[^\n]" */
    bl      scanf
    bl      getchar                     /* consume '\n' */

    /* Check exit condition: (length == 1 && (buf[0] == 'Q' || buf[0] == 'q')) */
    ldrb    r0, [r4]                    /* r0 = buf[0] */
    ldrb    r1, [r4, #1]                /* r1 = buf[1] */
    cmp     r1, #0                      /* is buf[1] == '\0' (string length == 1)? */
    bne     run_transform               /* if not, run string transformation */
    cmp     r0, #'Q'
    beq     do_exit
    cmp     r0, #'q'
    beq     do_exit                     /* if q or Q, then exit */

run_transform:
    mov     r0, r4                      /* pass &buf[0] in r0 to transform_str */
    bl      transform_str

    ldr     r0, =result_str             /* r0 = "Result is: %s\n\n" */
    mov     r1, r4                      /* r1 = transformed string */
    bl      printf
    b       input_loop                  /* read next string */

do_exit:
    ldr     r0, =exit_str               /* r0 = "Exiting....\n" */
    bl      printf
    mov     r0, #0                      /* return 0 */
    pop     {r4, pc}                    /* restore r4 and return from main */


/* string transformation function
    void transform_str(char *s)
    r0: pointer to current character in string
    r1: current character byte */
.align 4
.global transform_str
.type transform_str, %function

transform_str:
    ldrb    r1, [r0], #1                /* load byte *r0 into r1, then r0++ */
    cmp     r1, #0                      /* end of string ('\0')? */
    bxeq    lr                          /* return to main */

    cmp     r1, #'A'                    /* check uppercase range A-Z */
    blo     chk_digit                   /* if r1 < 'A' (ASCII 65), then not letter, check digits */
    cmp     r1, #'Z'
    bls     flip_case                   /* if r1 <= 'Z' (ASCII 90), then A-Z, flip case */

    cmp     r1, #'a'                    /* check lowercase range a-z */
    blo     transform_str               /* if 'Z' < r1 < 'a' (ASCII 97), then symbol, skip */
    cmp     r1, #'z'
    bhi     transform_str               /* if r1 > 'z' (ASCII 122), then symbol, skip */

flip_case:                              /* reached when r1 is A-Z or a-z */
    eor     r1, r1, #0x20               /* bit 5 toggles A-Z <-> a-z */
    b       store_byte

chk_digit:
    cmp     r1, #'0'                    /* check digit range 0-9 */
    blo     transform_str               /* if r1 < '0' (ASCII 48), then not digit, skip */
    cmp     r1, #'9'
    bhi     transform_str               /* if r1 > '9' (ASCII 57), then not digit, skip */
    add     r1, r1, #5                  /* '0'-'4' -> '5'-'9', '5'-'9' -> ':'-'>' */
    cmp     r1, #'9'
    subhi   r1, r1, #10                 /* wrap > '9' back to '0'-'4' (modulo arithmetic) */

store_byte:
    strb    r1, [r0, #-1]               /* write modified char back to original address */
    b       transform_str               /* loop to next byte */
