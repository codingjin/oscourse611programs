/* 
    File: disk_back.H

    Author: Jin Huang
    Date  : 04/17/2021

	Interrupt Handler for Disk Interrupt, NO.14

*/

#ifndef _DISK_BACK_H
#define _DISK_BACK_H

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "interrupts.H"

/*--------------------------------------------------------------------------*/
/* DiskBack */
/*--------------------------------------------------------------------------*/

class DiskBack : public InterruptHandler {

public :

  DiskBack();

  virtual void handle_interrupt(REGS *_r);
  /* This must be installed as the interrupt handler for the timer 
     when the system gets initialized. (e.g. in "kernel.C")  
  */

};

#endif
