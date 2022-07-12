/* xsh_kbdtest.c - xsh_kbdtest */
#include <xinu.h>

shellcmd xsh_kbdvgatest(int nargs, char *args[]) {
	while (1) {
		char ch = syscall_fgetc(KBDVGA);
		syscall_fputc(ch, KBDVGA);
	}
	return 0;
}
