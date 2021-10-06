//
// Created by chasingfar on 2021/7/25.
//

#ifndef BBCPU_ASM_TEST_UTIL_H
#define BBCPU_ASM_TEST_UTIL_H

#include "catch.hpp"
#include "asm/asm.h"
#include "lang/lang.h"

using namespace BBCPU::ASM;
using BBCPU::CPU;
using BBCPU::MARG;
using ALU74181::Carry;

#define _REG(name) cpu.REG[CPU::Reg::name.v()]
#define _REG16(name) cpu.get_pair(CPU::Reg16::name)
#define _STACK_TOP cpu.read_pair(CPU::Reg16::SP,1)
#define _STACK_INSERT cpu.read_pair(CPU::Reg16::SP)
#define _LOCAL(var) cpu.read_pair(CPU::ASM_BP,std::dynamic_pointer_cast<LocalVar>((var).value)->offset)
#define _STATIC(label,var) cpu.RAM[*(label).start.addr+std::dynamic_pointer_cast<StaticVar>((var).value)->offset]
#define _RUN_OP(code) cpu.load_op(ASM{}<<(code)<<ASM::END);cpu.tick_op();
#define _SET_FLAG(flag,value) cpu.marg=MARG::state::flag::set(cpu.marg,value);

inline CPU run(CPU& cpu,const std::vector<Label>& pause_at={},size_t max_iter=1024){
	for (size_t i = 0; i < max_iter; ++i) {
		cpu.tick_op();
		if(cpu.isHalt()){
			return cpu;
		}
		for (const auto& label:pause_at){
			if (_REG16(PC)==*label){
				return cpu;
			}
		}
	}
	return cpu;
}
inline CPU run(const code_t& program,const std::vector<Label>& pause_at={},size_t max_iter=1024){
	CPU cpu;
	cpu.load(ASM::parse(program));
	return run(cpu,pause_at,max_iter);
}
#endif //BBCPU_ASM_TEST_UTIL_H
