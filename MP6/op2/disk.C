/*
     File        : disk.c

     Author      : Jin Huang
     Modified    : 04/16/2021

     Description : Disk implementation
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "disk.H"
#include "machine.H"
#include "scheduler.H"

/* Declaration */
extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

Disk::Disk(DISK_ID _disk_id, unsigned int _size)
 : SimpleDisk(_disk_id, _size) {}

/*--------------------------------------------------------------------------*/
/* DISK CONFIGURATION */
/*--------------------------------------------------------------------------*/

unsigned int Disk::size() {
	return disk_size;
}

/*--------------------------------------------------------------------------*/
/* DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void Disk::issue_operation(DISK_OPERATION _op, unsigned long _block_no) {

	Machine::outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
	Machine::outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
	Machine::outportb(0x1F3, (unsigned char)_block_no);
                         /* send low 8 bits of block number */
	Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
                         /* send next 8 bits of block number */
	Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
                         /* send next 8 bits of block number */
	Machine::outportb(0x1F6, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (disk_id << 4));
                         /* send drive indicator, some bits, 
                            highest 4 bits of block no */

	Machine::outportb(0x1F7, (_op == READ) ? 0x20 : 0x30);
}

bool Disk::is_ready() {
	return ((Machine::inportb(0x1F7) & 0x08) != 0);
}

void Disk::read(unsigned long _block_no, unsigned char * _buf) {

	if (Machine::interrupts_enabled())
		Machine::disable_interrupts();

	SYSTEM_SCHEDULER->add_wait(Thread::CurrentThread());
	issue_operation(READ, _block_no);
	SYSTEM_SCHEDULER->yield(); 

	/* read data from port */
	int i;
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = Machine::inportw(0x1F0);
	    _buf[i*2]   = (unsigned char)tmpw;
		_buf[i*2+1] = (unsigned char)(tmpw >> 8);
	}

	if (!Machine::interrupts_enabled())
		Machine::enable_interrupts();

}

void Disk::write(unsigned long _block_no, unsigned char * _buf) {
	
	if (Machine::interrupts_enabled())
		Machine::disable_interrupts();
	
	issue_operation(WRITE, _block_no);
	if (is_ready()) {
	// just ready for to be written so we could just write to disk directly
	// and unnecessary to deal with it in InterruptHandler
		int i; 
		unsigned short tmpw;
		for (i = 0; i < 256; i++) {
			tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
			Machine::outportw(0x1F0, tmpw);
		}

	}else {
	// in this case, Interrupt14 will occur
		SYSTEM_SCHEDULER->add_wait(Thread::CurrentThread());
		SYSTEM_SCHEDULER->yield();
		// write data to port
		int i; 
		unsigned short tmpw;
		for (i = 0; i < 256; i++) {
			tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
			Machine::outportw(0x1F0, tmpw);
		}
	}
	
	if (!Machine::interrupts_enabled())
		Machine::enable_interrupts();

}
