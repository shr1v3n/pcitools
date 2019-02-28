/* 
 * findmem(1)
 * - Rewritten to use mmap(2) instead of read(2), lseek(2), write(2)
 * - syncs PAGE_SIZE at a time
 * - Patch kernel to fix x86/mm/pat.c bug that breaks /dev/mem 
 *   (TODO: Get linux lkml to patch this and commit)
 * - Disable your kernel's STRICT_DEVMEM setting.
 *
 * Usage: findmem -F /dev/mem -0 word1 -1 word2 -2 word3 -3 word4 -4 word5
 *
 * -JS 2016
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mman.h>

#define PAGE_SIZE	 getpagesize()
#define ROUND_PAGE(x)    ((void *)(((unsigned long)(x)) & ~((unsigned long)(PAGE_SIZE - 1))))

unsigned int	value[16] = { 0 };
int 		values = 0;

int main(int argc, char **argv)
{
	int		i;
	int		opt;
	int 		fd;
	unsigned int	*loc = NULL;
	char   		 *mem;
	char 		*filename = NULL;
	unsigned long	mapaddr = 0;
	unsigned long   offset;
	unsigned long	addr = 0;
	unsigned long	limit = 0x10000;

	while ((opt = getopt(argc, argv, "A:L:F:0:1:2:3:4:5:6:7:8:9:a:b:c:d:e:f")) != -1) switch (opt) {
		case 'A':
			addr = strtoul(optarg, NULL, 0);
			break;
		case 'L':
			limit = strtoul(optarg, NULL, 0);
			break;
		case 'F':
			filename = optarg;
			break;
		case '0':
			value[0] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '1':	
			value[1] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '2':
			value[2] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '3':
			value[3] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '4':
			value[4] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '5':
			value[5] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '6':
			value[6] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '7':
			value[7] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '8':
			value[8] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case '9':
			value[9] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case 'a':
			value[10] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case 'b':
			value[11] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case 'c':
			value[12] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case 'd':
			value[13] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case 'e':
			value[14] = strtoul(optarg, NULL, 0);
			values++;
			break;
		case 'f':
			value[15] = strtoul(optarg, NULL, 0);
			values++;
			break;
	}

	if (!filename || !values) {
		fprintf(stderr, "Usage: findmem [ -A base ] [ -F filename ] [ -0123456789abcdef longword ]\n");
		exit(0);
	}
			
	/*
	 * Open file or device (/dev/mem usually).
	 */
		
	if ((fd = open(filename, O_RDONLY)) < 0) {
		perror("open(2)");
		exit(1);
	}
		
	/*
 	 * Reasons mmap(2) returns EINVAL:
	 * The first reason is because offset is not on a page boundary, and is what is happening. 
         * EINVAL We don't like addr, length, or offset (e.g., they are too large, or not aligned on a page boundary).
         * EINVAL (since Linux 2.6.12) length was 0.
         * EINVAL flags contained neither MAP_PRIVATE or MAP_SHARED, or contained both of these values.
	 */

	mapaddr = (long)ROUND_PAGE(addr);

	printf("\n");
	printf("       Opened %s for read.  File desc=%d\n", filename, fd);
	printf("       Calling mmap(2) to map limit bytes (%ld pages) from address: %#lx.\n", limit / getpagesize(),
					mapaddr);
        if ((mem = 
                mmap(NULL, 
                (size_t)ROUND_PAGE(limit), 
                PROT_READ, 
                MAP_SHARED, 
                fd, (off_t)mapaddr)) == MAP_FAILED) {
                perror("mmap(2)");
                exit(1);
        }

	printf("       mmap(2) gave address range %p->%p\n", mem, mem + limit);
	printf("\n");

        printf("       Performing memmem(3) to find %d values:\n", values);

        for (i = 0; i < values; i++)
                printf("                Decimal: %d    Hexadecimal: %#x\n", value[i], value[i]);

	(void)mprotect(mem, limit, PROT_READ);

	printf("       Memmem(3) finding needles in haystack...");
	fflush(stdout);

	if ((loc = memmem(mem, limit, 
			  value, values * sizeof(value)[0])) == NULL) { 
		printf("\n                Found none!\n");
		munmap(mem, limit);
		close(fd);
		exit(1);
	}

	offset = (long)loc - (long)mem;
	printf("\n              Found values! At Offset=%#lx (mmap(2) pointer %p)\n", offset, loc);
	printf("              Offset from mapaddr: %#lx\n", (long)mapaddr + offset);

	munmap(mem, limit);
	close(fd);
	exit(0);
}
