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
	REQUIRE(_REG(A)==5);
	run(cpu,{a,b});
	REQUIRE(_REG(A)==10);
	run(cpu,{a,b});
	REQUIRE(_REG(A)==15);
}

TEST_CASE("if","[asm][statement]"){
	CPU cpu;
	SECTION("if true"){
		cpu.init();
		load_run(cpu,{
			IF{1_u8,
				{imm(Reg::A,5)},
				{imm(Reg::A,6)}
			},
			halt(),
		});
		REQUIRE(_REG(A)==5);
	}
	SECTION("if false"){
		cpu.init();
		load_run(cpu,{
			IF{0_u8,
				{imm(Reg::A,5)},
				{imm(Reg::A,6)}
			},
			halt(),
		});
		REQUIRE(_REG(A)==6);
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
	UInt8 a{RegVar::make(Reg::A)},b{RegVar::make(Reg::B)};
	CPU cpu;
	load_run(cpu,{
		imm(Reg::A,0),imm(Reg::B,3),
		While{b,{{
			a.set(add(a,b)),
			b.set(sub(b,1_u8)),
		}}},
		halt(),
	});
	REQUIRE(_REG(A)==6);
	REQUIRE(cpu.is_halt()==true);
}

TEST_CASE("static variable","[asm][statement]"){
	StaticVars vars;
	UInt8 a{vars.preset({0_op})};
	auto [b,c]=vars.preset_vars<UInt8,UInt8>({12_op},{34_op});

	CPU cpu;
	_LOAD_TO(MEM::ram_min,vars);

	REQUIRE(_STATIC(vars,a)==0);
	REQUIRE(_STATIC(vars,b)==12);
	REQUIRE(_STATIC(vars,c)==34);

	load_run(cpu,{
		a.set(56_u8),
		b.set(78_u8),
		c.set(90_u8),
		halt(),
	});

	REQUIRE(_STATIC(vars,a)==56);
	REQUIRE(_STATIC(vars,b)==78);
	REQUIRE(_STATIC(vars,c)==90);
}
TEST_CASE("static variable with custom type","[asm][statement]"){
	struct Vec:Struct<UInt8,UInt8,UInt8>{};
	StaticVars vars;
	Vec vec{vars.preset({3_op,7_op,11_op})};
	auto [x,y,z]=vec.extract();
	
	CPU cpu;
	_LOAD_TO(MEM::ram_min,vars);

	REQUIRE(_STATIC(vars,x)==3);
	REQUIRE(_STATIC(vars,y)==7);
	REQUIRE(_STATIC(vars,z)==11);

	load_run(cpu,{
		x.set(x+1_u8),
		y.set(y+2_u8),
		z.set(z+3_u8),
		halt(),
	});
	
	REQUIRE(_STATIC(vars,x)==4);
	REQUIRE(_STATIC(vars,y)==9);
	REQUIRE(_STATIC(vars,z)==14);
}

TEST_CASE("big variable","[asm][statement]"){
	CPU cpu;
	load_run(cpu,{
		add(0x12f3_u16,0x32cc_u16),
		pop(Reg::B),
		pop(Reg::A),
		halt(),
	});
	REQUIRE(_REG(B)==0x45);
	REQUIRE(_REG(A)==0xbf);
}
TEST_CASE("if cmp","[asm][statement]"){
	op_t T=7,F=6;
	auto _if=[=](Bool cond)->code_t{
		return {
			IF{cond,
				{imm(Reg::A,T)},
	        	{imm(Reg::A,F)}
			},
			halt(),
		};
	};
	CPU cpu;
	SECTION("if !") {
		SECTION("if !0") {
			cpu.init();
			load_run(cpu,_if(!0_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if !123") {
			cpu.init();
			load_run(cpu,_if(!123_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if !255") {
			cpu.init();
			load_run(cpu,_if(!255_u8));
			REQUIRE(_REG(A) == F);
		}
	}
	SECTION("if !=") {
		SECTION("if 3!=5") {
			cpu.init();
			load_run(cpu,_if(3_u8 != 5_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 3!=3") {
			cpu.init();
			load_run(cpu,_if(3_u8 != 3_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 5!=3") {
			cpu.init();
			load_run(cpu,_if(5_u8 != 3_u8));
			REQUIRE(_REG(A) == T);
		}
	}
	SECTION("if ==") {
		SECTION("if 3==5") {
			cpu.init();
			load_run(cpu,_if(3_u8 == 5_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 3==3") {
			cpu.init();
			load_run(cpu,_if(3_u8 == 3_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 5==3") {
			cpu.init();
			load_run(cpu,_if(5_u8 == 3_u8));
			REQUIRE(_REG(A) == F);
		}
	}
	SECTION("if >=") {
		SECTION("if 3>=5") {
			cpu.init();
			load_run(cpu,_if(3_u8 >= 5_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 3>=3") {
			cpu.init();
			load_run(cpu,_if(3_u8 >= 3_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 5>=3") {
			cpu.init();
			load_run(cpu,_if(5_u8 >= 3_u8));
			REQUIRE(_REG(A) == T);
		}
	}
	SECTION("if <") {
		SECTION("if 3<5") {
			cpu.init();
			load_run(cpu,_if(3_u8 < 5_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 3<3") {
			cpu.init();
			load_run(cpu,_if(3_u8 < 3_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 5<3") {
			cpu.init();
			load_run(cpu,_if(5_u8 < 3_u8));
			REQUIRE(_REG(A) == F);
		}
	}
	SECTION("if <=") {
		SECTION("if 3<=5") {
			cpu.init();
			load_run(cpu,_if(3_u8 <= 5_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 3<=3") {
			cpu.init();
			load_run(cpu,_if(3_u8 <= 3_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 5<=3") {
			cpu.init();
			load_run(cpu,_if(5_u8 <= 3_u8));
			REQUIRE(_REG(A) == F);
		}
	}
	SECTION("if >") {
		SECTION("if 3>5") {
			cpu.init();
			load_run(cpu,_if(3_u8 > 5_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 3>3") {
			cpu.init();
			load_run(cpu,_if(3_u8 > 3_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 5>3") {
			cpu.init();
			load_run(cpu,_if(5_u8 > 3_u8));
			REQUIRE(_REG(A) == T);
		}
	}
}