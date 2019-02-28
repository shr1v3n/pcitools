/*
 * pioscan(1)
 * - Scan all 64k PIO ports
 * - Ports that aren't valid return 0xFF
 * - Of course, valid ports might also return that.
 * - This is just a quick scan, since /proc/ioports is not always reliable.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>

int main(int argc, char **argv)
{
	unsigned char	val;
	int		i;

	if (iopl(3) != 0) {
		perror("iopl(2)");
		exit(1);
	}

	for (i = 0; i <= 65535; i++) {
		val = inb(i);
		if (val != 0xff) 
			printf("Port %#hx: Value %#hhx\n", i, val);
	}
	
	exit(0);
}
