#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Maximum number of LDT entries supported. */
#define LDT_ENTRIES     8192
/* The size of each LDT entry. */
#define LDT_ENTRY_SIZE  8

/*
 * Note on 64bit base and limit is ignored and you cannot set DS/ES/CS
 * not to the default values if you still want to do syscalls. This
 * call is more for 32bit mode therefore.
 */
struct user_desc {
        unsigned int  entry_number;
        unsigned int  base_addr;
        unsigned int  limit;
        unsigned int  seg_32bit:1;
        unsigned int  contents:2;
        unsigned int  read_exec_only:1;
        unsigned int  limit_in_pages:1;
        unsigned int  seg_not_present:1;
        unsigned int  useable:1;
        /*
         * Because this bit is not present in 32-bit user code, user
         * programs can pass uninitialized values here.  Therefore, in
         * any context in which a user_desc comes from a 32-bit program,
         * the kernel must act as though lm == 0, regardless of the
         * actual value.
         */
        unsigned int  lm:1;
};

#define NENT 	LDT_ENTRIES	

struct user_desc	dt[NENT];

int main(int argc, char **argv)
{
	struct user_desc 	*ldt;
	int			i;
	register long		r  asm("%ecx");

	r = (long) &dt[0];
	__asm__ __volatile__("sldt  %ecx");

	ldt     = (struct user_desc *) &dt;

	printf("LDT @ Address %p : %d entries\n", ldt, NENT);	
	printf("\n");

	for (i = 0; i < NENT; ldt, i++) {
		printf("LDT entry %4d   >   ldt entry_number 0x%.08x | ldt base_addr 0x%.08x | ldt limit %.08x | seg_32bit %d | contents %d\n"
		       ">>>>>>>>>>>>>>>>>>   read_exec_only %d | limit_in_pages %d | seg_not_present %d | useable %d | longmode %d\n",
			i, ldt->entry_number,   ldt->base_addr, ldt->limit, ldt->seg_32bit, ldt->contents,
			   ldt->read_exec_only, ldt->limit_in_pages, ldt->seg_not_present, ldt->useable, ldt->lm);
		printf("\n");
	}
}


	
