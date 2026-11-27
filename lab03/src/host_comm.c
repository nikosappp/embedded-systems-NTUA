#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_CHARS 64

static int configure_raw_pty(int fd) {
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("tcgetattr");
        return -1;
    }
    cfmakeraw(&tty);
    tty.c_cflag |= (CLOCAL | CREAD);    // ignore modem lines; enable receiver 
    tty.c_cflag &= ~CRTSCTS;            // disable hardware flow control 
    tty.c_cc[VMIN] = 0;                 // allow read() to time out 
    tty.c_cc[VTIME] = 50;               // 5 second timeout 
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        return -1;
    }
    return 0;
}

int main(void) {
    // open /dev/ptmx (pseudoterminal master) per man pts 
    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd < 0) {
        perror("posix_openpt");
        return 1;
    }

    // grant access to slave pseudoterminal 
    if (grantpt(master_fd) != 0) {
        perror("grantpt");
        close(master_fd);
        return 1;
    }

    // unlock slave pseudoterminal device
    if (unlockpt(master_fd) != 0) {
        perror("unlockpt");
        close(master_fd);
        return 1;
    }

    // retrieve slave path (/dev/pts/N) 
    const char *slave_name = ptsname(master_fd);
    if (!slave_name) {
        perror("ptsname");
        close(master_fd);
        return 1;
    }

    // put the PTY into raw mode before QEMU attaches 
    if (configure_raw_pty(master_fd) != 0) {
        close(master_fd);
        return 1;
    }

    printf("PTY slave created at: %s\n", slave_name);
    printf("Start QEMU with: -serial %s and run: ./guest_freq.out\n", slave_name);
    printf("Press ENTER here once guest_freq.out is listening...\n");

    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    // discard any stale bytes generated while QEMU opened the slave 
    tcflush(master_fd, TCIOFLUSH);

    // interactive send/receive loop (Ctrl+D to exit) 
    char line[MAX_CHARS + 2];           // Up to 64 chars + '\n' + '\0' 
    while (1) {
        printf("\nPlease give a string to send to host:\n");

        if (!fgets(line, sizeof(line), stdin)) {
            break;                      // EOF (Ctrl+D) 
        }
        
        size_t len = strlen(line);
        if (line[len-1] != '\n') {
            // drain characters beyond 64-character limit
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF);
            // add '\n' if string is over 64 characters long
            line[MAX_CHARS] = '\n';
            line[MAX_CHARS+1] = '\0';
        }

        // transmit string over PTY master to QEMU
        size_t sent = 0;
        while (sent < len) {
            ssize_t w = write(master_fd, line + sent, len - sent);
            if (w <= 0) {
                perror("write to PTY master");
                close(master_fd);
                return 1;
            }
            sent += (size_t)w;
        }

        // read formatted reply from guest ("CHAR=<c> COUNT=<n>\n") 
        char resp[128] = {0};
        size_t rpos = 0;
        int timed_out = 0;
        while (rpos < sizeof(resp) - 1) {
            ssize_t r = read(master_fd, resp + rpos, sizeof(resp) - 1 - rpos);
            if (r < 0) {
                perror("read from PTY master");
                close(master_fd);
                return 1;
            }
            if (r == 0) {
                timed_out = 1;
                break;
            }
            rpos += (size_t)r;
            if (memchr(resp, '\n', rpos) != NULL) {
                break;
            }
        }
        resp[rpos] = '\0';

        if (timed_out) {
            fprintf(stderr, "Timed out waiting for reply from guest.\n");
            continue;
        }

        char best_char = '\0';
        int best_count = 0;
        if (sscanf(resp, "CHAR=%c COUNT=%d", &best_char, &best_count) == 2) {
            if (best_count == 0) {
                printf("No non-space characters were found.\n");
            } else {
                printf("The most frequent character is\n\"%c\"\nand it appeared %d times.\n",
                       best_char, best_count);
            }
        } else {
            printf("Unexpected reply from guest: %s\n", resp);
        }
    }

    close(master_fd);
    return 0;
}
