#include "cpu.h"

#include <iostream>

namespace svm
{
    Registers::Registers()
        : a(0), b(0), c(0), flags(0), ip(0), sp(0) { }

    CPU::CPU(Memory &memory, PIC &pic)
        : registers(),
          _memory(memory),
          _pic(pic) { }

    CPU::~CPU() { }

    void CPU::Step()
    {
        int ip =
            registers.ip;

        int instruction =
            _memory.ram[ip];
        int data =
            _memory.ram[ip + 1];

        if (instruction ==
                CPU::MOVA_OPCODE) {
            registers.a = data;
            registers.ip += 2;
        } else if (instruction ==
                       CPU::MOVB_OPCODE) {
            registers.b = data;
            registers.ip += 2;
        } else if (instruction ==
                       CPU::MOVC_OPCODE) {
            registers.c = data;
            registers.ip += 2;
        } else if (instruction ==
                       CPU::JMP_OPCODE) {
            registers.ip += data;
        } else if (instruction ==
                       CPU::INT_OPCODE) {
            switch (data)
            {
                case 1:
                    _pic.isr_3();
                    break;
                    //case 2:
                    //  _pic.isr_5(); // `isr_4` is reserved for page fault
                    //                // exceptions
                    //  break;
                    // ...
            }
            registers.ip += 2;
	    } else if (instruction ==
                        CPU::LDA_OPCODE) { //load to register a
            auto virtual_page_index_and_offset = 
				_memory.GetPageIndexAndOffsetForVirtualAddress(data);
            auto page_frame_index = 
					_memory.page_table->at(virtual_page_index_and_offset.first);
            if (page_frame_index == Memory::INVALID_PAGE) {
                auto previous_a = registers.a;
                registers.a = virtual_page_index_and_offset.first;
                _pic.isr_4();
                registers.a = previous_a;
            } else {
                auto physical_index = 
						virtual_page_index_and_offset.second + 
						Memory::PAGE_SIZE * page_frame_index;
                registers.a = _memory.ram[physical_index];
                ++instruction;
            }
        } else if (instruction ==
                        CPU::LDB_OPCODE) {
			auto virtual_page_index_and_offset = 
					_memory.GetPageIndexAndOffsetForVirtualAddress(data);
            auto page_frame_index = 
					_memory.page_table->at(virtual_page_index_and_offset.first);
            if (page_frame_index == Memory::INVALID_PAGE) {
                auto previous_a = registers.a;
                registers.a = virtual_page_index_and_offset.first;
                _pic.isr_4();
                registers.a = previous_a;
            } else {
                auto physical_index = 
						virtual_page_index_and_offset.second + 
						Memory::PAGE_SIZE * page_frame_index;
                registers.b = _memory.ram[physical_index];
                ++instruction;
            }
        } else if (instruction ==
                        CPU::LDC_OPCODE) {
			auto virtual_page_index_and_offset = 
				_memory.GetPageIndexAndOffsetForVirtualAddress(data);
            auto page_frame_index = 
					_memory.page_table->at(virtual_page_index_and_offset.first);
            if (page_frame_index == Memory::INVALID_PAGE) {
                auto previous_a = registers.a;
                registers.a = virtual_page_index_and_offset.first;
                _pic.isr_4();
                registers.a = previous_a;
            } else {
                auto physical_index = 
						virtual_page_index_and_offset.second + 
						Memory::PAGE_SIZE * page_frame_index;
                registers.c = _memory.ram[physical_index];
                ++instruction;
            }
        } else if (instruction ==
                        CPU::STA_OPCODE) {
			auto virtual_page_index_and_offset = 
				_memory.GetPageIndexAndOffsetForVirtualAddress(data);
            auto page_frame_index = 
					_memory.page_table->at(virtual_page_index_and_offset.first);
            if (page_frame_index == Memory::INVALID_PAGE) {
                auto previous_a = registers.a;
                registers.a = virtual_page_index_and_offset.first;
                _pic.isr_4();
                registers.a = previous_a;
            } else {
                auto physical_index = 
						virtual_page_index_and_offset.second + 
						Memory::PAGE_SIZE * page_frame_index;
                _memory.ram[physical_index] = registers.a; // write to the physical memory
                ++instruction;
            }
        } else if (instruction ==
                        CPU::STB_OPCODE) {
            auto virtual_page_index_and_offset = 
				_memory.GetPageIndexAndOffsetForVirtualAddress(data);
            auto page_frame_index = 
					_memory.page_table->at(virtual_page_index_and_offset.first);
            if (page_frame_index == Memory::INVALID_PAGE) {
                auto previous_a = registers.a;
                registers.a = virtual_page_index_and_offset.first;
                _pic.isr_4();
                registers.a = previous_a;
            } else {
                auto physical_index = 
						virtual_page_index_and_offset.second + 
						Memory::PAGE_SIZE * page_frame_index;
                _memory.ram[physical_index] = registers.b;
                ++instruction;
            }
        } else if (instruction ==
                        CPU::STC_OPCODE) {
			auto virtual_page_index_and_offset = 
				_memory.GetPageIndexAndOffsetForVirtualAddress(data);
            auto page_frame_index = 
					_memory.page_table->at(virtual_page_index_and_offset.first);
            if (page_frame_index == Memory::INVALID_PAGE) {
                auto previous_a = registers.a;
                registers.a = virtual_page_index_and_offset.first;
                _pic.isr_4();
                registers.a = previous_a;
            } else {
                auto physical_index = 
						virtual_page_index_and_offset.second + 
						Memory::PAGE_SIZE * page_frame_index;
                _memory.ram[physical_index] = registers.c;
                ++instruction;
            }
        } else {
            std::cerr << "CPU: invalid opcode data. Skipping..."
                      << std::endl;
            registers.ip += 2;
        }
    }
}
