/*
     File        : file_system.C

     Author      : Jin Huang
     Date	: 05/01/2021

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
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
#include "file_system.H"
#include "utils.H"

int FileSystem::super_blockid = 0;
int FileSystem::dbitmap_blockid = 1;
int FileSystem::dir_blockid = 2;
int FileSystem::inode_blockid = 4;
int FileSystem::data_blockid = 260;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem()
: freeblock_num(0), datablock_num(0), size(0), autoname(1), freeinode_num(0), disk(0)
{
    Console::puts("In file system constructor.\n");
	memset(super_blockbuf, 0, BLOCKSIZE);
	memset(dbitmap, 0, BLOCKSIZE);
	memset(dir_blockbuf, 0, DBLOCKSIZE);
	for (int i=0;i<MAXFILENUM;++i)	dir_name[i]=0;
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM :FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system form disk\n");
	if (!_disk) {
		Console::puts("invalid disk!\n");
		assert(false);
	}
	disk = _disk;

	// Load super block, retrive #freeblock, #datablock and size
	disk->read(super_blockid, super_blockbuf);
	freeblock_num = (super_blockbuf[3]<<24) + (super_blockbuf[2]<<16)
					+ (super_blockbuf[1]<<8) + super_blockbuf[0];
	datablock_num = (super_blockbuf[7]<<24) + (super_blockbuf[6]<<16) 
					+ (super_blockbuf[5]<<8) + super_blockbuf[4];
	freeinode_num = (super_blockbuf[11]<<24) + (super_blockbuf[10]<<16)
					+ (super_blockbuf[9]<<8) + super_blockbuf[8];
	size = (super_blockbuf[15]<<24) + (super_blockbuf[14]<<16)
			+ (super_blockbuf[13]<<8) + super_blockbuf[12];

	Console::puts("#freeblock=");Console::puti(freeblock_num);
	Console::puts("\n#datablock=");Console::puti(datablock_num);
	Console::puts("\nfreeinode_num=");Console::puti(freeinode_num);
	Console::puts("\nsize=");Console::puti(size);Console::puts("\n");


	if (size <= MINSIZE) { // MINSIZE 260*512=133120
		Console::puts("size must be greater than 260*512=133120 B\n");
		assert(false);
	}

	if (freeblock_num > datablock_num) {
		Console::puts("freeblock_num=");Console::puti(freeblock_num);
		Console::puts(" is greater than datablock_num=");Console::puti(datablock_num);
		Console::puts("\n");
	}

	// Load dbitmap
	disk->read(dbitmap_blockid, dbitmap);

	// Load directory block and sync dir_name table
	disk->read(dir_blockid, dir_blockbuf);
	disk->read(dir_blockid+1, dir_blockbuf+BLOCKSIZE);
	for (int i=0;i<MAXFILENUM;++i) {
		int name = (dir_blockbuf[4*i+3]<<24) + (dir_blockbuf[4*i+2]<<16)
			+ (dir_blockbuf[4*i+1]<<8) + dir_blockbuf[4*i];

		dir_name[i] = name;
	}

	return true;

}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
    Console::puts("formatting disk\n");

	if (_size <= MINSIZE) { // MINSIZE 260*512=133120
		Console::puts("size must be greater than 260*512=133120 B\n");
		assert(false);
	}

	if (!_disk) {
		Console::puts("invalid simpledisk \n");
		assert(false);
	}

	// Write Super Block
	int _datablock_num = (_size-MINSIZE)/512;
	int _freeblock_num = _datablock_num;
	unsigned char blockbuf[BLOCKSIZE];
	memset(blockbuf, 0, BLOCKSIZE);
	// #freeblock
	blockbuf[0] = (unsigned char)(_freeblock_num & 0xFF);
	blockbuf[1] = (unsigned char)((_freeblock_num>>8) & 0xFF);
	blockbuf[2] = (unsigned char)((_freeblock_num>>16) & 0xFF);
	blockbuf[3] = (unsigned char)((_freeblock_num>>24) & 0xFF);
	// #datablock
	blockbuf[4] = (unsigned char)(_datablock_num & 0xFF);
	blockbuf[5] = (unsigned char)((_datablock_num>>8) & 0xFF);
	blockbuf[6] = (unsigned char)((_datablock_num>>16) & 0xFF);
	blockbuf[7] = (unsigned char)((_datablock_num>>24) & 0xFF);
	// #freeinode
	blockbuf[8] = (unsigned char)(MAXINODENUM & 0xFF);
	blockbuf[9] = (unsigned char)((MAXINODENUM>>8) & 0xFF);
	blockbuf[10] = (unsigned char)((MAXINODENUM>>16) & 0xFF);
	blockbuf[11] = (unsigned char)((MAXINODENUM>>24) & 0xFF);
	// size of FS
	blockbuf[12] = (unsigned char)(_size & 0xFF);
	blockbuf[13] = (unsigned char)((_size>>8) & 0xFF);
	blockbuf[14] = (unsigned char)((_size>>16) & 0xFF);
	blockbuf[15] = (unsigned char)((_size>>24) & 0xFF);

	_disk->write(super_blockid, blockbuf);

	// Write DBITMAP
	memset(blockbuf, 0, BLOCKSIZE);
	_disk->write(dbitmap_blockid, blockbuf);

	// Write DIRECTORY BLOCK
	//memset(blockbuf, 0, BLOCKSIZE);
	_disk->write(dir_blockid, blockbuf);
	_disk->write(dir_blockid+1, blockbuf);

	// Write inode blocks
	//memset(blockbuf, 0, BLOCKSIZE);
	for (int i=0;i<256;++i)	_disk->write(inode_blockid+i, blockbuf);

	return true;

}

File * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file\n");

	if (_file_id < 1) {
		Console::puts("Invalid file_id! Valid file_id should be a positive int!\n");
		return NULL;
	}

	for (int i=0;i<MAXFILENUM;++i) {
		if (_file_id == dir_name[i]) {
			File *fp = new File(i);
			return fp;
		}
	}
	return NULL;

}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");
	if (_file_id < 1) {
		Console::puts("Invalid file_id! Valid file_id should be a positive int!\n");
		return false;
	}

	if (LookupFile(_file_id)) {
		Console::puts("file_id=");
		Console::puti(_file_id);
		Console::puts(" already exists!\n");
		return false;
	}

	if (freeinode_num == 0) {
		Console::puts("No free inode!\n");
		return false;
	}

	// create this file in FS
	int name = _file_id;
	int inode = -1;
	for (int i=0;i<MAXINODENUM;++i) {
		if (dir_name[i]==0) {
			inode = i;
			dir_name[i] = name;
			break;
		}
	}
	if (inode<0 || inode>=MAXINODENUM) {
		Console::puts("exception inode=");Console::puti(inode);
		Console::puts("\n");
		return false;
	}

	unsigned char blockbuf[BLOCKSIZE];
	// (1)inodeblock
	int inodeblock_no = inode_blockid + inode;
	memset(blockbuf, 0, BLOCKSIZE);
	disk->write(inodeblock_no, blockbuf);

	// (2)directory block
	update_dirblock(inode);

	// (3)superblock
	--freeinode_num;
	update_superblock();

	return true;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");
	if (_file_id < 1) {
		Console::puts("Invalid file_id! Valid file_id should be a positive int!\n");
		return false;
	}

	// lookup the _file_id in dir_name
	int flag = 0;
	for (int i=0;i<MAXFILENUM;++i) {
		if (dir_name[i] == _file_id) {
			flag = 1;

			unsigned char ibbuf[BLOCKSIZE];
			disk->read(inode_blockid+i, ibbuf);
			int db_num = ibbuf[0] + (ibbuf[1]<<8) + (ibbuf[2]<<16) 
						+ (ibbuf[3]<<24);

			if (db_num<0 || db_num>127) {
				Console::puts("Invalid db_num=");Console::puti(db_num);
				Console::puts("\n");
				assert(false);
				return false;
			}

			unsigned char emptybuf[BLOCKSIZE];
			memset(emptybuf, 0, BLOCKSIZE);
			for (int j=1;j<=db_num;++j) {
				int bi = j<<2;
				int dbno = ibbuf[bi] + (ibbuf[bi+1]<<8) + (ibbuf[bi+2]<<16)
							+ (ibbuf[bi+3]<<24);
				cleardbit(dbno);
				disk->write(data_blockid+dbno, emptybuf);
				++freeblock_num;
			}
			// 1. for not empty file, so update dbitmap
			if (db_num)	sync_dbitmap();

			// 2. update directory block
			dir_name[i] = 0;
			++freeinode_num;
			update_dirblock(i);

			// 3. clear the inode block
			memset(ibbuf, 0, BLOCKSIZE);
			disk->write(inode_blockid+i, ibbuf);

			// 4. update super block: #freeblock_num and #freeinode
			update_superblock();
			
			break;
		} // end of dealing with _file_id

	}

	if (!flag) {
		Console::puts("_file_id=");Console::puti(_file_id);
		Console::puts(" not exists!\n");
		return false;
	}

	return true;
}

unsigned char FileSystem::getdbit(unsigned int index) {
	if (index >= datablock_num) {
		Console::puts("index=");Console::puti(index);
		Console::puts(" is larger than datablock_num=");
		Console::puti(datablock_num);Console::puts("\n");
		assert(false);
	}
	int nbyte = index >> 3;
	int offset = index & 0x7;
	int rshift = 7 - offset;
	return (dbitmap[nbyte]>>rshift) & 1;
}

void FileSystem::setdbit(unsigned int index) {
	if (index >= datablock_num) {
		Console::puts("index=");Console::puti(index);
		Console::puts(" is larger than datablock_num=");
		Console::puti(datablock_num);Console::puts("\n");
		assert(false);
	}
	int nbyte = index >> 3;
	int offset = index & 0x7;
	int lshift = 7 - offset;
	dbitmap[nbyte] |= (1<<lshift);
}

void FileSystem::cleardbit(unsigned int index) {
	if (index >= datablock_num) {
		Console::puts("index=");Console::puti(index);
		Console::puts(" is larger than datablock_num=");
		Console::puti(datablock_num);Console::puts("\n");
		assert(false);
	}
	int nbyte = index >> 3;
	int offset = index & 0x7;
	int rshift = 7 - offset;
	if ((dbitmap[nbyte]>>rshift) & 1)
		dbitmap[nbyte] ^= (0x80>>offset);
}

void FileSystem::sync_dbitmap() {
	disk->write(dbitmap_blockid, dbitmap);
}

void FileSystem::update_superblock() {

	// (5.1)#freeblock
	super_blockbuf[0] = (unsigned char)(freeblock_num & 0xFF);
	super_blockbuf[1] = (unsigned char)((freeblock_num>>8) & 0xFF);
	super_blockbuf[2] = (unsigned char)((freeblock_num>>16) & 0xFF);
	super_blockbuf[3] = (unsigned char)((freeblock_num>>24) & 0xFF);
	// (5.2)#datablock
	super_blockbuf[4] = (unsigned char)(datablock_num & 0xFF);
	super_blockbuf[5] = (unsigned char)((datablock_num>>8) & 0xFF);
	super_blockbuf[6] = (unsigned char)((datablock_num>>16) & 0xFF);
	super_blockbuf[7] = (unsigned char)((datablock_num>>24) & 0xFF);
	// (5.3)#freeinode
	super_blockbuf[8] = (unsigned char)(freeinode_num & 0xFF);
	super_blockbuf[9] = (unsigned char)((freeinode_num>>8) & 0xFF);
	super_blockbuf[10] = (unsigned char)((freeinode_num>>16) & 0xFF);
	super_blockbuf[11] = (unsigned char)((freeinode_num>>24) & 0xFF);
	// (5.4)size
	super_blockbuf[12] = (unsigned char)(size & 0xFF);
	super_blockbuf[13] = (unsigned char)((size>>8) & 0xFF);
	super_blockbuf[14] = (unsigned char)((size>>16) & 0xFF);
	super_blockbuf[15] = (unsigned char)((size>>24) & 0xFF);

	disk->write(super_blockid, super_blockbuf);

}

void FileSystem::update_dirblock(int inode) {
	if (inode<=127 && inode>=0) {
		int dir1_index = inode<<2;
		dir_blockbuf[dir1_index] = (unsigned char)(dir_name[inode] & 0xFF);
		dir_blockbuf[dir1_index+1] = 
			(unsigned char)((dir_name[inode]>>8) & 0XFF);
		dir_blockbuf[dir1_index+2] = 
			(unsigned char)((dir_name[inode]>>16) & 0XFF);
		dir_blockbuf[dir1_index+3] = 
			(unsigned char)((dir_name[inode]>>24) & 0XFF);

		disk->write(dir_blockid, dir_blockbuf);

	}else if (inode>=128 && inode<MAXINODENUM) {
		int dir2_index = inode<<2;
		dir_blockbuf[dir2_index] = (unsigned char)(dir_name[inode] & 0xFF);
		dir_blockbuf[dir2_index+1] = 
			(unsigned char)((dir_name[inode]>>8) & 0XFF);
		dir_blockbuf[dir2_index+2] = 
			(unsigned char)((dir_name[inode]>>16) & 0XFF);
		dir_blockbuf[dir2_index+3] = 
			(unsigned char)((dir_name[inode]>>24) & 0XFF);

		disk->write(dir_blockid+1, dir_blockbuf+BLOCKSIZE);

	}else {
		Console::puts("Invalid inode=");Console::puti(inode);
		Console::puts("\n");
		assert(false);
	}
	
}
/*
void FileSystem::new_inodeblock(int inodeblock_no) {

	unsigned char buf[BLOCKSIZE];
	memset(buf, 0, BLOCKSIZE);
	// (2.1) #datablock in inode
	int inode_datablock_num = 0;
	buf[0] = (unsigned char)(inode_datablock_num & 0xFF);
	buf[1] = (unsigned char)((inode_datablock_num>>8) & 0xFF);
	buf[2] = (unsigned char)((inode_datablock_num>>16) & 0xFF);
	buf[3] = (unsigned char)((inode_datablock_num>>24) & 0xFF);
	// (2.2) 1st absolute datablock number in inode
	buf[4] = (unsigned char)(datablock_no & 0xFF);
	buf[5] = (unsigned char)((datablock_no>>8) & 0xFF);
	buf[6] = (unsigned char)((datablock_no>>16) & 0xFF);
	buf[7] = (unsigned char)((datablock_no>>24) & 0xFF);
	// (2.3) write this inode block buf into disk
	disk->write(inodeblock_no, buf);

}
*/

