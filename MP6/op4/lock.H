/*
    File: lock.H

    Author: Jin Huang
            Department of Computer Science
            Texas A&M University
    Date  : 04/18/2021

    Description: Lock Implementation. 
                 
*/

#ifndef _LOCK_H_
#define _LOCK_H_

// xchgl
int TSL(int *addr);

void lock(int *lck);
void unlock(int *lck);


#endif
