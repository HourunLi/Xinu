/*  main.c  - main */
#include <xinu.h>

process	main(void)
{
	while (1) {
		char ch = syscall_fgetc(KBDVGA);
		syscall_fputc(ch, KBDVGA);
	}
	return 0;
}
