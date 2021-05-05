/* 
    File: disk_back.C

    Author: Jin Huang
    Date  : 04/17/2021

	Implementation of Disk Interrupt Handler
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "interrupts.H"
#include "disk_back.H"
#include "scheduler.H"
#include "machine.H"

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
DiskBack::DiskBack()
{}

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   DiskBack */
/*--------------------------------------------------------------------------*/

void DiskBack::handle_interrupt(REGS *_r) {

	if (Machine::interrupts_enabled())
		Machine::disable_interrupts();

	if (SYSTEM_SCHEDULER->wait_size()) {
		SYSTEM_SCHEDULER->raw_add(Thread::CurrentThread());
		SYSTEM_SCHEDULER->switch_to_wait();
	}

	if (!Machine::interrupts_enabled())
		Machine::enable_interrupts();

}

