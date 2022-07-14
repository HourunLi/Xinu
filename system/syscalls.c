#include <xinu.h>

// Syscall list to dispatch in kernel space

const void *syscalls[] = {
	&addargs,       // 0
	&create,		// 1
	&resume,		// 2
	&recvclr,		// 3
	&receive,		// 4
	&sleepms,		// 5
	&sleep,			// 6
	&fprintf,		// 7
	&printf,		// 8
	&fscanf,		// 9
	&read,			// 10
	&open,			// 11
	&control,		// 12
	&kill,			// 13
	&getpid,		// 14
	&malloc,        // 15
    &free,          // 16
    &realloc,       // 17
    &fgetc,         // 18
    &fputc,         // 19
    &getc,          // 20
    &seek           // 21
};

// Syscall wrapper for doing syscall in user space

uint32 do_syscall(uint32 id, uint32 args_count, ...) {
	uint32 return_value;

	// You may need to pass these veriables to kernel side:

	uint32 *ptr_return_value = &return_value;
	// args_count;
	uint32 *args_array = 1 + &args_count;
    uint32 callee = syscalls[id];

    /**
     * @eax = ptr_return_value
     * @edi = args_count
     * @esi = args_array
     * @edx = callee
     */

    asm("movl %0, %%edi;"
        :
        :"m"(args_count)
        :"%edi"
    );

    asm("movl %0, %%esi;"
        :
        :"m"(args_array)
        :"%esi"
    );

    asm("movl %0, %%edx;"
        :
        :"m"(callee)
        :"%edx"
    );

    asm("movl %0, %%eax;"
        :
        :"m"(ptr_return_value)
        :"%eax"
    );

    asm("int $48");
	return return_value;
}