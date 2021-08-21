//
// Created by chasingfar on 2020/11/23.
//

#ifndef BREADBOARDCPU_ASM_ASM_H
#define BREADBOARDCPU_ASM_ASM_H

#include "function.h"
//#include "function_static.h"


namespace BreadBoardCPU::ASM {
	inline void generate(const std::string& name,auto program) {
		std::ofstream fout{name};
		if (!fout) { return; }
		fout<<ROM(program);
	}
	inline void simulate(const std::string& name,auto program) {
		std::cout<<name<<std::endl;
		CPU cpu;
		cpu.load(program);
		for (int i = 0; i < 50 && !cpu.isHalt(); ++i) {
			std::cout<<i<<std::endl;
			cpu.tick_op(true);
			cpu.print_reg();
			cpu.print_stack();
			std::cout<<std::endl;
		}
	}
	inline void generateASMROM() {
		ASM program{};
		generate("program",program
		<<ASM::END);
	}
}
#endif //BREADBOARDCPU_ASM_ASM_H
