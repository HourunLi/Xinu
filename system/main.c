/*  main.c  - main */

#include <xinu.h>
char buffer[16400];

int sys_uprintf(const char* fmt, ...) {
    syscall_printf("Hello world from hello.elf\n");
    return 0;
}

process	main(void)
{
	// /* Run the Xinu shell */
	syscall_recvclr();
    uint32 size = read_file("HELLO.ELF", buffer, 16384);
    uint32 entrypoint = (uint32) buffer + get_elf_entrypoint(buffer);
	syscall_resume(syscall_create(entrypoint, 8192, 50, "printf", 1, sys_uprintf));
}

