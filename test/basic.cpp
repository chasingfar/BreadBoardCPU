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

TEST_CASE("load and save","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	cpu.load(ops_t{12,34},0xABCD);
	cpu.REG[CPU::Reg::B.v()]=0xAB;
	cpu.REG[CPU::Reg::A.v()]=0xCD;

	run_op(cpu,load(Reg16::BA));
	REQUIRE(*cpu.get_pointer(CPU::Reg16::SP,1) == 12);
	run_op(cpu,load(Reg16::BA,1));
	REQUIRE(*cpu.get_pointer(CPU::Reg16::SP,1) == 34);

	run_op(cpu,save(Reg16::BA));
	REQUIRE(cpu.RAM[0xABCD]==34);
	run_op(cpu,save(Reg16::BA,1));
	REQUIRE(cpu.RAM[0xABCE]==12);
}

TEST_CASE("push and pop","[asm][basic]"){
    CPU cpu;
    cpu.tick_op();

	cpu.REG[CPU::Reg::B.v()]=123;

    run_op(cpu,push(Reg::B));
    REQUIRE(*cpu.get_pointer(CPU::Reg16::SP,1) == 123);
	run_op(cpu,pop(Reg::A));
    REQUIRE(cpu.REG[CPU::Reg::A.v()]==123);
}