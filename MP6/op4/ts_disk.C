/*
     File        : ts_disk.c

     Author      : Jin Huang
     Modified    : 04/16/2021

     Description : TSDisk implementation
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "lock.H"
#include "ts_disk.H"
#include "machine.H"
#include "scheduler.H"

//#define DISK_BLOCK_NUM 20480

extern Scheduler * SYSTEM_SCHEDULER;
extern int mutex;

//extern int block_mutex[DISK_BLOCK_NUM];

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

TSDisk::TSDisk(DISK_ID _disk_id, unsigned int _size)
 : SimpleDisk(_disk_id, _size) {}

/*--------------------------------------------------------------------------*/
/* DISK CONFIGURATION */
/*--------------------------------------------------------------------------*/

unsigned int TSDisk::size() {
	return disk_size;
}

/*--------------------------------------------------------------------------*/
/* TSDISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void TSDisk::issue_operation(DISK_OPERATION _op, unsigned long _block_no) {

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

bool TSDisk::is_ready() {
	return ((Machine::inportb(0x1F7) & 0x08) != 0);
}

void TSDisk::wait_until_ready() {
	
	while (!is_ready()) {
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
		SYSTEM_SCHEDULER->yield();
	}

}

void TSDisk::read(unsigned long _block_no, unsigned char * _buf) {
	
	lock(&mutex);
	Console::puts("Reading block ");Console::puti(_block_no);Console::puts("\n");
	issue_operation(READ, _block_no);
	wait_until_ready();
	/* read data from port */
	int i;
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = Machine::inportw(0x1F0);
	    _buf[i*2]   = (unsigned char)tmpw;
		_buf[i*2+1] = (unsigned char)(tmpw >> 8);
	}
	Console::puts("Finish reading block ");Console::puti(_block_no);Console::puts("\n");
	unlock(&mutex);

}

void TSDisk::write(unsigned long _block_no, unsigned char * _buf) {
	
	lock(&mutex);
	Console::puts("Writing block ");Console::puti(_block_no);Console::puts("\n");
	issue_operation(WRITE, _block_no);
	wait_until_ready();
	/* write data to port */
	int i; 
	unsigned short tmpw;
	for (i = 0; i < 256; i++) {
		tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
		Machine::outportw(0x1F0, tmpw);
	}
	Console::puts("Finish writing block ");Console::puti(_block_no);Console::puts("\n");
	unlock(&mutex);

}
