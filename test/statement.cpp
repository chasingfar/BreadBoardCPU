//
// Created by chasingfar on 2021/7/23.
//
#include "asm_test_util.h"


TEST_CASE("block","[asm][statement]"){
	Label a,b;
	CPU cpu;
	load_run(cpu,Block{{
		imm(Reg::A,5),
		a,
		imm(Reg::A,10),
		b,
		imm(Reg::A,15),
		halt(),
	}},{a,b});
	REQUIRE(REG_(A) == 5);
	run(cpu,{a,b});
	REQUIRE(REG_(A) == 10);
	run(cpu,{a,b});
	REQUIRE(REG_(A) == 15);
}

TEST_CASE("if","[asm][statement]"){
	CPU cpu;
	SECTION("if true"){
		cpu.init();
		load_run(cpu,{
			if_(1_u8).then({
				imm(Reg::A,5),
			}).else_({
				imm(Reg::A,6),
			}),
			halt(),
		});
		REQUIRE(REG_(A) == 5);
	}
	SECTION("if false"){
		cpu.init();
		load_run(cpu,{
			if_(0_u8).then({
				imm(Reg::A,5),
			}).else_({
				imm(Reg::A,6)
			}),
			halt(),
		});
		REQUIRE(REG_(A) == 6);
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
	load_run(cpu,{
		imm(Reg::A,0),imm(Reg::B,3),
		while_(b).do_({
			a.set(add(a,b)),
			b.set(sub(b,1_u8)),
		}),
		halt(),
	});
	REQUIRE(REG_(A) == 6);
	REQUIRE(cpu.is_halt()==true);
}

TEST_CASE("static variable","[asm][statement]"){
	StaticVars vars;
	u8 a{vars.preset({0_op})};
	auto [b,c]=vars.preset_vars<u8,u8>({12_op},{34_op});

	CPU cpu;
	LOAD_TO_(MEM::ram_min, vars);

	REQUIRE(STATIC_(vars, a) == 0);
	REQUIRE(STATIC_(vars, b) == 12);
	REQUIRE(STATIC_(vars, c) == 34);

	load_run(cpu,{
		a.set(56_u8),
		b.set(78_u8),
		c.set(90_u8),
		halt(),
	});

	REQUIRE(STATIC_(vars, a) == 56);
	REQUIRE(STATIC_(vars, b) == 78);
	REQUIRE(STATIC_(vars, c) == 90);
}
TEST_CASE("static variable with custom type","[asm][statement]"){
	struct Vec:Struct<u8,u8,u8>{};
	StaticVars vars;
	Vec vec{vars.preset({3_op,7_op,11_op})};
	auto [x,y,z]=vec.extract();
	
	CPU cpu;
	LOAD_TO_(MEM::ram_min, vars);

	REQUIRE(STATIC_(vars, x) == 3);
	REQUIRE(STATIC_(vars, y) == 7);
	REQUIRE(STATIC_(vars, z) == 11);

	load_run(cpu,{
		x.set(x+1_u8),
		y.set(y+2_u8),
		z.set(z+3_u8),
		halt(),
	});
	
	REQUIRE(STATIC_(vars, x) == 4);
	REQUIRE(STATIC_(vars, y) == 9);
	REQUIRE(STATIC_(vars, z) == 14);
}

TEST_CASE("big variable","[asm][statement]"){
	CPU cpu;
	load_run(cpu,{
		add(0x12f3_u16,0x32cc_u16),
		pop(Reg::B),
		pop(Reg::A),
		halt(),
	});
	REQUIRE(REG_(B) == 0x45);
	REQUIRE(REG_(A) == 0xbf);
}
TEST_CASE("if cmp","[asm][statement]"){
	op_t T=7,F=6;
	auto test_if=[=](bool_ cond)->code_t{
		return {
			if_(cond).then({
				imm(Reg::A,T),
			}).else_({
				imm(Reg::A,F),
			}),
			halt(),
		};
	};
	CPU cpu;
	SECTION("if !") {
		SECTION("if !0") {
			cpu.init();
			load_run(cpu,test_if(!0_u8));
			REQUIRE(REG_(A) == T);
		}
		SECTION("if !123") {
			cpu.init();
			load_run(cpu,test_if(!123_u8));
			REQUIRE(REG_(A) == F);
		}
		SECTION("if !255") {
			cpu.init();
			load_run(cpu,test_if(!255_u8));
			REQUIRE(REG_(A) == F);
		}
	}
	SECTION("if !=") {
		SECTION("if 3!=5") {
			cpu.init();
			load_run(cpu,test_if(3_u8 != 5_u8));
			REQUIRE(REG_(A) == T);
		}
		SECTION("if 3!=3") {
			cpu.init();
			load_run(cpu,test_if(3_u8 != 3_u8));
			REQUIRE(REG_(A) == F);
		}
		SECTION("if 5!=3") {
			cpu.init();
			load_run(cpu,test_if(5_u8 != 3_u8));
			REQUIRE(REG_(A) == T);
		}
	}
	SECTION("if ==") {
		SECTION("if 3==5") {
			cpu.init();
			load_run(cpu,test_if(3_u8 == 5_u8));
			REQUIRE(REG_(A) == F);
		}
		SECTION("if 3==3") {
			cpu.init();
			load_run(cpu,test_if(3_u8 == 3_u8));
			REQUIRE(REG_(A) == T);
		}
		SECTION("if 5==3") {
			cpu.init();
			load_run(cpu,test_if(5_u8 == 3_u8));
			REQUIRE(REG_(A) == F);
		}
	}
	SECTION("if >=") {
		SECTION("if 3>=5") {
			cpu.init();
			load_run(cpu,test_if(3_u8 >= 5_u8));
			REQUIRE(REG_(A) == F);
		}
		SECTION("if 3>=3") {
			cpu.init();
			load_run(cpu,test_if(3_u8 >= 3_u8));
			REQUIRE(REG_(A) == T);
		}
		SECTION("if 5>=3") {
			cpu.init();
			load_run(cpu,test_if(5_u8 >= 3_u8));
			REQUIRE(REG_(A) == T);
		}
	}
	SECTION("if <") {
		SECTION("if 3<5") {
			cpu.init();
			load_run(cpu,test_if(3_u8 < 5_u8));
			REQUIRE(REG_(A) == T);
		}
		SECTION("if 3<3") {
			cpu.init();
			load_run(cpu,test_if(3_u8 < 3_u8));
			REQUIRE(REG_(A) == F);
		}
		SECTION("if 5<3") {
			cpu.init();
			load_run(cpu,test_if(5_u8 < 3_u8));
			REQUIRE(REG_(A) == F);
		}
	}
	SECTION("if <=") {
		SECTION("if 3<=5") {
			cpu.init();
			load_run(cpu,test_if(3_u8 <= 5_u8));
			REQUIRE(REG_(A) == T);
		}
		SECTION("if 3<=3") {
			cpu.init();
			load_run(cpu,test_if(3_u8 <= 3_u8));
			REQUIRE(REG_(A) == T);
		}
		SECTION("if 5<=3") {
			cpu.init();
			load_run(cpu,test_if(5_u8 <= 3_u8));
			REQUIRE(REG_(A) == F);
		}
	}
	SECTION("if >") {
		SECTION("if 3>5") {
			cpu.init();
			load_run(cpu,test_if(3_u8 > 5_u8));
			REQUIRE(REG_(A) == F);
		}
		SECTION("if 3>3") {
			cpu.init();
			load_run(cpu,test_if(3_u8 > 3_u8));
			REQUIRE(REG_(A) == F);
		}
		SECTION("if 5>3") {
			cpu.init();
			load_run(cpu,test_if(5_u8 > 3_u8));
			REQUIRE(REG_(A) == T);
		}
	}
}