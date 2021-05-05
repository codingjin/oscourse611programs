/*
     File        : disk.H

     Author      : Jin Huang
     Modified    : 04/16/2021

     Description : Disk system
*/

#ifndef DISK_H_
#define DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* Disk, supporting INTERRUPTS */
/*--------------------------------------------------------------------------*/

class Disk : public SimpleDisk {
private:

     DISK_ID      disk_id; // This disk is either MASTER or SLAVE

     unsigned int disk_size; // In Byte

     void issue_operation(DISK_OPERATION _op, unsigned long _block_no);
     
protected:
     /* -- HERE WE CAN DEFINE THE BEHAVIOR OF DERIVED DISKS */ 

     virtual bool is_ready();

     virtual void wait_until_ready() {
        while (!is_ready()) { /* wait */; }
     }

public:

   Disk(DISK_ID _disk_id, unsigned int _size); 

   unsigned int size();

   /* DISK OPERATIONS */

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

};

#endif
