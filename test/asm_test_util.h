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
using namespace BBCPU::Lang;
using BBCPU::RegSet_SRAM::Hardware::CPU;
using ALU74181::Carry;
using MEM=decltype(CPU{}.mem);

#define REG_(name) cpu.reg.data[CPU::Reg::name.v()]
#define REG16_(name) cpu.get_ptr(CPU::Reg16::name)
#define STACK_TOP_ cpu.read_ptr(CPU::Reg16::SP,1)
#define STACK_INSERT_ cpu.read_ptr(CPU::Reg16::SP)
#define LOCAL_(var) cpu.read_ptr(CPU::ASM_BP,std::dynamic_pointer_cast<LocalVar>((var).value)->offset)
#define STATIC_(label,var) cpu.mem.get_data(*(label).start.addr+std::dynamic_pointer_cast<StaticVar>((var).value)->offset).value_or(0)
#define RUN_OP_(code) cpu.load_op(Code{(code)}.assemble());cpu.tick_op();
#define SET_FLAG_(flag,value_) cpu.cu.tbl.D=MCTRL::state::flag::set(cpu.cu.tbl.D.value(),value_);
#define LOAD_TO_(start,code) cpu.load(Code{(code)}.assemble(start),start);

inline op_t operator "" _op(unsigned long long value) {
    return static_cast<op_t>(value);
}

inline CPU& run(CPU& cpu,const std::vector<Label>& pause_at={},size_t max_iter=1024){
	for (size_t i = 0; i < max_iter; ++i) {
		cpu.tick_op();
		if(cpu.is_halt()){
			return cpu;
		}
		for (const auto& label:pause_at){
			if (REG16_(PC) == *label){
				return cpu;
			}
		}
	}
	std::cout<<"run over max_iter="<<max_iter<<std::endl;
	return cpu;
}
inline CPU& load_run(CPU& cpu,const Code& program,const std::vector<Label>& pause_at={},size_t max_iter=1024){
	cpu.load(program.assemble());
	return run(cpu,pause_at,max_iter);
}
#endif //BBCPU_ASM_TEST_UTIL_H
