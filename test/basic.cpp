//
// Created by chasingfar on 2021/7/9.
//
#include "catch.hpp"
#include "asm/asm.h"

using namespace BreadBoardCPU::ASM;
using BreadBoardCPU::CPU;

#define _REG(name) cpu.REG[CPU::Reg::name.v()]
#define _REG16(name) cpu.get_pair(CPU::Reg16::name)
#define _STACK_TOP *cpu.get_pointer(CPU::Reg16::SP,1)
#define _STACK_INSERT *cpu.get_pointer(CPU::Reg16::SP)
#define _RUN_OP(code) cpu.load_op(ASM{}<<(code)<<ASM::END);cpu.tick_op();

TEST_CASE("load and save","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	cpu.load(ops_t{12,34},0xABCD);
	_REG(B)=0xAB;
	_REG(A)=0xCD;

	_RUN_OP(load(Reg16::BA));
	REQUIRE(_STACK_TOP == 12);
	_RUN_OP(load(Reg16::BA,1));
	REQUIRE(_STACK_TOP == 34);

	_RUN_OP(save(Reg16::BA));
	REQUIRE(cpu.RAM[0xABCD]==34);
	_RUN_OP(save(Reg16::BA,1));
	REQUIRE(cpu.RAM[0xABCE]==12);
}

TEST_CASE("push and pop","[asm][basic]"){
    CPU cpu;
    cpu.tick_op();

	_REG(B)=123;

	_RUN_OP(push(Reg::B));
    REQUIRE(_STACK_TOP == 123);
	_RUN_OP(pop(Reg::A));
    REQUIRE(_REG(A)==123);
}

TEST_CASE("immediate value","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	_RUN_OP(imm(123));
	REQUIRE(_STACK_TOP == 123);
	_RUN_OP(imm(Label{0xABCD}));
	REQUIRE(_STACK_TOP == 0xCD);
	cpu.tick_op();
	REQUIRE(_STACK_TOP == 0xAB);
}

TEST_CASE("jump and branch","[asm][basic]"){
	using BreadBoardCPU::MARG;
	using ALU74181::Carry;
	CPU cpu;
	cpu.tick_op();

	_RUN_OP(jmp(Label{0xABCD}));
	REQUIRE(_REG16(PC) == 0xABCD);

	cpu.marg=MARG::state::CF::set(cpu.marg,Carry::yes);
	_RUN_OP(brc(Label{0xBBCD}));
	REQUIRE(_REG16(PC)  == 0xBBCD);
	cpu.marg=MARG::state::CF::set(cpu.marg,Carry::no);
	_RUN_OP(brc(Label{0xCBCD}));
	REQUIRE(_REG16(PC)  == 0xBBCD+3);


	_RUN_OP(imm(0));
	_RUN_OP(brz(Label{0x1234}));
	REQUIRE(_REG16(PC) == 0x1234);
	_RUN_OP(imm(1));
	_RUN_OP(brz(Label{0x2234}));
	REQUIRE(_REG16(PC) == 2+0x1234+3);
}

#undef _REG
#undef _REG16
#undef _STACK_TOP
#undef _STACK_INSERT
#undef _RUN_OP