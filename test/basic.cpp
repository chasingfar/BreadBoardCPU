//
// Created by chasingfar on 2021/7/9.
//
#include "catch.hpp"
#include "asm/asm.h"

using namespace BreadBoardCPU::ASM;
using BreadBoardCPU::CPU;

void run_op(CPU& cpu,const code_t& code){
	ops_t op=ASM{}<<code<<ASM::END;
	cpu.load_op(op);
	cpu.tick_op();
}

TEST_CASE("push and pop","[asm][basic]"){
    CPU cpu;
    cpu.tick_op();

    run_op(cpu,push(123));
    REQUIRE(*cpu.get_pointer(CPU::Reg16::SP,1) == 123);
	run_op(cpu,pop(Reg::A));
    REQUIRE(cpu.REG[CPU::Reg::A.v()]==123);
}