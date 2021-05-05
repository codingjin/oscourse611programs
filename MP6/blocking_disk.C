/*
     File        : blocking_disk.c

     Author      : Jin Huang
     Modified    : 04/12/2021

     Description : Blocking_disk implementation

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
#include "simple_disk.H"
#include "blocking_disk.H"
#include "scheduler.H"

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) 
{}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::issue_operation(DISK_OPERATION _op, unsigned long _block_no) {
	Machine::outportb(0x1F1, 0x00);
	Machine::outportb(0x1F2, 0x01);
	Machine::outportb(0x1F3, (unsigned char)_block_no);
	Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
	Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
	Machine::outportb(0x1F6, ((unsigned char)(_block_no>>24)&0x0F) | 0xE0 | (disk_id<<4));
	Machine::outportb(0x1F7, (_op==READ)? 0x20:0x30);
}

bool BlockingDisk::is_ready() {
	return ((Machine::inportb(0x1F7)&0x08) != 0);
}

void BlockingDisk::wait_until_ready() {
	while (!is_ready()) {
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
		SYSTEM_SCHEDULER->yield();
	}
}

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
	issue_operation(READ, _block_no);
	wait_until_ready();
	// disk to read is ready now
	unsigned short tmpw;
	for (int i=0;i<256;++i) {
		tmpw = Machine::inportw(0x1F0);
		_buf[i*2] = (unsigned char)tmpw;
		_buf[i*2 + 1] = (unsigned char)(tmpw>>8);
	}
}

void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
	issue_operation(WRITE, _block_no);
	wait_until_ready();
	// disk to write is ready now
	unsigned short tmpw;
	for (int i=0;i<256;++i) {
		tmpw = _buf[2*i] | (_buf[2*i + 1] << 8);
		Machine::outportw(0x1F0, tmpw);
	}
}

