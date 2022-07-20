//
// Created by chasingfar on 2021/7/9.
//
#include "asm_test_util.h"

TEST_CASE("load and save","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	cpu.load(ops_t{12,34},0xABCD);
	REG_(B)=0xAB;
	REG_(A)=0xCD;

	RUN_OP_(load(Reg16::BA));
	REQUIRE(STACK_TOP_ == 12);
	RUN_OP_(load(Reg16::BA, 1));
	REQUIRE(STACK_TOP_ == 34);

	RUN_OP_(save(Reg16::BA));
	REQUIRE(cpu.mem.ram.data[0xABCD]==34);
	RUN_OP_(save(Reg16::BA, 1));
	REQUIRE(cpu.mem.ram.data[0xABCE]==12);
}

TEST_CASE("push and pop","[asm][basic]"){
    CPU cpu;
    cpu.tick_op();

	REG_(B)=123;

	RUN_OP_(push(Reg::B));
    REQUIRE(STACK_TOP_ == 123);
	RUN_OP_(pop(Reg::A));
    REQUIRE(REG_(A) == 123);
}

TEST_CASE("immediate value","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	RUN_OP_(imm(123));
	REQUIRE(STACK_TOP_ == 123);
	RUN_OP_(imm(Label{0xABCD}));
	REQUIRE(STACK_TOP_ == 0xCD);
	cpu.tick_op();
	REQUIRE(STACK_TOP_ == 0xAB);
}

TEST_CASE("jump and branch","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();

	Label a{0xABCD};
	RUN_OP_(jmp(a));
	REQUIRE(REG16_(PC) == *a.addr);

	Label b{0xBBCD},c{0xCBCD};
	SET_FLAG_(CF, Carry::yes);
	RUN_OP_(brc(b));
	REQUIRE(REG16_(PC) == *b.addr);
	SET_FLAG_(CF, Carry::no);
	RUN_OP_(brc(c));
	REQUIRE(REG16_(PC) == (*b.addr) + 3);

	Label d{0x1234},e{0x2234};
	RUN_OP_(imm(0));
	RUN_OP_(brz(d));
	REQUIRE(REG16_(PC) == *d.addr);
	RUN_OP_(imm(1));
	RUN_OP_(brz(e));
	REQUIRE(REG16_(PC) == 2 + (*d.addr) + 3);
}
TEST_CASE("calculation","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();
	SECTION("shift"){
		RUN_OP_(imm(0b1011'0110));
		SECTION("shift left"){
			RUN_OP_(shl());
			REQUIRE(STACK_TOP_ == 0b0110'1100);
		}
		SECTION("rotate left with carry=no"){
			SET_FLAG_(CF, Carry::no);
			RUN_OP_(rcl());
			REQUIRE(STACK_TOP_ == 0b0110'1100);
		}
		SECTION("rotate left with carry=yes"){
			SET_FLAG_(CF, Carry::yes);
			RUN_OP_(rcl());
			REQUIRE(STACK_TOP_ == 0b0110'1101);
		}
		SECTION("shift right"){
			RUN_OP_(shr());
			REQUIRE(STACK_TOP_ == 0b0101'1011);
		}
		SECTION("rotate right with carry=no"){
			SET_FLAG_(CF, Carry::no);
			RUN_OP_(rcr());
			REQUIRE(STACK_TOP_ == 0b0101'1011);
		}
		SECTION("rotate right with carry=yes"){
			SET_FLAG_(CF, Carry::yes);
			RUN_OP_(rcr());
			REQUIRE(STACK_TOP_ == 0b1101'1011);
		}
	}
	SECTION("arithmetic"){
		uint8_t a=123,b=100;
		RUN_OP_(imm(a));
		RUN_OP_(imm(b));
		SECTION("addition"){
			RUN_OP_(add());
			REQUIRE(STACK_TOP_ == a + b);
		}
		SECTION("subtraction"){
			RUN_OP_(sub());
			REQUIRE(STACK_TOP_ == a - b);
		}
		SECTION("addition with carry=no"){
			SET_FLAG_(CF, Carry::no);
			RUN_OP_(adc());
			REQUIRE(STACK_TOP_ == a + b + 0);
		}
		SECTION("addition with carry=yes"){
			SET_FLAG_(CF, Carry::yes);
			RUN_OP_(adc());
			REQUIRE(STACK_TOP_ == a + b + 1);
		}
		SECTION("subtraction with carry=no"){
			SET_FLAG_(CF, Carry::no);
			RUN_OP_(suc());
			REQUIRE(STACK_TOP_ == a - b - 1);
		}
		SECTION("subtraction with carry=yes"){
			SET_FLAG_(CF, Carry::yes);
			RUN_OP_(suc());
			REQUIRE(STACK_TOP_ == a - b - 0);
		}
	}
	SECTION("logic"){
		uint8_t a=0b1100'0101,b=0b1010'1100;
		RUN_OP_(imm(a));
		RUN_OP_(imm(b));
		SECTION("not"){
			RUN_OP_(NOT());
			REQUIRE(STACK_TOP_ == (uint8_t)(~b));
		}
		SECTION("and"){
			RUN_OP_(AND());
			REQUIRE(STACK_TOP_ == (a & b));
		}
		SECTION("or"){
			RUN_OP_(OR());
			REQUIRE(STACK_TOP_ == (a | b));
		}
		SECTION("xor"){
			RUN_OP_(XOR());
			REQUIRE(STACK_TOP_ == (a ^ b));
		}
	}
}