//
// Created by chasingfar on 2021/7/23.
//
#include "catch.hpp"
#include "asm/asm.h"
#include "cpu/cpu.h"
#include <cstddef>

using namespace BreadBoardCPU::ASM;
using BreadBoardCPU::CPU;

#define _REG(name) cpu.REG[CPU::Reg::name.v()]
#define _REG16(name) cpu.get_pair(CPU::Reg16::name)
#define _STACK_TOP *cpu.get_pointer(CPU::Reg16::SP,1)
#define _STACK_INSERT *cpu.get_pointer(CPU::Reg16::SP)

inline CPU run(code_t program,size_t max_iter=1024){
	CPU cpu;
	cpu.load(ASM{}<<program<<halt()<<ASM::END);
	for (size_t i = 0; i < max_iter && !cpu.isHalt(); ++i) {
		cpu.tick_op();
	}
	return cpu;
}

TEST_CASE("block","[asm][advance]"){
	CPU cpu=run(
		Block{imm(Reg::A,5)}
	);
	REQUIRE(_REG(A)==5);
}

TEST_CASE("if","[asm][advance]"){
	SECTION("if true"){
		CPU cpu=run(
			IF{{imm(1)},
				{imm(Reg::A,5)},
				{imm(Reg::A,6)}
			}
		);
		REQUIRE(_REG(A)==5);
	}
	SECTION("if false"){
		CPU cpu=run(
			IF{{imm(0)},
				{imm(Reg::A,5)},
				{imm(Reg::A,6)}
			}
		);
		REQUIRE(_REG(A)==6);
	}
}
TEST_CASE("while","[asm][advance]"){
	/*
	a=0,b=3;
	while(b){
		a+=b;
		b-=1;
	}
	*/
	CPU cpu=run({
		imm(Reg::A,0),imm(Reg::B,3),
		While{{push(Reg::B)},{{
			add(Reg::A,Reg::A,Reg::B),
			sub(Reg::B,Reg::B,1),
		}}},
	});
	REQUIRE(_REG(A)==6);
}