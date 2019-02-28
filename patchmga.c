/* 
 * patchmga(1)
 * - Rewritten to use mmap(2) instead of read(2), lseek(2), write(2)
 * - Because /dev/mem does not allow write(2) to succeed, writes must
 *   be mmap(2)'d.
 * - syncs PAGE_SIZE at a time
 * - Apply the patch for pat.c provided to the Linux kernel and rebuild.
 * - Disable your kernel's STRICT_DEVMEM setting.
 *
 * NOTE TO SELF:  
 *
 *  Patch the MGA device framebuffer to a preferrable state to
 *   cause a bus-mastering to succeed, or.. maybe not.. havent had to use it
 *   since X5-2 hack on the SP ARM processor from host.
 *  
 *  Or, vaguely, it modifies the chipset memory into a preferrable state
 *   which I honestly cant remember and dont feel like discerning from
 *   reading this code right now.  
 *  
 *  So the above is just sort of a hunch.  80% sure it's correct.
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

#define PAGE_SIZE	 getpagesize()
#define ROUND_PAGE(x)    ((void *)(((unsigned long)(x)) & ~((unsigned long)(PAGE_SIZE - 1))))

unsigned long value = 0;
unsigned int flataddr, mapaddr, offset;
int 	     rflag = 0, wflag = 0;

int main(int argc, char **argv)
{
	int	opt;
	int 	fd;
	char    *mem;

	while ((opt = getopt(argc, argv, "f:rw:")) != -1) switch (opt) {
		case 'f':
			flataddr = strtoul(optarg, NULL, 0);
			break;
		case 'w':
			value = strtoul(optarg, NULL, 0);
			wflag++;
			break;
		case 'r':
			rflag++;
			break;
	}

	if (!wflag && !rflag) {
		fprintf(stderr, "usage: patchmem [-f flataddr] [-r]           (read word)\n");
		fprintf(stderr, "usage: patchmem [-f flataddr] [-w <value>]\n");
		exit(0);
	}
			
		
	if ((fd = open("/dev/mem", O_RDWR)) < 0) {
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

	mapaddr = (long)ROUND_PAGE(flataddr);
	offset  = flataddr - mapaddr;

	printf("\n");
	printf("       Opened /dev/mem for read and write.  Fd=%d\n", fd);
	printf("       Calling mmap(2) to map 0x1000 bytes (%d pages) from offset to page: %#x.\n",0x1000/4096,
					mapaddr);
	printf("       Flataddr = %p, mapaddr = %p, offset = %d (%#x)\n", (void *)flataddr, (void *)mapaddr, offset, offset);


	if ((mem = 
                mmap(NULL, 
		0x1000, 
		PROT_READ|PROT_WRITE, 
		MAP_SHARED, 
		fd, (off_t)mapaddr)) == MAP_FAILED) {
		perror("mmap(2)");
		exit(1);
	}

	printf("       mmap(2) gave address range %p->%p\n", mem, mem+0x1000);
	printf("       Index to change in mem is %d (address %p).\n",  offset, &mem[offset]);
	printf("\n");

	(void)mprotect(mem, 0x1000, PROT_READ|PROT_WRITE);
	perror("mprotect(2)");

	if (rflag) {
		printf("%#lx\n", *(unsigned long *)&mem[offset]);
		munmap(mem, 0x1000);	
		close(fd);
		exit(0);
	}
	
	if (wflag) {
		printf("%#lx->", *(unsigned long *)&mem[offset]);
		*(unsigned int *)&mem[offset] = value;
		fflush(stdout);
		printf("%#lx\n", *(unsigned long *)&mem[offset]);
		msync(mem, 0x1000, MS_SYNC | MS_INVALIDATE);
		perror("msync(2)");
		fsync(fd);
		munmap(mem, 0x1000);
		fsync(fd);
		perror("fsync(2)");
		exit(0);
	}

	/*NOTREACHED*/
	perror("NOTREACHED reached");
}	
