//
// Created by chasingfar on 2021/7/9.
//
#include "catch.hpp"
#include "asm/asm.h"

using namespace BreadBoardCPU::ASM;
using BreadBoardCPU::CPU;

TEST_CASE("push and pop","[asm][basic]"){
    CPU cpu;
	cpu.load(ASM{}
    <<push(123)
    <<pop(Reg::A)
    <<ASM::END);
    cpu.tick_op();

    cpu.tick_op();
    REQUIRE(cpu.RAM[0xFFFF]==123);
    cpu.tick_op();
    REQUIRE(cpu.REG[8]==123);
}