/*
 File: vm_pool.C
 
 Author: Jin Huang
 Date  : 03/16/2021
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "page_table.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table):base_address(_base_address), size(_size), frame_pool(_frame_pool), page_table(_page_table) {
	
	base_address = base_address & 0xFFFFF000;
	index = 0;
	assert(size >= (Machine::PAGE_SIZE << 1));
	size_available = size - Machine::PAGE_SIZE;
	page_table->register_pool(this);
	map = (Map*)base_address;
	Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
	
	assert(_size > 0);
	if (index >= VMPool::MAX_INDEX) {
		Console::puts("out of map index for VMPool\n");
		return 0;
	}

	if (_size > size_available) {
		Console::puts("no space in VMPool!\n");
		return 0;
	}

	if (index == 0) {
		map[0].base = base_address + Machine::PAGE_SIZE;
		//map[0].base = base_address;
		map[0].size = _size;
	}else {
		map[index].base = map[index-1].base + map[index-1].size;
		map[index].size = _size;
	}
	++index;
	size_available -= _size;

	/*
    Console::puts("Allocated region of memory.\n");
	Console::puts("index=");
	Console::putui(index);
	Console::puts("\n");
	Console::puts("allocated addr=");
	Console::putui(map[index-1].base);
	*/
	return map[index-1].base;
}

void VMPool::release(unsigned long _start_address) {
	assert(index>0);
	/*
	Console::puts("want to release addr=");
	Console::putui(_start_address);
	Console::puts("\n");
	Console::puts("index=");
	Console::putui(index);
	Console::puts("\n");
	*/
	unsigned int flag = 0;
	unsigned int i = 0;
	for (;i<index;++i) {
		/*
		Console::puts("base=");
		Console::putui(map[i].base);
		Console::puts("\n");
		*/
		if (_start_address == map[i].base) {
			flag = 1;
			break;
		}
	}

	if (flag == 0) {
		Console::puts("The pool wanted to be released is not found!\n");
		return;
	}

	unsigned long release_size = map[i].size;
	size_available += release_size;
	unsigned long pagenum = release_size >> 12;
	unsigned long pgno = _start_address>>12;
	for (unsigned int j=0;j<pagenum;++j) {
		page_table->free_page(pgno);
		++pgno;
		page_table->load();
	}

	for (unsigned int j=i+1;j<index;++j)	map[j-1] = map[j];
	--index;

    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    Console::puts("Checked whether address is part of an allocated region.\n");
	return ((_address>=base_address) && (_address<base_address+size));
}

