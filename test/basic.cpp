//
// Created by chasingfar on 2021/7/9.
//
#include "asm_test_util.h"

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
	CPU cpu;
	cpu.tick_op();

	Label a{0xABCD};
	_RUN_OP(jmp(a));
	REQUIRE(_REG16(PC) == *a.addr);

	Label b{0xBBCD},c{0xCBCD};
	_SET_FLAG(CF,Carry::yes);
	_RUN_OP(brc(b));
	REQUIRE(_REG16(PC)  == *b.addr);
	_SET_FLAG(CF,Carry::no);
	_RUN_OP(brc(c));
	REQUIRE(_REG16(PC)  == (*b.addr)+3);

	Label d{0x1234},e{0x2234};
	_RUN_OP(imm(0));
	_RUN_OP(brz(d));
	REQUIRE(_REG16(PC) == *d.addr);
	_RUN_OP(imm(1));
	_RUN_OP(brz(e));
	REQUIRE(_REG16(PC) == 2+(*d.addr)+3);
}
TEST_CASE("calculation","[asm][basic]"){
	CPU cpu;
	cpu.tick_op();
	SECTION("shift"){
		_RUN_OP(imm(0b1011'0110));
		SECTION("shift left"){
			_RUN_OP(shl());
			REQUIRE(_STACK_TOP == 0b0110'1100);
		}
		SECTION("rotate left with carry=no"){
			_SET_FLAG(CF,Carry::no);
			_RUN_OP(rcl());
			REQUIRE(_STACK_TOP == 0b0110'1100);
		}
		SECTION("rotate left with carry=yes"){
			_SET_FLAG(CF,Carry::yes);
			_RUN_OP(rcl());
			REQUIRE(_STACK_TOP == 0b0110'1101);
		}
		SECTION("shift right"){
			_RUN_OP(shr());
			REQUIRE(_STACK_TOP == 0b0101'1011);
		}
		SECTION("rotate right with carry=no"){
			_SET_FLAG(CF,Carry::no);
			_RUN_OP(rcr());
			REQUIRE(_STACK_TOP == 0b0101'1011);
		}
		SECTION("rotate right with carry=yes"){
			_SET_FLAG(CF,Carry::yes);
			_RUN_OP(rcr());
			REQUIRE(_STACK_TOP == 0b1101'1011);
		}
	}
	SECTION("arithmetic"){
		uint8_t a=123,b=100;
		_RUN_OP(imm(a));
		_RUN_OP(imm(b));
		SECTION("addition"){
			_RUN_OP(add());
			REQUIRE(_STACK_TOP == a+b);
		}
		SECTION("subtraction"){
			_RUN_OP(sub());
			REQUIRE(_STACK_TOP == a-b);
		}
		SECTION("addition with carry=no"){
			_SET_FLAG(CF,Carry::no);
			_RUN_OP(adc());
			REQUIRE(_STACK_TOP == a+b+0);
		}
		SECTION("addition with carry=yes"){
			_SET_FLAG(CF,Carry::yes);
			_RUN_OP(adc());
			REQUIRE(_STACK_TOP == a+b+1);
		}
		SECTION("subtraction with carry=no"){
			_SET_FLAG(CF,Carry::no);
			_RUN_OP(suc());
			REQUIRE(_STACK_TOP == a-b-1);
		}
		SECTION("subtraction with carry=yes"){
			_SET_FLAG(CF,Carry::yes);
			_RUN_OP(suc());
			REQUIRE(_STACK_TOP == a-b-0);
		}
	}
	SECTION("logic"){
		uint8_t a=0b1100'0101,b=0b1010'1100;
		_RUN_OP(imm(a));
		_RUN_OP(imm(b));
		SECTION("not"){
			_RUN_OP(NOT());
			REQUIRE(_STACK_TOP == (uint8_t)(~b));
		}
		SECTION("and"){
			_RUN_OP(AND());
			REQUIRE(_STACK_TOP == (a&b));
		}
		SECTION("or"){
			_RUN_OP(OR());
			REQUIRE(_STACK_TOP == (a|b));
		}
		SECTION("xor"){
			_RUN_OP(XOR());
			REQUIRE(_STACK_TOP == (a^b));
		}
	}
}
#undef _REG
#undef _REG16
#undef _STACK_TOP
#undef _STACK_INSERT
#undef _RUN_OP
#undef _SET_FLAG