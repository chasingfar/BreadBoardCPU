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

TEST_CASE("block","[asm][advance]"){
	Label a,b;
	CPU cpu=run(Block{{
		imm(Reg::A,5),
		a,
		imm(Reg::A,10),
		b,
		imm(Reg::A,15),
	}},{a,b});
	REQUIRE(_REG(A)==5);
	run(cpu,{a,b});
	REQUIRE(_REG(A)==10);
	run(cpu,{a,b});
	REQUIRE(_REG(A)==15);
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