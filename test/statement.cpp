//
// Created by chasingfar on 2021/7/23.
//
#include "asm_test_util.h"


TEST_CASE("program","[asm][statement]"){
	Label a,b;
	CPU cpu;
	cpu.load({
		imm(Reg::A,5),
		a,
		imm(Reg::A,10),
		b,
		imm(Reg::A,15),
		halt(),
	}).run({a,b});
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 5);
	cpu.run({a,b});
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 10);
	cpu.run({a,b});
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 15);
}

TEST_CASE("if","[asm][statement]"){
	CPU cpu;
	SECTION("if true"){
		cpu.init();
		cpu.load({
			if_(1_u8).then({
				void_{imm(Reg::A,5)},
			}).else_({
				void_{imm(Reg::A,6)},
			}).end(),
			halt(),
		}).run();
		REQUIRE(cpu.get_reg(CPU::Reg::A) == 5);
	}
	SECTION("if false"){
		cpu.init();
		cpu.load({
			if_(0_u8).then({
				void_{imm(Reg::A, 5)},
			}).else_({
				void_{imm(Reg::A,6)},
			}).end(),
			halt(),
		}).run();
		REQUIRE(cpu.get_reg(CPU::Reg::A) == 6);
	}
}
TEST_CASE("while","[asm][statement]"){
	/*
	a=0,b=3;
	while(b){
		a+=b;
		b-=1;
	}
	*/
	u8 a{RegVar::make(Reg::A)},b{RegVar::make(Reg::B)};
	CPU cpu;
	cpu.load({
		imm(Reg::A,0),imm(Reg::B,3),
		while_(b).do_({
			a.set(add(a, b)),
			b.set(sub(b, 1_u8))
		}).end(),
		halt(),
	}).run();
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 6);
	REQUIRE(cpu.is_halt()==true);
}

TEST_CASE("static variable","[asm][statement]"){
	StaticVars vars;
	u8 a{vars.preset({0_op})};
	auto [b,c]=vars.preset_vars<u8,u8>({12_op},{34_op});

	CPU cpu;
	cpu.load(vars,MEM::ram_min);

	REQUIRE(cpu.get_static(vars, a) == 0);
	REQUIRE(cpu.get_static(vars, b) == 12);
	REQUIRE(cpu.get_static(vars, c) == 34);

	cpu.load({
		a.set(56_u8),
		b.set(78_u8),
		c.set(90_u8),
		halt(),
	}).run();

	REQUIRE(cpu.get_static(vars, a) == 56);
	REQUIRE(cpu.get_static(vars, b) == 78);
	REQUIRE(cpu.get_static(vars, c) == 90);
}
TEST_CASE("static variable with custom type","[asm][statement]"){
	struct Vec:Struct<u8,u8,u8>{
		using This = Vec;
		using Base = Struct<u8,u8,u8>;
		DEF_TYPE0
	};
	StaticVars vars;
	Vec vec{vars.preset({3_op,7_op,11_op})};
	auto [x,y,z]=vec.extract();
	
	CPU cpu;
	cpu.load(vars,MEM::ram_min);

	REQUIRE(cpu.get_static(vars, x) == 3);
	REQUIRE(cpu.get_static(vars, y) == 7);
	REQUIRE(cpu.get_static(vars, z) == 11);

	cpu.load({
		x.set(x+1_u8),
		y.set(y+2_u8),
		z.set(z+3_u8),
		halt(),
	}).run();
	
	REQUIRE(cpu.get_static(vars, x) == 4);
	REQUIRE(cpu.get_static(vars, y) == 9);
	REQUIRE(cpu.get_static(vars, z) == 14);
}

TEST_CASE("big variable","[asm][statement]"){
	CPU cpu;
	cpu.load({
		add(0x12f3_u16,0x32cc_u16),
		pop(Reg::B),
		pop(Reg::A),
		halt(),
	}).run();
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 0x45);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 0xbf);
}
TEST_CASE("if cmp","[asm][statement]"){
	op_t T=7,F=6;
	auto test_if=[=](bool_ cond)->Code{
		return {
			if_(cond).then({
				void_{imm(Reg::A,T)},
			}).else_({
				void_{imm(Reg::A,F)},
			}).end(),
			halt(),
		};
	};
	CPU cpu;
	SECTION("if !") {
		SECTION("if !0") {
			cpu.init();
			cpu.load(test_if(!0_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if !123") {
			cpu.init();
			cpu.load(test_if(!123_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if !255") {
			cpu.init();
			cpu.load(test_if(!255_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
	}
	SECTION("if !=") {
		SECTION("if 3!=5") {
			cpu.init();
			cpu.load(test_if(3_u8 != 5_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 3!=3") {
			cpu.init();
			cpu.load(test_if(3_u8 != 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 5!=3") {
			cpu.init();
			cpu.load(test_if(5_u8 != 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
	}
	SECTION("if ==") {
		SECTION("if 3==5") {
			cpu.init();
			cpu.load(test_if(3_u8 == 5_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 3==3") {
			cpu.init();
			cpu.load(test_if(3_u8 == 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 5==3") {
			cpu.init();
			cpu.load(test_if(5_u8 == 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
	}
	SECTION("if >=") {
		SECTION("if 3>=5") {
			cpu.init();
			cpu.load(test_if(3_u8 >= 5_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 3>=3") {
			cpu.init();
			cpu.load(test_if(3_u8 >= 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 5>=3") {
			cpu.init();
			cpu.load(test_if(5_u8 >= 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
	}
	SECTION("if <") {
		SECTION("if 3<5") {
			cpu.init();
			cpu.load(test_if(3_u8 < 5_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 3<3") {
			cpu.init();
			cpu.load(test_if(3_u8 < 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 5<3") {
			cpu.init();
			cpu.load(test_if(5_u8 < 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
	}
	SECTION("if <=") {
		SECTION("if 3<=5") {
			cpu.init();
			cpu.load(test_if(3_u8 <= 5_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 3<=3") {
			cpu.init();
			cpu.load(test_if(3_u8 <= 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 5<=3") {
			cpu.init();
			cpu.load(test_if(5_u8 <= 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
	}
	SECTION("if >") {
		SECTION("if 3>5") {
			cpu.init();
			cpu.load(test_if(3_u8 > 5_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 3>3") {
			cpu.init();
			cpu.load(test_if(3_u8 > 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 5>3") {
			cpu.init();
			cpu.load(test_if(5_u8 > 3_u8)).run();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
	}
}