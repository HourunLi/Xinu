/*  main.c  - main */

#include <xinu.h>

void sort(int32 *array, uint32 n) {
	for (uint32 i = 0; i < n; i++)
		for (uint32 j = i + 1; j < n; j++)
			if (array[i] > array[j]) {
				int32 tmp = array[i];
				array[i] = array[j];
				array[j] = tmp;
			}
}

process	main(void)
{

	// /* Run the Xinu shell */

	syscall_recvclr();
	syscall_resume(syscall_create(shell, 8192, 50, "shell", 1, CONSOLE));

	/* Wait for shell to exit and recreate it */

	while (TRUE) {
		syscall_receive();
		syscall_sleepms(200);
        syscall_printf("\n\nMain process recreating shell\n\n");
		syscall_resume(syscall_create(shell, 4096, 20, "shell", 1, CONSOLE));
	}
	// int32 *array = malloc(0);
	// uint32 n = 0;
	
	// for (int32 i = 1; i < nargs; i++) {
	// 	int32 x = atoi(args[i]);
	// 	kprintf("xsh_sort: x = %d\n", x);

	// 	array = realloc(array, sizeof(int32) * (n + 1));
	// 	array[n++] = x;

	// 	sort(array, n);
	// 	for (uint32 j = 0; j < n; j++)
	// 		printf("%d%c", array[j], " \n"[j == n - 1]);
	// }
	// return OK;
}

/*  main.c  - main */

// #include <xinu.h>
// process	main(void)
// {

// 	// /* Run the Xinu shell */

// 	// recvclr();
// 	// resume(create(shell, 8192, 50, "shell", 1, CONSOLE));

// 	// /* Wait for shell to exit and recreate it */

// 	// while (TRUE) {
// 	// 	receive();
// 	// 	sleepms(200);
// 	// 	kprintf("\n\nMain process recreating shell\n\n");
// 	// 	resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
// 	// }
//     // while (TRUE) {
//         do_syscall(8, 1, "hello from syscall\n");
//     // }
// 	return OK;
    
// }