int FileSystem::getDBN(int inode_no) {
	if (inode_no<0 || inode_no>MAXINODENUM) {
		Console::puts("Invalid inode_no=");Console::puti(inode_no);
		Console::puts("\n");
		assert(false);
	}
	
	unsigned char buf[BLOCKSIZE];
	disk->read(inode_blockid+inode_no, buf);
	int db_num = buf[0] + (buf[1]<<8) + (buf[2]<<16) + (buf[3]<<24);
	
	if (db_num<0 || db_num>MAXFDBNUM) {
		Console::puts("Invalid db_num=");Console::puti(db_num);
		Console::puts("\n");
		assert(false);
	}

	return db_num;

}

void FileSystem::FreeData(int inode) {
	// empty the corresponding inode block and the bit in data bitmap
	// then update the #freeblock in super block

	unsigned char blockbuf[BLOCKSIZE];
	disk->read(inode_blockid+inode, blockbuf);
	
	int dbn = blockbuf[0] + (blockbuf[1]<<8)
		+ (blockbuf[2]<<16) + (blockbuf[3]<<24);
	
	int db_index[MAXFDBNUM];
	int ii;
	for (int i=0;i<dbn;++i) {
		ii = (i+1)<<2;
		db_index[i] = blockbuf[ii] + (blockbuf[ii+1]<<8)
			+ (blockbuf[ii+2]<<16) + (blockbuf[ii+3]<<24);
	}

	// (1) empty the corresponding inode block
	memset(blockbuf, 0, BLOCKSIZE);
	disk->write(inode_blockid+inode, blockbuf);

	// (2) sync the db bitmap
	for (int i=0;i<dbn;++i)	cleardbit(db_index[i]);
	sync_dbitmap();

	// (3) update super block
	freeblock_num += dbn;
	update_superblock();

}

