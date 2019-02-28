#include <stdio.h>

char ldt[2048];

unsigned long get_r8()
{
	__asm__ __volatile__ ("movq %r8, %rax");
}

main()
{
	int i;
	unsigned long r8 = 0;
	char *p = NULL;

	r8 = (void *)&ldt;

	__asm__ __volatile__ ("sldt %r8");

	r8 = get_r8();

	printf("ldt @ %#lx\n", ldt);

	p = (char *)ldt;

	for (i = 0; i < 512; i++) {
		printf("%.02hhx   ", p[i]);
		if ((i & 15) == 15) 
			printf("\n");
	}
	printf("\n");
}

