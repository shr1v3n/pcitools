/*
 * pcireadrom(1) - Read the VGA and non-VGA ROMs from /dev/mem
 *
 * - Written by JS - March 2016.
 *
 * - Rewritten to use mmap(2)
 * - VGA ROMs are at 0xc0000 - 0xc7fff (on 2kb boundaries)
 * - Non-VGA ROMs are at 0xc8000 - 0xf0000 (on 2kb boundaries)
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mman.h>

#define VGA_ROM_START           0xC0000
#define VGA_ROM_END             0xC7FFF

#define NON_VGA_ROM_START       0xC8000
#define NON_VGA_ROM_END         0xF0000

#define STEP                    2048
#define LENGTH			65535

#define ROUND_DOWN(n)           ((n) & (~(STEP-1))
#define ROUND_UP(n)             (((n) + STEP-1) & (~(STEP-1)))

unsigned flataddr = 0;
unsigned step     = STEP;

int main(int argc, char **argv)
{
        int     opt;
        int     fd;
        int     i;
        char    *mem;

        if ((fd = open("/dev/mem", O_RDWR)) < 0) {
                perror("open(2)");
                exit(1);
        }

        /*
         * mmap(2) 
         * Starting at the lowest address containing a ROM (usually VGA_ROM_START).
         */
        if ((mem =
                mmap(NULL,
                0xF0010,
                PROT_READ|PROT_WRITE,
                MAP_SHARED,
                fd, (off_t)0L)) == NULL) {
                perror("mmap(2)");
                exit(1);
        }

        /*
         * Iterate and dump each 2KB region to a .ROM file
         */

        for (i = VGA_ROM_START; i <= VGA_ROM_END - STEP; i += STEP) {
                int romfd;
                char filename[512];

                sprintf(filename, "%x.rom", i);

                /*
                 * Open <addr>.rom
                 */
                if ((romfd = open(filename, O_CREAT|O_WRONLY, 0600)) < 0) {
                        perror("open(2)");
                        fprintf(stderr, "filename was %s\n", filename);
                        continue;
                }

                /*
                 * Write 64KB.
                 */
                if (write(romfd, mem + i, LENGTH) < 0) {
                        perror("write(2)");
                        close(romfd);
                        continue;
                }

                fsync(romfd);
                close(romfd);
        }

        /*
         * Now dump non-VGA Expansion ROMs.
         */
        for (i = NON_VGA_ROM_START; i <= NON_VGA_ROM_END - STEP; i += STEP) {
               int romfd;
                char filename[512];

                sprintf(filename, "%x.rom", i);

                /*
                 * Open <addr>.rom
                 */
                if ((romfd = open(filename, O_CREAT|O_WRONLY, 0600)) < 0) {
                        perror("open(2)");
                        fprintf(stderr, "filename was %s\n", filename);
                        continue;
                }

                /*
                 * Write 64KB.
                 */
                if (write(romfd, mem + i, LENGTH) < 0) {
                        perror("write(2)");
                        close(romfd);
                        continue;
                }

                fsync(romfd);
                close(romfd);
        }


        close(fd);
        exit(0);
}
