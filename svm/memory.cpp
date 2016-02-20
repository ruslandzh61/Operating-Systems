#include "memory.h"

namespace svm
{
    Memory::Memory()
        : ram(DEFAULT_RAM_SIZE)
    {
        // initialize data structures for the frame allocator
		int frames_number = DEFAULT_RAM_SIZE / PAGE_SIZE;
        for (int i = 0; i < frames_number; ++i) { 
            frames.push(i);
        }
	}

    Memory::~Memory() { }

    Memory::page_table_type* Memory::CreateEmptyPageTable()
    {
        /*
              Return a new page table (for kernel or processes)
              Each entry should be invalid
        */
		return new vector<Memory::page_entry_type>(DEFAULT_RAM_SIZE / PAGE_SIZE, -1);
    }

    Memory::page_index_offset_pair_type
        Memory::GetPageIndexAndOffsetForVirtualAddress(
                 vmem_size_type virtual_address
             )
    {
        /*
             Calculate the page index from the virtual address
             Calculate the offset in the physical memory from the virtual
             address
        */
		Memory::page_index_offset_pair_type result =
            std::make_pair(virtual_address / PAGE_SIZE, virtual_address % PAGE_SIZE);

        return result;
    }

    Memory::page_entry_type Memory::AcquireFrame()
    {
        // find a new free frame (you can use a bitmap or stack)
		if (!frames.empty()) {
            auto t = frames.top();
            frames.pop();
			return t;
        }
		
        return INVALID_PAGE;
    }

    void Memory::ReleaseFrame(page_entry_type page)
    {
        //free the physical frame (you can use a bitmap or stack)
		frames.push(page);
	}
}
