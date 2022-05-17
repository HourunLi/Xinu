/*  main.c  - main */

#include <xinu.h>

process	main(void)
{

	/* Run the Xinu shell */

	// syscall_recvclr();
	// syscall_resume(syscall_create(shell, 8192, 50, "shell", 1, CONSOLE));

	// /* Wait for shell to exit and recreate it */

	// while (TRUE) {
	// 	syscall_receive();
	// 	syscall_sleepms(200);
    syscall_printf("\n\nMain process recreating shell\n\n");
	// 	syscall_resume(syscall_create(shell, 4096, 20, "shell", 1, CONSOLE));
	// }
	return OK;

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

