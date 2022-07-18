//
// Created by chasingfar on 2021/7/25.
//

#ifndef BBCPU_ASM_TEST_UTIL_H
#define BBCPU_ASM_TEST_UTIL_H

#include "catch.hpp"
#include "asm/asm.h"
#include "cpu/regset_sram/cpu.h"
#include "lang/lang.h"

using namespace BBCPU::ASM;
using BBCPU::CPU;
using BBCPU::MARG;
using ALU74181::Carry;

#define _REG(name) cpu.reg.data[CPU::Reg::name.v()]
#define _REG16(name) cpu.get_ptr(CPU::Reg16::name)
#define _STACK_TOP cpu.read_ptr(CPU::Reg16::SP,1)
#define _STACK_INSERT cpu.read_ptr(CPU::Reg16::SP)
#define _LOCAL(var) cpu.read_ptr(CPU::ASM_BP,std::dynamic_pointer_cast<LocalVar>((var).value)->offset)
#define _STATIC(label,var) cpu.mem.get_data(*(label).start.addr+std::dynamic_pointer_cast<StaticVar>((var).value)->offset)
#define _RUN_OP(code) cpu.load_op(ASM{}<<(code)<<ASM::END);cpu.tick_op();
#define _SET_FLAG(flag,value_) cpu.cu.sreg.output=MARG::state::flag::set(cpu.cu.sreg.output.value(),value_);

inline CPU run(CPU& cpu,const std::vector<Label>& pause_at={},size_t max_iter=1024){
	for (size_t i = 0; i < max_iter; ++i) {
		cpu.tick_op();
		if(cpu.is_halt()){
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
