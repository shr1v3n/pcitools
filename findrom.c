/*
 * pcifindrom(1) - Read the VGA and non-VGA ROMs from /dev/mem
 * 
 * - Rewritten to use mmap(2)
 * 
 * Traditionally: - VGA ROMs are at 0xc0000 - 0xc7fff (on 2kb boundaries) -
 * Non-VGA ROMs are at 0xc8000 - 0xf0000 (on 2kb boundaries)
 * 
 * - New features:  Locates 0xaa55 header (checks + 18 for a pointer to the PDS
 * structure, starting with 'PCIR') -                The above should help
 * locate ROMs throughout system memory, using memmem(3).
 * 
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

/*
 * PCI bit encodings of pci_phys_hi of PCI 1275 address cell.
 */
#define PCI_ADDR_MASK           PCI_REG_ADDR_M
#define PCI_ADDR_CONFIG         0x00000000	/* configuration address */
#define PCI_ADDR_IO             0x01000000	/* I/O address */
#define PCI_ADDR_MEM32          0x02000000	/* 32-bit memory address */
#define PCI_ADDR_MEM64          0x03000000	/* 64-bit memory address */
#define PCI_ALIAS_B             PCI_REG_ALIAS_M	/* aliased bit */
#define PCI_PREFETCH_B          PCI_REG_PF_M	/* prefetch bit */
#define PCI_RELOCAT_B           PCI_REG_REL_M	/* non-relocatable bit */
#define PCI_CONF_ADDR_MASK      0x00ffffff	/* mask for config address */

#define PCI_HARDDEC_8514 2	/* number of reg entries for 8514 hard-decode */
#define PCI_HARDDEC_VGA 3	/* number of reg entries for VGA hard-decode */
#define PCI_HARDDEC_IDE 4	/* number of reg entries for IDE hard-decode */
#define PCI_HARDDEC_IDE_PRI 2	/* number of reg entries for IDE primary */
#define PCI_HARDDEC_IDE_SEC 2	/* number of reg entries for IDE secondary */

/*
 * PCI Expansion ROM Header Format
 */
#define PCI_ROM_SIGNATURE               0x0	/* ROM Signature 0xaa55 */
#define PCI_ROM_ARCH_UNIQUE_START       0x2	/* Start of processor unique */
#define PCI_ROM_PCI_DATA_STRUCT_PTR     0x18	/* Ptr to PCI Data Structure */

/*
 * PCI Data Structure
 * 
 * The PCI Data Structure is located within the first 64KB of the ROM image and
 * must be DWORD aligned.
 */
#define PCI_PDS_SIGNATURE       0x0	/* Signature, the string 'PCIR' */
#define PCI_PDS_VENDOR_ID       0x4	/* Vendor Identification */
#define PCI_PDS_DEVICE_ID       0x6	/* Device Identification */
#define PCI_PDS_VPD_PTR         0x8	/* Pointer to Vital Product Data */
#define PCI_PDS_PDS_LENGTH      0xa	/* PCI Data Structure Length */
#define PCI_PDS_PDS_REVISION    0xc	/* PCI Data Structure Revision */
#define PCI_PDS_CLASS_CODE      0xd	/* Class Code */
#define PCI_PDS_IMAGE_LENGTH    0x10	/* Image Length in 512 byte units */
#define PCI_PDS_CODE_REVISON    0x12	/* Revision Level of Code/Data */
#define PCI_PDS_CODE_TYPE       0x14	/* Code Type */
#define PCI_PDS_INDICATOR       0x15	/* Indicates if image is last in ROM */

/*
 * PCI Memory search size.
 */
#define PCI_MEMORY_ROM_SIZE	0x10000

int		iflag = 0;
int		aflag = 0;
unsigned	flataddr = 0;
unsigned long	haystacklen = 0;

