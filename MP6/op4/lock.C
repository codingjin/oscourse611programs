/* 
    File: lock.C

    Author: Jin Huang
            Department of Computer Science
            Texas A&M University
    Date  : 04/18/2021
	
	Lock Implementation

*/

/*--------------------------------------------------------------------------*/
/* EXTERNS */
/*--------------------------------------------------------------------------*/

#include "lock.H"

void lock(int *lck) {
	while (TSL(lck) != 0);
}

void unlock(int *lck) {
	*lck = 0;
}

int TSL(int *addr) {

	register int content = 1;
	// xchgl content, *addr
	// xchgl exchanges the values of its 2 operands,
	// while locking the memory bus to exclude other operations.
	asm volatile (
		"xchgl %0,%1" : "=r" (content),
		"=m" (*addr) : "0" (content),
		"m" (*addr)
	);
	return (content);

}

