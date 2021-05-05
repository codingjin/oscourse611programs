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

PageTable::PageTable() : page_directory(NULL)
{

	page_directory = 
		(unsigned long *)(PageTable::kernel_mem_pool->get_frames(1) * Machine::PAGE_SIZE);
		
	unsigned long *first_pagetable =
		(unsigned long *)(PageTable::kernel_mem_pool->get_frames(1) * Machine::PAGE_SIZE);

	// initialize the page_directory and first_pagetable
	// attribute set:supervisor level, read/write, present (011)
	page_directory[0] = (unsigned long)first_pagetable | 3;
	first_pagetable[0] = 3; // page 0; kernel r/w present

	unsigned int index = 1;
	while (index < Machine::PT_ENTRIES_PER_PAGE) {
		page_directory[index] = 0;
		first_pagetable[index] = first_pagetable[index-1] + Machine::PAGE_SIZE;
		++index;
	}

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

void PageTable::handle_fault(REGS * _r)
{
	unsigned int attribute = _r->err_code & 7;
	if (attribute & 1)	Console::puts("protection fault!\n");
	else {
		attribute = attribute | 1; // set present bit 1
		unsigned long *addr = (unsigned long *)read_cr2(); // address accessed
		unsigned long *directory = (unsigned long *)read_cr3(); // page directory base addr

		unsigned long *page_addr = 
			(unsigned long *)(PageTable::kernel_mem_pool->get_frames(1) * Machine::PAGE_SIZE);
		if (page_addr == 0) {
			Console::puts("no space!\n");
			return;
		}

		unsigned long directory_index = (unsigned long)addr>>22;
		unsigned long directory_content = directory[directory_index];
		unsigned long table_index = ((unsigned long)addr>>12) & 0x3ff;
		unsigned long *table;

		if (directory_content & 1) {
			// page table already exists
			table = (unsigned long *)(directory_content & 0xfffff000);
		}else {
			// page table not exists yet
			table = (unsigned long *)(PageTable::kernel_mem_pool->get_frames(1) * Machine::PAGE_SIZE);
			directory[directory_index] = (unsigned long)table | 3;// 011
			
			// initialize the new pagetable
			unsigned int i = 0;
			while (i != Machine::PT_ENTRIES_PER_PAGE)
				table[i++] = 0;

		}
		// load the new page into page table with user r/w present
		table[table_index] = (unsigned long)page_addr | attribute;

	}

	Console::puts("handled page fault\n");

}

