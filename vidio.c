#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>

#define   MGA_PORT_BASE		0x03C2

#define	  MGA_PORT_CNTL		MGA_PORT_BASE
#define   MGA_PORT_READ		(MGA_PORT_BASE + 1)

int main()
{
	unsigned char	val;

	if (iopl(3) != 0) {
		perror("iopl(2)");
		exit(1);
	}

	outb(0, MGA_PORT_CNTL);

	usleep(15);

	outb(0x0F, MGA_PORT_CNTL);

	usleep(15);

	val = inb(MGA_PORT_CNTL);
	printf("%#hhx\n", val);
	
	usleep(15);
	
	val = inb(MGA_PORT_READ);
	printf("%#hhx\n", val);

	exit(0);
}