int 
main(int argc, char **argv)
{
	int		opt;
	int		fd;
	int		i;
	char           *rmem, *mem, *loc;
	unsigned	ofs;
	unsigned short	hdr = 0xAA55;

	while ((opt = getopt(argc, argv, "i:a:")) != -1)
		switch (opt) {
		case 'i':
			iflag++;/* Iterate through all ROM memory. */
			break;
		case 'a':
			aflag++;
			flataddr = strtoul(optarg, NULL, 0);
			break;
		}

	if (!aflag)
		flataddr = 0;

	/*
	 * Open /dev/mem cdev.
	 */

	if ((fd = open("/dev/mem", O_RDWR)) < 0) {
		perror("open(2)");
		exit(1);
	}

	/*
	 * mmap(2) 4GB of address space (32-bits full) Starting at the lowest
	 * address containing a ROM (usually PCI_VGA_ROM_START).
	 */
	if ((mem =
	     mmap(NULL,
		  PCI_MEMORY_ROM_SIZE,
		  //0x1000000,
		  PROT_READ | PROT_WRITE,
		  MAP_SHARED,
		  fd, (off_t) 0L)) == NULL) {
		perror("mmap(2)");
		exit(1);
	}
	fprintf(stderr, "\n"
		"Build without CONFIG_STRICT_DEVMEM kernel configuration setting!\n\n");

	fprintf(stderr, "      Page Size = %d bytes (%#x).\n", getpagesize(), getpagesize());
	fprintf(stderr, "      Mmap(2) of /dev/mem 4GB (%#x bytes) @ %p.  File desc = %d, PROT_READ|PROT_WRITE\n", PCI_MEMORY_ROM_SIZE, mem, fd);
	fprintf(stderr, "           Searching for PCI Option ROM / Expansion ROM header (0xAA55)\n");

	haystacklen = PCI_MEMORY_ROM_SIZE;
	rmem = mem;

	for (;;) {
		int		romfd;
		char		filename  [512], *ptr;
		unsigned short *shptr;
		unsigned       *pds;
		unsigned	totalsz;
		unsigned short	units;

		hdr = 0xAA55;

		printf("Calling memmem(%p, %zu, %p, %zu)...\n", mem, haystacklen, &hdr, sizeof(hdr));;
		if ((loc = memmem(mem, haystacklen, &hdr, sizeof(hdr))) == NULL) {
			fprintf(stderr, "\n     Finished search!\n");
			munmap(rmem, PCI_MEMORY_ROM_SIZE);
			close(fd);
		}
		shptr = (unsigned short *)(loc + PCI_ROM_PCI_DATA_STRUCT_PTR);
		pds = (unsigned *)shptr;
		fprintf(stderr, "      PDS = loc + 0x18 (%p = %p + 0x18)\n", pds, loc);
		fprintf(stderr, "      PDS dereferenced = %#x\n", *pds);

		/*
		 * Validate the PCI Data Structure Signature. 0x52494350 =
		 * "PCIR"
		 */

		if (strncmp((char *)pds, "PCIR", 4) != 0) {
			mem = loc + sizeof(hdr);
			continue;
		}
		printf("     PCIR at %p (offset %p)!\n", ptr, ptr - rmem);

		/*
		 * Found a 0xAA55 where 0x18 bytes past is a pointer to a PDS
		 * struct starting with 'PCIR'. 0x10 bytes later is the
		 * length of the ROM in 512 byte units.
		 */
		ptr += 0x10;
		units = *(unsigned short *)&ptr[0];
		totalsz = units * 512;
		fprintf(stderr, "     PCIR PDS String Located at offset 0x0 from pointer at offset 0x18.\n");
		fprintf(stderr, "     Units = %d * 512 byte blocks.  Total size = %d.\n", units, totalsz);

		sprintf(filename, "%x.rom", loc);
		if ((romfd = open(filename, O_RDWR)) < 0) {
			perror("open(2)");
			exit(1);
		}
		/*
		 * Write out (unsigned)units * 512 of data at 'loc' to ROM
		 * file descriptor.
		 */
		(void)write(romfd, loc, totalsz);
		close(romfd);

		/*
		 * Increment mem, decrement haystacklen.
		 */
		unsigned long	tmp = (unsigned long)(loc + totalsz) - (unsigned long)rmem;

		fprintf(stderr, "     Found PCI Option ROM!! Loc @ %p (Offset = %lu).  Saved to %s.\n", loc, (unsigned long)loc - (unsigned long)mem, filename);
		mem = loc + totalsz;
		haystacklen -= tmp;
	}

	close(fd);
	exit(0);
}
