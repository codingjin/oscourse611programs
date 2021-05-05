/*
 File: page_table.C
 
 Author: Jin Huang
 Department of Computer Science
 Texas A&M University
 Date  : 03/16/2021
 
 Description: PageTable with Recursive Table Lookup.
 
 */

#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;


void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
	PageTable::kernel_mem_pool = _kernel_mem_pool;
	PageTable::process_mem_pool = _process_mem_pool;
	PageTable::shared_size = _shared_size;

    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
	page_directory = 
		(unsigned long *)(PageTable::process_mem_pool->get_frames(1) * Machine::PAGE_SIZE);

	unsigned long *first_pagetable = 
		(unsigned long *)(PageTable::process_mem_pool->get_frames(1) * Machine::PAGE_SIZE);

	// initialize the page_directory and first_pagetable
	// attribute set:supervisor level, read/write, present (011)
	page_directory[0] = (unsigned long)first_pagetable | 3;
	first_pagetable[0] = 3; // 011 super, r/w, present

	int index = 1;
	while (index < Machine::PT_ENTRIES_PER_PAGE) {
		//page_directory[index] = 0;
		page_directory[index] = 2;// 010 super,r/w, not present
		first_pagetable[index] = first_pagetable[index-1] + Machine::PAGE_SIZE;
		++index;
	}

	// Recursive Table Lookup
	page_directory[Machine::PT_ENTRIES_PER_PAGE -1] = (unsigned long)page_directory | 3;


	pool_index = 0;
    Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
	PageTable::current_page_table = this;
	write_cr3((unsigned long int)page_directory);
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
	PageTable::paging_enabled = 1;
	write_cr0(read_cr0() | 0x80000000);
    Console::puts("Enabled paging\n");
}

/*
 * Notice
 * unsigned long * page_directory = 0xFFFFF000
 * unsigned long * pagetable = (unsigned long *)((0xFFC << 20) | (dir_index<<12))
 */
void PageTable::handle_fault(REGS * _r)
{
	//Console::puts("enter handle_fault!\n");
	unsigned int attribute = _r->err_code & 7;
	if (attribute & 1)	Console::puts("protection fault!\n");
	else {
		attribute = attribute | 1; // set present
		unsigned long addr = read_cr2();
		/*
		Console::puts("read_cr2()=");
		Console::putui(addr);
		Console::puts("\n");
		*/

		unsigned long dir_index = addr >> 22;
		/*
		Console::puts("dir_index=");
		Console::putui(dir_index);
		Console::puts("\n");
		*/

		unsigned long pt_index = (addr>>12) & 0x3FF;
		/*
		Console::puts("pt_index=");
		Console::putui(pt_index);
		Console::puts("\n");
		*/

		unsigned long *directory = (unsigned long *)0xFFFFF000;
		/*
		Console::puts("vm directory addr=");
		Console::putui((unsigned long)directory);
		Console::puts("\n");
		*/

		unsigned long newpage_phyaddr =
			PageTable::process_mem_pool->get_frames(1) * Machine::PAGE_SIZE;

		if (!newpage_phyaddr) {
			Console::puts("no space!\n");
			return;
		}

		unsigned long *pagetable;
		if (directory[dir_index] & 1) {
			// page table already exists
			pagetable = (unsigned long *)((0xFFC<<20) | (dir_index<<12));
		}else {
			// page table not exists yet
			unsigned long pagetable_phyaddr =
				PageTable::process_mem_pool->get_frames(1) * Machine::PAGE_SIZE;
			if (!pagetable_phyaddr) {
				Console::puts("no space!\n");
				return;
			}

			// fill in the pde, directory[dir_index]
			directory[dir_index] = pagetable_phyaddr | 3; // 011 s,r/w,p

			// initialize the new pagetable
			pagetable = (unsigned long *)((0xFFC<<20) | (dir_index<<12));
			for (int i=0; i< Machine::PT_ENTRIES_PER_PAGE;++i)
				pagetable[i] = 0;

		}

		pagetable[pt_index] = newpage_phyaddr | attribute;
	}

    Console::puts("handled page fault\n");
}

void PageTable::register_pool(VMPool * _vm_pool)
{
	if (pool_index >= MAX_POOL_SIZE) {
		Console::puts("pool size limits!\n");
		return;
	}

	pools[pool_index++] = _vm_pool;

    Console::puts("registered VM pool\n");
}

void PageTable::free_page(unsigned long _page_no) {
	unsigned long dir_index = _page_no >> 10;
	unsigned long pt_index = _page_no & 0x3FF;

	unsigned long *directory = (unsigned long *)0xFFFFF000;

	unsigned long *table = 
		(unsigned long *)((0xFFC<<20)| (dir_index<<12));

	ContFramePool::release_frames(table[pt_index]>>12);
	table[pt_index] = 0;
	unsigned int empty = 1;
	for (unsigned int i=0;i<Machine::PT_ENTRIES_PER_PAGE;++i) {
		if (table[i]) {
			empty = 0;
			break;
		}

	}

	if (empty) {
		ContFramePool::release_frames(directory[dir_index]>>12);
		directory[dir_index] = 2;
	}
	
    Console::puts("freed page\n");
}
