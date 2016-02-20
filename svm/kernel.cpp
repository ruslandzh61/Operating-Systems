#include "kernel.h"

#include <iostream>
#include <algorithm>

namespace svm
{
    Kernel::Kernel(
                Scheduler scheduler,
                std::vector<Memory::ram_type> executables_paths
            )
        : board(),
          processes(),
          priorities(),
          scheduler(scheduler),
          _last_issued_process_id(0),
          _last_ram_position(0),
          _cycles_passed_after_preemption(0),
          _current_process_index(0)
    {

        // Memory Management

        /*
         *     Initialize data structures for methods `AllocateMemory` and
         *       `FreeMemory`
         */
		_last_free_block_index = 0;
		board.memory.ram[0] = 0;
		board.memory.ram[1] = Memory::DEFAULT_RAM_SIZE - 2;
		

        // Process page faults (find empty frames)
        board.pic.isr_4 = [&]() {	
            TryPageFault();
        };

        // Process Management

        std::for_each(
            executables_paths.begin(),
            executables_paths.end(),
            [&](Memory::ram_type &executable) {
                CreateProcess(executable);
            }
        );

        /*
         *    Switch to the first process on the CPU
         *    Switch the page table in the MMU to the table of the current
         *      process
         *    Set a proper state for the first process
         */
		if (scheduler == FirstComeFirstServed || scheduler == ShortestJob ||
			scheduler == RoundRobin) {
			if (!processes.empty()) {
				_current_process_index = 0;
				board.memory.page_table = 
					processes[_current_process_index ].page_table;
				board.cpu.registers = 
					processes[_current_process_index ].registers;
				processes[_current_process_index ].state = 
					Process::States::Running;
			} else
				board.Stop();
		} else if (scheduler == Priority) {
			if (!priorities.empty()) {
				//always pick first
				Process t = priorities.top();
				t.state = Process::States::Running;
				board.memory.page_table = t.page_table;
				board.cpu.registers = t.registers;
				
				priorities.pop();
                priorities.push(t);
			} else 
				board.Stop();
		}
		

        if (scheduler == FirstComeFirstServed) {
            board.pic.isr_0 = [&]() {
                // Process the timer interrupt for the FCFS
            };

            board.pic.isr_3 = [&]() {
                // Process the first software interrupt for the FCFS

                if (!processes.empty()) {
						// Unload the current process
						// release data in RAM
						FreeMemory(processes[_current_process_index].memory_start_position);
                        processes.erase(processes.begin());
                        if (!processes.empty()) {
                                //always pick first
                                _current_process_index = 0;
                                processes[_current_process_index].state = Process::States::Running;
                                //schedular doesn't block process, just terminates it
								//and as a result doesn't load it later, therefore
                                //the following line is redundant
                                //board.cpu.registers = processes[_current_process_index].registers;
                        }
                        else board.Stop();
                }
            };
        } else if (scheduler == ShortestJob) {
            board.pic.isr_0 = [&]() {
                // Process the timer interrupt for the Shortest
                //  Job First scheduler
            };

            board.pic.isr_3 = [&]() {
                // Process the first software interrupt for the Shortest
                //  Job scheduler
				if (!processes.empty()) {
						// Unload the current process
						// release data in RAM
						FreeMemory(processes[_current_process_index].memory_start_position);
                        processes.erase(processes.begin());
                        if (!processes.empty()) {
                                //always pick first
                                _current_process_index = 0;
                                processes[_current_process_index].state = Process::States::Running;
                                //schedular doesn't block process, just terminates it
								//and as a result doesn't load it later, therefore
                                //the following line is redundant
                                //board.cpu.registers = processes[_current_process_index].registers;
                        }
                        else board.Stop();
                }
                
            };
        } else if (scheduler == RoundRobin) {
            board.pic.isr_0 = [&]() {
                // Process the timer interrupt for the Round Robin
                //  scheduler
				++_cycles_passed_after_preemption;
                if (_cycles_passed_after_preemption > _MAX_CYCLES_BEFORE_PREEMPTION) {
						
                        processes[_current_process_index ].registers = board.cpu.registers;
                        processes[_current_process_index ].state = Process::States::Ready;
                        if (_current_process_index < processes.size() - 1) {
                                ++_current_process_index;
                        }
                        else _current_process_index = 0;
						board.memory.page_table = 
							processes[_current_process_index ].page_table;
                        board.cpu.registers = 
							processes[_current_process_index ].registers;
                        processes[_current_process_index ].state = 
							Process::States::Running;

                        _cycles_passed_after_preemption = 0;
                }
            };

            board.pic.isr_3 = [&]() {
                //Process the first software interrupt for the
                //Round Robin scheduler

                // Unload the current process
				if (!processes.empty()) {
                        //terminate current process
						//release data in RAM
						FreeMemory(processes[_current_process_index].memory_start_position);
                        processes.erase(processes.begin() + _current_process_index);
                        if (!processes.empty()) {
                                if (_current_process_index < processes.size()) {
                                        ++_current_process_index;
                                }
                                else _current_process_index = 0;
                                //load next
								//change address space
								board.memory.page_table = 
									processes[_current_process_index ].page_table;
                                board.cpu.registers = 
									processes[_current_process_index ].registers;
                                processes[_current_process_index ].state = 
									Process::States::Running;
                        }
                        else board.Stop();
                }
            };
        } else if (scheduler == Priority) {
            board.pic.isr_0 = [&]() {
                //  Process the timer interrupt for the Priority Queue
                //  Priority scheduler
				++_cycles_passed_after_preemption;
                if (_cycles_passed_after_preemption > _MAX_CYCLES_BEFORE_PREEMPTION) {
                        Process t = priorities.top();
                        --t.priority;
                        t.registers = board.cpu.registers;
                        t.state = Process::States::Ready;

                        priorities.pop();
                        priorities.push(t);

                        t = priorities.top();
						board.memory.page_table = t.page_table; 
                        t.state = Process::States::Running;
                        board.cpu.registers = t.registers;

                        priorities.pop();
                        priorities.push(t);

                        _cycles_passed_after_preemption = 0;
                }
            };

            board.pic.isr_3 = [&]() {
                // Process the first software interrupt for the Priority
                //  Priority scheduler
				if (!priorities.empty()) {
						// Unload the current process
						//release data in RAM
						Process t = priorities.top();
						FreeMemory(t.memory_start_position);
                        priorities.pop();
                        if (!priorities.empty()) {
                                //always pick first
                                Process t = priorities.top();
                                t.state = Process::States::Running;
								board.memory.page_table = t.page_table;
                                board.cpu.registers = t.registers;

                                priorities.pop();
                                priorities.push(t);
                        }
                        else board.Stop();
                }
                
            };
        }

        board.Start();
    }

