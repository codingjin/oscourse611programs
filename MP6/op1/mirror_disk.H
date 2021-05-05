/*
	Class : MirrorDisk
	File : mirror_disk.H
	Author : Jin Huang
	Data : 04/13/2021
	Description : Mirrored Disk for Blocking Disk
*/

#ifndef _MIRRORED_DISK_H
#define _MIRRORED_DISK_H

#include "simple_disk.H"

class MirrorDisk : public SimpleDisk {

private:
	unsigned int disk_size; // in byte
	DISK_ID disk_id;

public:
	MirrorDisk(DISK_ID _disk_id, unsigned int _size);
	MirrorDisk(unsigned int _size); 

	void issue_read(unsigned long _block_no);

	void issue_write(DISK_ID _disk_id, unsigned long _block_no);
	
	virtual void read(unsigned long _block_no, unsigned char * _buf);

	virtual void write(unsigned long _block_no, unsigned char * _buf);

	virtual void wait_until_ready();

	virtual bool is_ready();

};

#endif
