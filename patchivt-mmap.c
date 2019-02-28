/*
 * patchivt(1) - Patch/view the real-mode Interrupt Vector Table using /dev/mem
 *
 * - Written by JS - March 2016.
 *
 * - Rewritten to use mmap(2)
 * - Apparently write(2) on /dev/mem and /dev/kmem does not work (but returns EOK)
 * - While read(2) does.. this is quite confusing.  mmap(2) is the only way
 * - To get your vector table changes to stick.
 *
 * - Thank Linux, and FreeBSD (same problem) though it's write(2) returns ENODIR.
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

int 	intno;
int 	pflag = 0, sflag = 0, oflag = 0;
unsigned short segment = 0;
unsigned short offset = 0;
unsigned int flataddr;

unsigned short read_segment;
unsigned short read_offset;
unsigned int read_flataddr;


int main(int argc, char **argv)
{
	int	opt;
	int 	fd;
	void    *mem;

	while ((opt = getopt(argc, argv, "i:s:o:p")) != -1) switch (opt) {
		case 'i':
			intno = strtoul(optarg, NULL, 0);
			break;
		case 's':
			segment = strtoul(optarg, NULL, 0);
			sflag++;
			break;
		case 'o':
			offset = strtoul(optarg, NULL, 0);
			oflag++;
			break;
		case 'p':
			pflag++;
			break;
	}

	if (!sflag && !oflag && !pflag) {
		fprintf(stderr, "usage: patchivt [-i interrupt] [-p]    (print address of interrupt in ivt)\n");
		fprintf(stderr, "usage: patchivt [-i interrupt] [-s segment] [-o offset]\n");
		exit(0);
	}
			
		
	if ((fd = open("/dev/mem", O_RDWR)) < 0) {
		perror("open(2)");
		exit(1);
	}

	if ((mem = 
                mmap(NULL, 
		0x1000, 
		PROT_READ|PROT_WRITE, 
		MAP_SHARED, 
		fd, (off_t)0L)) == NULL) {
		perror("mmap(2)");
		exit(1);
	}

	read_offset = *(unsigned short *)&mem[intno << 2];
	read_segment = *(unsigned short *)&mem[(intno << 2) + 2];
	
	read_flataddr = (read_segment << 4) | read_offset;

	printf("(Before Patching) Interrupt %#x: Segment:Offset = %.4hx:%.4hx, Flataddr = %#x\n", intno, read_segment, 
			read_offset, read_flataddr);

	if (pflag) {
		close(fd);
		exit(0);
	}

	*(unsigned short *)&mem[intno << 2] = offset;
	*(unsigned short *)&mem[(intno << 2) + 2] = segment;
	
	/*
	 * Now msync() and fsync() and re-check.
	 */

	msync(mem, 0x1000, MS_SYNC);
	fsync(fd);

        read_offset = *(unsigned short *)&mem[intno << 2];
        read_segment = *(unsigned short *)&mem[(intno << 2) + 2];

        read_flataddr = (read_segment << 4) | read_offset;
        
        printf("(After Patching) Interrupt %#x: Segment:Offset = %.4x:%.4x, Flataddr = %#x\n", intno, read_segment,
                        read_offset, read_flataddr);
                        

	close(fd);
	exit(0);
}
	

