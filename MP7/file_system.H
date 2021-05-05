/* 
    File: file_system.H

    Author: Jin Huang
            Department of Computer Science
            Texas A&M University
    Date  : 05/01/2021

    Description: Simple File System.
    

*/

#ifndef _FILE_SYSTEM_H_                   // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/
#define MINSIZE 133120
#define MAXINODENUM 256
#define MAXFDBNUM 127
#define MAXDBNO 4095
#define MAXINDBOFFSET 511
/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "file.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/
class File;
class FileSystem {

friend class File; /* -- not sure if we need this; feel free to delete */

private:
     /* -- DEFINE YOUR FILE SYSTEM DATA STRUCTURES HERE. */

	static int super_blockid; //{0};
	static int dbitmap_blockid; //{1};
	static int dir_blockid; //{2}; 2 blocks for dir_block
	static int inode_blockid; //{4};
	static int data_blockid; //{260};

	int freeblock_num, datablock_num, size, autoname, freeinode_num;
     
	SimpleDisk * disk;
	 
	unsigned char super_blockbuf[BLOCKSIZE]; // BLOCKSIZE=512
	unsigned char dbitmap[BLOCKSIZE]; //BLOCKSIZE=512(B) => 2MB maxsize
	unsigned char dir_blockbuf[DBLOCKSIZE]; // DBLOCKSIZE=1024
	int dir_name[MAXFILENUM]; // MAXFILENUM=256, inode/file number max=256
     
public:

    FileSystem();
    /* Just initializes local data structures. Does not connect to disk yet. */
    
    bool Mount(SimpleDisk * _disk);
    /* Associates this file system with a disk. Limit to at most one file system per disk.
     Returns true if operation successful (i.e. there is indeed a file system on the disk.) */
    
    static bool Format(SimpleDisk * _disk, unsigned int _size);
    /* Wipes any file system from the disk and installs an empty file system of given size. */
    
    File * LookupFile(int _file_id);
    /* Find file with given id in file system. If found, return the initialized
     file object. Otherwise, return null. */
    
    bool CreateFile(int _file_id);
    /* Create file with given id in the file system. If file exists already,
     abort and return false. Otherwise, return true. */
    bool DeleteFile(int _file_id); /* Delete file with given id in the file system; free any disk block occupied by the file. */

	unsigned char getdbit(unsigned int index);
	void setdbit(unsigned int index);
	void cleardbit(unsigned int index);

	void sync_dbitmap();

	void update_dirblock(int inode);

	void update_superblock();

	//void sync_inodeblock(int inodeblock_no, int datablock_no);

	int getDBN(int inode_no);

	void FreeData(int inode);

	bool ApplyDB(int inode, int dbn);

	bool InodeExist(int inode);

	bool ValidInode(int inode);

	bool ValidInodeDBN(int dbn);

	bool ValidDBNO(int dbno);

	void CleanDB(int dbno);

	void WriteFile(int inode, int pos, int n, const char *buf);

	int ReadFile(int inode, int pos, int maxpos, int n, char *buf);

};
#endif
