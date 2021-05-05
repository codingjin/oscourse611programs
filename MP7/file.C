/*
     File        : file.C

     Author      : Jin Huang

     Date : 05/01/2021

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(int _inode) 
: pos(0), maxpos(0), inode(_inode)
{
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
    Console::puts("In file constructor.\n");

	if (inode<0 || inode>=MAXINODENUM) {
		Console::puts("Invalid inode=");Console::puti(inode);
		Console::puts("\n");
		assert(false);
	}

	datablock_num = FILE_SYSTEM->getDBN(inode);
	size = datablock_num * BLOCKSIZE;
	maxpos = (size==0? 0:size-1);
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char * _buf) {
    Console::puts("reading from file\n");
	if (!(FILE_SYSTEM->ValidInode(inode))) {
		Console::puts("Invalid inode=");Console::puti(inode);
		Console::puts("\n");
		assert(false);
		return 0;
	}
	if (_n==0 || pos>maxpos)	return 0;

	if (!_buf) {
		Console::puts("_buf is NULL in File::Read()!\n");
		assert(false);
		return 0;
	}

	int count = FILE_SYSTEM->ReadFile(inode, pos, maxpos, _n, _buf);
	pos += count;
	return count;

}


void File::Write(unsigned int _n, const char * _buf) {
    Console::puts("writing to file\n");
	if (!(FILE_SYSTEM->ValidInode(inode))) {
		Console::puts("Invalid inode=");Console::puti(inode);
		Console::puts("\n");
		assert(false);return;
	}

	// 1. ApplyDB when necessary
	if (!size) { // size==0

		if (pos!=0 || maxpos!=0) {
			Console::puts("Invalid pos=");Console::puti(pos);
			Console::puts(" maxpos=");Console::puti(maxpos);
			Console::puts("\n");assert(false);
		}

		int remain = _n & 0x1FF;
		int extra = (remain==0? 0:1);
		int dbn = extra + (_n>>9);

		if (!(FILE_SYSTEM->ApplyDB(inode, dbn))) {
			Console::puts("Failed to ApplyDB!\n");
			assert(false);return;
		}
		size = dbn * BLOCKSIZE;
		maxpos = size - 1; //pos is still 0

	}else {

		int position_left = maxpos-pos+1;
		if (position_left < 0) {
			Console::puts("Exception pos=");Console::puti(pos);
			Console::puts(" maxpos=");Console::puti(maxpos);
			Console::puts("\n");
			assert(false);return;
		}
		if (position_left < _n) {
			int byte_need = _n - position_left;
			int remain = byte_need & 0x1FF;
			int extra = (remain==0? 0:1);
			int dbn = (byte_need>>9) + extra;
			
			if (!(FILE_SYSTEM->ApplyDB(inode, dbn))) {
				Console::puts("Failed to ApplyDB!\n");
				assert(false);return;
			}

			size += (dbn * BLOCKSIZE);
			maxpos = size-1;
		}

	}

	// 2. Write
	FILE_SYSTEM->WriteFile(inode, pos, _n, _buf);
	pos += _n;

}

void File::Reset() {
    Console::puts("reset current position in file\n");
    pos = 0;
}

void File::Rewrite() {
    Console::puts("erase content of file\n");
	if (size==0)	return;
	
	FILE_SYSTEM->FreeData(inode);
	size = 0;
	pos = 0;
	maxpos = 0;
	datablock_num = 0;

}


bool File::EoF() {
    Console::puts("testing end-of-file condition\n");

	if (size==0) {
		if (pos==0)	return true;
		else {
			Console::puts("Invalid pos=");Console::puti(pos);
			Console::puts(" when size==0\n");
			assert(false);
			return false;
		}
	}else {
		return pos == maxpos+1;
	}

}

int File::GetPos() {
	return pos;
}

int File::GetSize() {
	return size;
}

int File::GetDBN() {
	return datablock_num;
}

int File::GetInodeNo() {
	return inode;
}


