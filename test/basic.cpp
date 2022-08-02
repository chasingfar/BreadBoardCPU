//
// Created by chasingfar on 2021/7/9.
//
#include "asm_test_util.h"

TEST_CASE("load and save","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	cpu.load(ops_t{12,34},0xABCD);
	cpu.set_reg16(CPU::Reg16::BA,0xABCD);

	cpu.run_op(load(Reg16::BA));
	REQUIRE(cpu.get_stack_top() == 12);
	cpu.run_op(load(Reg16::BA, 1));
	REQUIRE(cpu.get_stack_top() == 34);

	cpu.run_op(save(Reg16::BA));
	REQUIRE(cpu.get_mem_data(0xABCD)==34);
	cpu.run_op(save(Reg16::BA, 1));
	REQUIRE(cpu.get_mem_data(0xABCE)==12);
}

TEST_CASE("push and pop","[asm][basic]"){
    CPU cpu;
    cpu.tick_op();

	cpu.set_reg(CPU::Reg::B,123);

	cpu.run_op(push(Reg::B));
    REQUIRE(cpu.get_stack_top() == 123);
	cpu.run_op(pop(Reg::A));
    REQUIRE(cpu.get_reg(CPU::Reg::A) == 123);
}

TEST_CASE("immediate value","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	cpu.run_op(imm(123));
	REQUIRE(cpu.get_stack_top() == 123);
	cpu.run_op(imm(Label{0xABCD}));
	REQUIRE(cpu.get_stack_top() == 0xAB);
	cpu.tick_op();
	REQUIRE(cpu.get_stack_top() == 0xCD);
}

TEST_CASE("jump and branch","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	Label a{0xABCD};
	cpu.run_op(jmp(a));
	REQUIRE(cpu.get_reg16(CPU::Reg16::PC) == *a.addr);

	Label b{0xBBCD},c{0xCBCD};
	cpu.set_flag<CPU::Flags::CF>(Carry::yes);
	cpu.run_op(brc(b));
	REQUIRE(cpu.get_reg16(CPU::Reg16::PC) == *b.addr);
	cpu.set_flag<CPU::Flags::CF>(Carry::no);
	cpu.run_op(brc(c));
	REQUIRE(cpu.get_reg16(CPU::Reg16::PC) == (*b.addr) + 3);

	Label d{0x1234},e{0x2234};
	cpu.run_op(imm(0));
	cpu.run_op(brz(d));
	REQUIRE(cpu.get_reg16(CPU::Reg16::PC) == *d.addr);
	cpu.run_op(imm(1));
	cpu.run_op(brz(e));
	REQUIRE(cpu.get_reg16(CPU::Reg16::PC) == 2 + (*d.addr) + 3);
}
TEST_CASE("calculation","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();
	SECTION("shift"){
		cpu.run_op(imm(0b1011'0110));
		SECTION("shift left"){
			cpu.run_op(shl());
			REQUIRE(cpu.get_stack_top() == 0b0110'1100);
		}
		SECTION("rotate left with carry=no"){
			cpu.set_flag<CPU::Flags::CF>(Carry::no);
			cpu.run_op(rcl());
			REQUIRE(cpu.get_stack_top() == 0b0110'1100);
		}
		SECTION("rotate left with carry=yes"){
			cpu.set_flag<CPU::Flags::CF>(Carry::yes);
			cpu.run_op(rcl());
			REQUIRE(cpu.get_stack_top() == 0b0110'1101);
		}
		SECTION("shift right"){
			cpu.run_op(shr());
			REQUIRE(cpu.get_stack_top() == 0b0101'1011);
		}
		SECTION("rotate right with carry=no"){
			cpu.set_flag<CPU::Flags::CF>(Carry::no);
			cpu.run_op(rcr());
			REQUIRE(cpu.get_stack_top() == 0b0101'1011);
		}
		SECTION("rotate right with carry=yes"){
			cpu.set_flag<CPU::Flags::CF>(Carry::yes);
			cpu.run_op(rcr());
			REQUIRE(cpu.get_stack_top() == 0b1101'1011);
		}
	}
	SECTION("arithmetic"){
		uint8_t a=123,b=100;
		cpu.run_op(imm(a));
		cpu.run_op(imm(b));
		SECTION("addition"){
			cpu.run_op(add());
			REQUIRE(cpu.get_stack_top() == a + b);
		}
		SECTION("subtraction"){
			cpu.run_op(sub());
			REQUIRE(cpu.get_stack_top() == a - b);
		}
		SECTION("addition with carry=no"){
			cpu.set_flag<CPU::Flags::CF>(Carry::no);
			cpu.run_op(adc());
			REQUIRE(cpu.get_stack_top() == a + b + 0);
		}
		SECTION("addition with carry=yes"){
			cpu.set_flag<CPU::Flags::CF>(Carry::yes);
			cpu.run_op(adc());
			REQUIRE(cpu.get_stack_top() == a + b + 1);
		}
		SECTION("subtraction with carry=no"){
			cpu.set_flag<CPU::Flags::CF>(Carry::no);
			cpu.run_op(suc());
			REQUIRE(cpu.get_stack_top() == a - b - 1);
		}
		SECTION("subtraction with carry=yes"){
			cpu.set_flag<CPU::Flags::CF>(Carry::yes);
			cpu.run_op(suc());
			REQUIRE(cpu.get_stack_top() == a - b - 0);
		}
	}
	SECTION("logic"){
		uint8_t a=0b1100'0101,b=0b1010'1100;
		cpu.run_op(imm(a));
		cpu.run_op(imm(b));
		SECTION("not"){
			cpu.run_op(NOT());
			REQUIRE(cpu.get_stack_top() == (uint8_t)(~b));
		}
		SECTION("and"){
			cpu.run_op(AND());
			REQUIRE(cpu.get_stack_top() == (a & b));
		}
		SECTION("or"){
			cpu.run_op(OR());
			REQUIRE(cpu.get_stack_top() == (a | b));
		}
		SECTION("xor"){
			cpu.run_op(XOR());
			REQUIRE(cpu.get_stack_top() == (a ^ b));
		}
	}
}