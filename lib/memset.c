/* memset.c - memset */
#include <kernel.h>
/*------------------------------------------------------------------------
 *  memset  -  Set a block ot n bytes to the same value and return a
 *			   pointer to the memory
 *------------------------------------------------------------------------
 */
void	*memset(
	  void		*s,		/* Address of memory block	*/
	  int		c,		/* Byte value to use		*/
	  int		n		/* Size of block in bytes 	*/
	)
{
    register int i;
    char *cp = (char *)s;
    kprintf("memset %8x to %8x with %d\n", (uint32)s, (uint32)s + n, c);
    for (i = 0; i < n; i++)
    {
        *cp = (unsigned char)c;
        cp++;
    }
    return s;
}
