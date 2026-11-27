/* POSIX / termios constants */
.equ O_RDWR,     2
.equ O_NOCTTY,   0x100
.equ TCSANOW,    0
.equ TCIOFLUSH,  2
.equ B9600,      13                 

.section .rodata
dev_path:   .asciz "/dev/ttyAMA0"
reply_fmt:  .asciz "CHAR=%c COUNT=%d\n"

.section .text
.align 4
.global main
.type main, %function

main:
    /* Prologue:
    save r4-r7 (callee-saved regs) and lr = 20 bytes.
    allocate 68 bytes on the stack -> 20 + 68 = 88 bytes (keep sp 8-byte aligned).
    this single 68-byte stack buffer is reused:
        - during setup: holds struct termios (60 bytes)
        - during main loop: holds in_buf (64 chars + 1 scratch byte) */
    push    {r4-r7, lr}
    sub     sp, sp, #68

    /* open /dev/ttyAMA0 with O_RDWR | O_NOCTTY */
    ldr     r0, =dev_path
    mov     r1, #O_RDWR
    orr     r1, r1, #O_NOCTTY
    bl      open
    cmp     r0, #0
    movlt   r0, #1
    blt     exit_ret                /* return if open failed */
    mov     r4, r0                  /* save fd in callee-saved r4 */

    /* put /dev/ttyAMA0 into raw mode */
    mov     r1, sp                  /* arg2 (r1) = &tty_cfg (r0 still holds fd) */
    bl      tcgetattr               /* read current UART settings into [sp...sp+59] */

    mov     r0, sp                  /* arg1 (r0) = &tty_cfg */
    bl      cfmakeraw               /* disable echo/line-buffering - sets 8-bit, VMIN=1, VTIME=0 */

    /* ensure UART receiver is on, ignore modem lines, disable HW flow control */
    ldr     r1, [sp, #8]            /* load tty_cfg.c_cflag */
    orr     r1, r1, #0x0880         /* set CREAD (0x0080) | CLOCAL (0x0800) */
    bic     r1, r1, #0x80000000     /* clear CRTSCTS */
    str     r1, [sp, #8]            /* store updated c_cflag back */

    mov     r0, sp                  /* arg1 (r0) = &tty_cfg */
    mov     r1, #B9600              /* arg2 (r1) = 9600 baud */
    bl      cfsetspeed              /* set both input & output baud rate */

    mov     r0, r4                  /* arg1 (r0) = fd */
    mov     r1, #TCSANOW            /* arg2 (r1) = apply immediately */
    mov     r2, sp                  /* arg3 (r2) = &tty_cfg */
    bl      tcsetattr               /* apply settings to /dev/ttyAMA0 */

    mov     r0, r4                  /* arg1 (r0) = fd */
    mov     r1, #TCIOFLUSH          /* arg2 (r1) = flush both RX and TX queues */
    bl      tcflush                 /* clear any leftover garbage bytes */

    /* main loop: read 1 line until '\n', store max 64 chars */
    /* from here on, [sp...sp+63] is in_buf and [sp+64] is a scratch byte for overflow */
msg_loop:
    mov     r5, sp                  /* r5 = write pointer (at end of loop, r5 = end_ptr) */
    add     r6, sp, #64             /* r6 = limit pointer (sp + 64) */
read_loop:
    mov     r0, r4                  /* arg1 (r0) = fd */
    mov     r1, r5                  /* arg2 (r1) = current write position in in_buf */
    mov     r2, #1                  /* arg3 (r2) = read 1 byte (blocks until 1 byte arrives) */
    bl      read                    /* r0 = read(fd, r5, 1) */
    cmp     r0, #1
    blt     done                    /* if EOF or error, close fd and exit cleanly */

    ldrb    r0, [r5]                /* load the byte just read into r0 */
    cmp     r0, #'\n'
    beq     find_init               /* if '\n' reached, start counting frequencies */

    cmp     r5, r6
    addlo   r5, r5, #1
    b       read_loop

    /* in-place frequency search */
find_init:
    mov     r2, #'?'                /* best_char = '?' fallback (if line is empty/all spaces) */
    mov     r3, #0                  /* best_count = 0 */
    mov     r0, sp                  /* r0 = outer pointer p, starts at in_buf[0] */

outer_loop:
    cmp     r0, r5
    bhs     send_reply              /* if reached end_ptr, then reply */
    ldrb    r1, [r0], #1            /* r1 = cand = *p, then p++ */
    cmp     r1, #' '
    beq     outer_loop              /* skip spaces */

    /* count occurrences of cand from current p onward (includes cand itself -> start at 1) */
    mov     r6, r0                  /* r6 = inner pointer q = p (already points past cand) */
    mov     r12, #1                 /* r12 = count = 1 */
inner_loop:
    cmp     r6, r5
    bhs     check_best              /* if reached end_ptr (q >= r5), then compare count against current best */
    ldrb    r7, [r6], #1            /* r7 = *q, then q++ */
    cmp     r7, r1
    addeq   r12, r12, #1            /* if *q matches cand, then count++ */
    b       inner_loop

check_best:
    cmp     r12, r3                 /* if count > best_count, set hi */
    cmpeq   r2, r1                  /* if count == best_count AND best_char > cand (smaller ASCII), set hi */
    movhi   r3, r12                 /* if hi, then best_count = count */
    movhi   r2, r1                  /* if hi, then best_char  = cand */
    b       outer_loop

    /* format and send reply directly to serial fd */
send_reply:
    mov     r0, r4                  /* arg1 (r0) = fd */
    ldr     r1, =reply_fmt          /* arg2 (r1) = "CHAR=%c COUNT=%d\n" */
    bl      dprintf                 /* formats and writes directly to fd */

    mov     r0, r4                  /* arg1 (r0) = fd */
    bl      tcdrain                 /* wait until all transmitted bytes leave the UART */
    b       msg_loop                /* wait for the next string from the host */

    /* clean exit */
done:
    mov     r0, r4                  /* arg1 (r0) = fd */
    bl      close                   /* close(fd) */
    mov     r0, #0                  /* return 0 (success) */
exit_ret:
    add     sp, sp, #68             /* release 68-byte stack buffer */
    pop     {r4-r7, pc}             /* restore r4-r7 and return from main (pop lr into pc) */