bool FileSystem::ApplyDB(int inode, int dbn) {
	if (!InodeExist(inode)) {
		Console::puts("Invalid inode=");Console::puti(inode);
		Console::puts(" to ApplyDB!\n");
		assert(false);
		return false;
	}
	if (!ValidInodeDBN(dbn)) {
		Console::puts("Invalid dbn=");Console::puti(dbn);
		Console::puts(" to ApplyDB!\n");
		assert(false);
		return false;
	}

	if (dbn > freeblock_num) {
		Console::puts("DataBlock used up!\n");
		return false;
	}

	// Load the corresponding inode blockbuf
	unsigned char inode_blockbuf[BLOCKSIZE];
	disk->read(inode_blockid+inode, inode_blockbuf);

	// get the #datablock of this inode block
	int original_dbn = inode_blockbuf[0] + (inode_blockbuf[1]<<8)
					+ (inode_blockbuf[2]<<16) + (inode_blockbuf[3]<<24);
	
	if (!ValidInodeDBN(original_dbn)) {
		Console::puts("Invalid original_dbn=");Console::puti(original_dbn);
		Console::puts("\n");
		assert(false);
		return false;
	}
	
	if (!ValidInodeDBN(original_dbn+dbn)) {
		Console::puts("DataBlockNumber=");Console::puti(original_dbn+dbn);
		Console::puts(" exceeds!\n");
		assert(false);
		return false;
	}
	// 1.1 update #datablock in inodebuf
	int new_dbn = original_dbn + dbn;
	inode_blockbuf[0] = (unsigned char)(new_dbn & 0xFF);
	inode_blockbuf[1] = (unsigned char)((new_dbn>>8) & 0xFF);
	inode_blockbuf[2] = (unsigned char)((new_dbn>>16) & 0xFF);
	inode_blockbuf[3] = (unsigned char)((new_dbn>>24) & 0xFF);
	
	// 1.2 update dblock_no inside this inodebuf
	int ib_index = original_dbn+1;
	int bufindex;
	unsigned char emptybuf[BLOCKSIZE];
	memset(emptybuf, 0, BLOCKSIZE);
	int rj = 0;
	for (int i=1;i<=dbn;++i) {
		if (rj >= datablock_num) {
			Console::puts("Exception lack of datablock!\n");
			assert(false);
			return false;
		}
		for (int j=rj;j<datablock_num;++j) {
			if (!getdbit(j)) {
				// (1) set bitmap
				setdbit(j);
				CleanDB(j);
				// (2) update on inode blockbuf
				bufindex = (ib_index<<2);
				inode_blockbuf[bufindex] = (unsigned char)(j & 0xFF);
				inode_blockbuf[bufindex+1] = (unsigned char)((j>>8) & 0xFF);
				inode_blockbuf[bufindex+2] = (unsigned char)((j>>16) & 0xFF);
				inode_blockbuf[bufindex+3] = (unsigned char)((j>>24) & 0xFF);
				++ib_index;
				rj = j+1;
				break;
			}
		}

	}
	// 1.3 sync the dbitmap on disk
	disk->write(dbitmap_blockid, dbitmap);
	// 1.4 sync the blockbuffer on disk
	disk->write(inode_blockid+inode, inode_blockbuf);

	// 2. update freeblock_num in superblock
	freeblock_num -= dbn;
	super_blockbuf[0] = (unsigned char)(freeblock_num & 0xFF);
	super_blockbuf[1] = (unsigned char)((freeblock_num>>8) & 0xFF);
	super_blockbuf[2] = (unsigned char)((freeblock_num>>16) & 0xFF);
	super_blockbuf[3] = (unsigned char)((freeblock_num>>24) & 0xFF);
	// sync the superblock on disk
	disk->write(super_blockid, super_blockbuf);
	
	return true;

}

