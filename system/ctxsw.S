/* ctxsw.S - ctxsw (for x86) */

		.text
		.globl	ctxsw

/*------------------------------------------------------------------------
 * ctxsw -  X86 context switch; the call is ctxsw(&old_sp, &new_sp, pgdir, flag)
 *------------------------------------------------------------------------
 */
ctxsw:
		pushl	%ebp		/* Push ebp onto stack		                */
		movl	%esp, %ebp	/* Record current SP in ebp	                */
		pushfl			    /* Push flags onto the stack	            */
		pushal			    /* Push general regs. on stack	            */

		/* Save old segment registers here, if multiple allowed         */

		movl	8(%ebp), %eax	/* Get mem location in which to	        */
		/*   save the old process's SP	                                */
		movl	%esp, (%eax)	/* Save old process's SP	            */
		movl	12(%ebp), %eax	/* Get location from which to	        */
		
        movl    16(%ebp), %ebx  /* Get the page directory physical addr */
        movl    20(%ebp), %edx  /* Get the flag                         */
		/* The next instruction switches from the old process's	        */
		/* stack to the new process's stack.			                */
        /* restore new process's SP	                                    */
		movl	(%eax), %esp	/* Pop up new process's SP	            */
        movl    %ebx, %cr3      /* load the pgdir to cr3                */

        /* Check whether need to free last process's memory source      */
        testl    $1, %edx
        jz      .skipFree
.free
        call    freeLastProcessMemmoryResource
.skipFree
		/* Restore new seg. registers here, if multiple allowed         */
		popal			        /* Restore general registers	        */
		movl	4(%esp), %ebp	/* Pick up ebp before restoring	        */

		popfl			        /* Restore interrupt mask	            */
		add	$4, %esp		    /* Skip saved value of ebp	            */
		ret			            /* Return to new process	            */