    Kernel::~Kernel() { }

    void Kernel::CreateProcess(Memory::ram_type &executable)
    {
		// Allocate memory for the process with `AllocateMemory`
        Memory::ram_size_type
            new_memory_position = AllocateMemory(executable.size());
        
        if (new_memory_position == NO_FREE_LARGE_ENOUGH_BLOCK) {
            std::cerr << "Kernel: failed to allocate memory."
                      << std::endl;
        } else {
            // Assume that the executable image size can not be greater than
            //   a page size
            std::copy(
                executable.begin(),
                executable.end(),
                board.memory.ram.begin() + new_memory_position);

            Process process(
                _last_issued_process_id++,
                new_memory_position,
                new_memory_position + executable.size()
            );

            // add the new process to an appropriate data structure
			if (scheduler == FirstComeFirstServed || scheduler == RoundRobin) {
                processes.push_back(process);
			} else if (scheduler == ShortestJob) {
					processes.push_back(process);
					sort(processes.begin(), processes.end(), 
							[](const Process& p1, const Process& p2) {
							return p1.sequential_instruction_count < 
								p2.sequential_instruction_count;});
			}
			else if (scheduler == Priority) {
					priorities.push(process);
			}
        }
    }

    Memory::ram_size_type Kernel::AllocateMemory(
                                      Memory::ram_size_type units
                                  )
    {		
		 
		units += 2; // 2 additional cells for a header
        
        Memory::ram_size_type current_free_node_index, previous_free_node_index;
        
		auto &ram = board.memory.ram;
		
		
        previous_free_node_index = _last_free_block_index;
        for (current_free_node_index = ram[Translate(previous_free_node_index)]; ;
             previous_free_node_index = current_free_node_index,
             current_free_node_index = ram[Translate(current_free_node_index)]) {
            if (ram[Translate(current_free_node_index + 1)] >= units) {
                if (ram[Translate(current_free_node_index + 1)] == units) {
                    ram[Translate(previous_free_node_index)] =
                        ram[Translate(current_free_node_index)];
                    _last_free_block_index = ram[Translate(current_free_node_index)];
                } else {
                    ram[Translate(current_free_node_index + 1)] -= units;
                    _last_free_block_index = current_free_node_index;
                    current_free_node_index += 
						ram[Translate(current_free_node_index + 1)] + 2;
                    ram[Translate(current_free_node_index + 1)] = units - 2;
                }
				
				//return physical address
                return Translate(current_free_node_index + 2);
            }
            if (current_free_node_index == _last_free_block_index) {
                return NO_FREE_LARGE_ENOUGH_BLOCK;
            }
        }

        return NO_FREE_LARGE_ENOUGH_BLOCK;
    }

