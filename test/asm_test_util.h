//
// Created by chasingfar on 2021/7/25.
//

#ifndef BREADBOARDCPU_ASM_TEST_UTIL_H
#define BREADBOARDCPU_ASM_TEST_UTIL_H

#include "catch.hpp"
#include "asm/asm.h"

using namespace BreadBoardCPU::ASM;
using BreadBoardCPU::CPU;
using BreadBoardCPU::MARG;
using ALU74181::Carry;

#define _REG(name) cpu.REG[CPU::Reg::name.v()]
#define _REG16(name) cpu.get_pair(CPU::Reg16::name)
#define _STACK_TOP cpu.read_pair(CPU::Reg16::SP,1)
#define _STACK_INSERT cpu.read_pair(CPU::Reg16::SP)
#define _LOCAL(offset) cpu.read_pair(CPU::Reg16::HL,(offset))
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
	cpu.load(ASM{}<<program<<halt()<<ASM::END);
	return run(cpu,pause_at,max_iter);
}
#endif //BREADBOARDCPU_ASM_TEST_UTIL_H