bool FileSystem::InodeExist(int inode) {
	if (!ValidInode(inode)) {
		Console::puts("Invalid inode=");Console::puti(inode);
		Console::puts("\n");
		assert(false);
		return false;
	}
	return (dir_name[inode]>0);
}

bool FileSystem::ValidInode(int inode) {
	return (inode>=0) && (inode<MAXINODENUM);
}

bool FileSystem::ValidInodeDBN(int dbn) {
	return (dbn>=0) && (dbn<=MAXFDBNUM);
}

bool FileSystem::ValidDBNO(int dbno) {
	return (dbno>=0) && (dbno<=MAXDBNO); // MAXDBNO 4095
}

void FileSystem::WriteFile(int inode, int pos, int n, const char *buf) {
	
	if (!ValidInode(inode)) {
		Console::puts("Invalid inode=");Console::puti(inode);
		Console::puts("\n");
		assert(false);return;
	}

	// Load iblockbuf
	unsigned char iblockbuf[BLOCKSIZE];
	disk->read(inode_blockid+inode, iblockbuf);

	// locate the block to write
	int pos_baseblock = pos>>9;
	int pos_blockoffset = pos & 0x1FF;

	int iblock_index = 1 + pos_baseblock;
	unsigned char databuf[BLOCKSIZE];

	// CASE1: write operation only inside a datablock
	if (pos_blockoffset+n-1 <= MAXINDBOFFSET) {
		int ibi = iblock_index<<2;
		int db_no = iblockbuf[ibi] + (iblockbuf[ibi+1]<<8)
				+ (iblockbuf[ibi+2]<<16) + (iblockbuf[ibi+3]<<24);

		if (!ValidDBNO(db_no)) {
			Console::puts("Invalid db_no=");Console::puti(db_no);
			Console::puts("\n");
			assert(false);return;
		}
		// load databuf from disk
		disk->read(data_blockid+db_no, databuf);
		memcpy(databuf+pos_blockoffset, buf, n);
		// write sync databuf to disk
		disk->write(data_blockid+db_no, databuf);

	}else {
	// CASE2: write operation across datablocks
		// 1. finish writing up the current DB
		int inblock_remain = MAXINDBOFFSET - pos_blockoffset + 1;
		// load the current db block
		int ibi = iblock_index<<2;
		int db_no = iblockbuf[ibi] + (iblockbuf[ibi+1]<<8)
				+ (iblockbuf[ibi+2]<<16) + (iblockbuf[ibi+3]<<24);

		if (!ValidDBNO(db_no)) {
			Console::puts("Invalid db_no=");Console::puti(db_no);
			Console::puts("\n");
			assert(false);return;
		}

		disk->read(data_blockid+db_no, databuf);
		memcpy(databuf+pos_blockoffset, buf, inblock_remain);
		disk->write(data_blockid+db_no, databuf);

		// 2. Write the rest data blocks
		int write_count = n - inblock_remain;
		const char *p = buf + inblock_remain;
		int pn = 0;
		while (write_count > 0) {
			++iblock_index;
			ibi = iblock_index<<2;
			int db_no = iblockbuf[ibi] + (iblockbuf[ibi+1]<<8)
					+ (iblockbuf[ibi+2]<<16) + (iblockbuf[ibi+3]<<24);

			if (!ValidDBNO(db_no)) {
				Console::puts("Invalid db_no=");Console::puti(db_no);
				Console::puts("\n");
				assert(false);return;
			}

			if ((write_count>>9) > 0) {
				memcpy(databuf, p + pn*BLOCKSIZE, BLOCKSIZE);
				disk->write(data_blockid+db_no, databuf);
				write_count -= BLOCKSIZE;
				++pn;
			}else {
				// In this case, will just write the dblock from the begining
				// Notice, we should load databuf
				disk->read(data_blockid+db_no, databuf);
				memcpy(databuf, p + pn*BLOCKSIZE, write_count);
				disk->write(data_blockid+db_no, databuf);
				write_count = 0;
				break;
			}

		}

	} // end of CASE2

}

