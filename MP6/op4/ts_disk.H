/*
     File        : ts_disk.H

     Author      : Jin Huang
     Modified    : 04/16/2021

     Description : Thread-Safe Disk system
*/

#ifndef _TS_DISK_H_
#define _TS_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

//#define DISK_BLOCK_NUM 10
//int block_mutex[DISK_BLOCK_NUM];

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* Thread-Safe Disk */
/*--------------------------------------------------------------------------*/

class TSDisk : public SimpleDisk {
private:

     DISK_ID      disk_id;            /* This disk is either MASTER or SLAVE */

     unsigned int disk_size;          /* In Byte */

     void issue_operation(DISK_OPERATION _op, unsigned long _block_no);
     /* Send a sequence of commands to the controller to initialize the READ/WRITE 
        operation. This operation is called by read() and write(). */ 
     
protected:
     /* -- HERE WE CAN DEFINE THE BEHAVIOR OF DERIVED DISKS */ 

     virtual bool is_ready();
     /* Return true if disk is ready to transfer data from/to disk, false otherwise. */

     virtual void wait_until_ready();
     
     /* Is called after each read/write operation to check whether the disk is
        ready to start transfering the data from/to the disk. */
     /* In SimpleDisk, this function simply loops until is_ready() returns TRUE.
        In more sophisticated disk implementations, the thread may give up the CPU
        and return to check later. */

public:

   TSDisk(DISK_ID _disk_id, unsigned int _size); 

   unsigned int size();

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

};

#endif
