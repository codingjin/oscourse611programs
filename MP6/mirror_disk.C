/*
	File : mirror_disk.C
	Author : Jin Huang
	Data : 04/14/2021
*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "simple_disk.H"
#include "mirror_disk.H"
#include "scheduler.H"

extern Scheduler * SYSTEM_SCHEDULER;

/* Constructor */
MirrorDisk::MirrorDisk(DISK_ID _disk_id, unsigned int _size)
 : SimpleDisk(_disk_id, _size) {}


MirrorDisk::MirrorDisk(unsigned int _size)
 : SimpleDisk(MASTER, _size) {}

/* MirrorDisk Functions */

void MirrorDisk::issue_read(unsigned long _block_no)
{
	// 1. issue read to MASTER
	Machine::outportb(0x1F1, 0x00); // send NULL to port 0x1F1
	Machine::outportb(0x1F2, 0x01); // send sector count to port 0x1F2
	Machine::outportb(0x1F3, (unsigned char)_block_no);  // low 8 bits
	Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
	Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
	Machine::outportb(0x1F6, ((unsigned char)(_block_no>>24))&0x0F | 0xE0 | (MASTER<<4));
	Machine::outportb(0x1F7, 0x20);

	// 2. issue read to SLAVE
	Machine::outportb(0x1F1, 0x00); // send NULL to port 0x1F1
	Machine::outportb(0x1F2, 0x01); // send sector count to port 0x1F2
	Machine::outportb(0x1F3, (unsigned char)_block_no);  // low 8 bits
	Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
	Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
	Machine::outportb(0x1F6, ((unsigned char)(_block_no>>24))&0x0F | 0xE0 | (SLAVE<<4));
	Machine::outportb(0x1F7, 0x20);
}

void MirrorDisk::issue_write(DISK_ID _disk_id, unsigned long _block_no)
{
	Machine::outportb(0x1F1, 0x00); // send NULL to port 0x1F1
	Machine::outportb(0x1F2, 0x01); // send sector count to port 0x1F2
	Machine::outportb(0x1F3, (unsigned char)_block_no);  // low 8 bits
	Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
	Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
	Machine::outportb(0x1F6, ((unsigned char)(_block_no>>24))&0x0F | 0xE0 | (_disk_id<<4));
	Machine::outportb(0x1F7, 0x30);
}

bool MirrorDisk::is_ready()
{
	return ((Machine::inportb(0x1F7) & 0x08) != 0);
}

void MirrorDisk::wait_until_ready()
{
	while (!is_ready()) {
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
		SYSTEM_SCHEDULER->yield();
	}
}

void MirrorDisk::read(unsigned long _block_no, unsigned char * _buf)
{
	issue_read(_block_no);
	wait_until_ready();

	unsigned short tmpw;
	for(int i=0;i<256;++i) {
		tmpw = Machine::inportw(0x1F0);
		_buf[i<<1] = (unsigned char)tmpw;
		_buf[i<<1 + 1] = (unsigned char)(tmpw>>8);
	}
}

void MirrorDisk::write(unsigned long _block_no, unsigned char * _buf)
{
	// issue write to MASTER
	issue_write(MASTER, _block_no);
	wait_until_ready();

	// write to MASTER
	unsigned short tmpw;
	for(int i=0;i<256;++i) {
		tmpw = _buf[i<<1] | (_buf[i<<1 + 1]<<8);
		Machine::outportw(0x1F0, tmpw);
	}

	// issue write to SLAVE
	issue_write(SLAVE, _block_no);
	wait_until_ready();
	
	// write to SLAVE
	for(int i=0;i<256;++i) {
		tmpw = _buf[i<<1] | (_buf[i<<1 + 1]<<8);
		Machine::outportw(0x1F0, tmpw);
	}
}