    void Kernel::FreeMemory(
                     Memory::ram_size_type physical_address
                 )
    {
        Memory::ram_size_type index_of_used_block_header = physical_address - 2;
        Memory::ram_size_type index_of_size_of_used_block = physical_address - 1;
		
		auto &ram = board.memory.ram;
        
        Memory::ram_size_type current_index_of_free_node;
        for (current_index_of_free_node = _last_free_block_index;
                !(index_of_used_block_header > current_index_of_free_node &&
                index_of_used_block_header < ram[Translate(current_index_of_free_node)]);
                current_index_of_free_node = ram[Translate(current_index_of_free_node)]) {
            if (current_index_of_free_node >= ram[Translate(current_index_of_free_node)] &&
                (index_of_used_block_header > current_index_of_free_node ||
                 index_of_used_block_header < ram[Translate(current_index_of_free_node)])) {
                    break;
                }
        }
        
		//try to merge with the right block
        if (index_of_used_block_header + ram[Translate(index_of_size_of_used_block)] == 
			ram[Translate(current_index_of_free_node)]) {
            ram[Translate(index_of_size_of_used_block)] += 
				ram[Translate(ram[Translate(current_index_of_free_node)] + 1)] + 2;
            ram[Translate(index_of_used_block_header)] = 
				ram[Translate(ram[Translate(current_index_of_free_node)])];
        } else {
            ram[Translate(index_of_used_block_header)] = 
				ram[Translate(current_index_of_free_node)];
        }
        
        //try to merge with the left block
        if (current_index_of_free_node + 
			ram[Translate(current_index_of_free_node + 1)] + 2 == index_of_used_block_header) {
            ram[Translate(current_index_of_free_node + 1)] += 
				ram[Translate(index_of_size_of_used_block)] + 2;
            ram[Translate(current_index_of_free_node)] = 
				ram[Translate(index_of_used_block_header)];
        } else {
            ram[Translate(current_index_of_free_node)] = index_of_used_block_header;
        }
        
        _last_free_block_index = current_index_of_free_node;
    }
	
	//translate from virtual to physical address
	Memory::ram_size_type Kernel::Translate(Memory::ram_size_type virtual_address) {
        // 1. Get a page index and physical address offset from the MMU
        // for a virtual address in 'data'
        Memory::page_index_offset_pair_type page_index_offset_pair = 
			board.memory.GetPageIndexAndOffsetForVirtualAddress(virtual_address);
		
        // 2. Get the page from the current page table in the MMU with
        // the acquired index
        Memory::page_entry_type page_frame_index = 
			board.memory.page_table->at(page_index_offset_pair.first);
        Memory::ram_size_type physical_index;
		
        if (page_frame_index == Memory::INVALID_PAGE) {
			//save index in register 'a'
			auto previous_a = board.cpu.registers.a;
			board.cpu.registers.a = page_index_offset_pair.first;
			
            if (TryPageFault()) { //if there is a free physical memory calcuate index 
				// Calculate the physical address with the value in the page entry and the
				// physical address offset
				physical_index = page_index_offset_pair.second + 
									Memory::PAGE_SIZE * page_frame_index;
			}
			
			//restore register 'a'
			board.cpu.registers.a = previous_a;
        }
         
        return physical_index;
    }
	
	bool Kernel::TryPageFault() {
			bool is_there_free_memory = true;
            std::cout << "Kernel: page fault." << std::endl;
            
            // Get the faulting page index from the register 'a'
            auto faulting_page_index = board.cpu.registers.a;
            // Try to acquire a new frame from the MMU by calling `AcquireFrame`
            auto free_frame = board.memory.AcquireFrame();
            
            if (free_frame != Memory::INVALID_PAGE) {
                // Write the frame to the current faulting page in the
                // MMU page table (at index from register 'a')
                board.memory.page_table->at(faulting_page_index) = free_frame;
            } else {
                // Notify the process or stop the board (out of
                // physical memory)
				is_there_free_memory = false;
                std::cout << "Out of physical memory." << std::endl;
                board.Stop();
            }
			
			return is_there_free_memory;
	}
}