void FileSystem::CleanDB(int dbno) {

	unsigned char emptybuf[BLOCKSIZE];
	memset(emptybuf, 0, BLOCKSIZE);
	disk->write(data_blockid+dbno, emptybuf);

}

int FileSystem::ReadFile(int inode, int pos, int maxpos, int n, char *buf) {

	if (!ValidInode(inode)) {
		Console::puts("Invalid inode=");Console::puti(inode);
		Console::puts("\n");
		assert(false);
		return 0;
	}


	// Load inodeblock buf
	unsigned char inodeblockbuf[BLOCKSIZE];
	disk->read(inode_blockid+inode, inodeblockbuf);

	int total_db = inodeblockbuf[0] + (inodeblockbuf[1]<<8)
		+ (inodeblockbuf[2]<<16) + (inodeblockbuf[3]<<24);
	if (total_db<0 || total_db>128) {
		Console::puts("Invalid total_db=");Console::puti(total_db);
		Console::puts("\n");
		assert(false);
		return 0;
	}
	if (total_db==0) {
		Console::puts("Read empty file\n");
		return 0;
	}
	int size = total_db * BLOCKSIZE;

	if (size != maxpos+1) {
		Console::puts("Invalid size=");Console::puti(size);
		Console::puts(" maxpos=");Console::puti(maxpos);Console::puts("\n");
		assert(false);
		return 0;
	}

	if (pos<0 || pos>maxpos) {
		Console::puts("Invalid pos=");Console::puti(pos);
		Console::puts(" maxpos=");Console::puti(maxpos);Console::puts("\n");
		assert(false);
		return 0;
	}

	int pos_inblockoffset = pos & 0x1FF;
	int pos_ininodeindex = (pos>>9) + 1;
	if (pos_ininodeindex > total_db) {
		Console::puts("Invalid pos_ininodeindex=");
		Console::puti(pos_ininodeindex);
		Console::puts(" total_db=");Console::puti(total_db);
		Console::puts("\n");
		return 0;
	}
	int ibi = pos_ininodeindex<<2;

	int dbno = inodeblockbuf[ibi] + (inodeblockbuf[ibi+1]<<8)
			+ (inodeblockbuf[ibi+2]<<16) + (inodeblockbuf[ibi+3]<<24);

	if (!ValidDBNO(dbno)) {
		Console::puts("Invalid dbno=");Console::puti(dbno);
		Console::puts("\n");
		assert(false);
		return 0;
	}
	
	unsigned char datablockbuf[BLOCKSIZE];
	// CASE1: read operation only inside a datablock
	if (pos_inblockoffset+n-1 <= MAXINDBOFFSET) {
		disk->read(data_blockid+dbno, datablockbuf);
		memcpy(buf, datablockbuf + pos_inblockoffset, n);
		return n;

	}else { // CASE2: read operation across datablocks
		int inblock_left = MAXINDBOFFSET - pos_inblockoffset + 1;
		disk->read(data_blockid+dbno, datablockbuf);
		memcpy(buf, datablockbuf + pos_inblockoffset, inblock_left);

		char *destbuf = buf + inblock_left;
		int count = inblock_left;
		int buf_left = n-inblock_left;
		pos_ininodeindex++;

		for (int i=pos_ininodeindex;i<=total_db;++i) {
			if (!buf_left)	break;
			if (buf_left>>9) { // more than 1 block need to read into buf
				ibi = i<<2;
				dbno = inodeblockbuf[ibi] + (inodeblockbuf[ibi+1]<<8)
				+ (inodeblockbuf[ibi+2]<<16) + (inodeblockbuf[ibi+3]<<24);

				disk->read(data_blockid+dbno, datablockbuf);
				memcpy(destbuf, datablockbuf, BLOCKSIZE);
				buf_left -= BLOCKSIZE;
				count += BLOCKSIZE;
				destbuf += BLOCKSIZE;
			}else { // buf_left only occupies a small part of db, from beg
				ibi = i<<2;
				dbno = inodeblockbuf[ibi] + (inodeblockbuf[ibi+1]<<8)
				+ (inodeblockbuf[ibi+2]<<16) + (inodeblockbuf[ibi+3]<<24);
				
				disk->read(data_blockid+dbno, datablockbuf);
				memcpy(destbuf, datablockbuf, buf_left);
				count += buf_left;
				buf_left = 0;
				break;
			}

		}
		return count;

	}

}




