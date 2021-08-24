//
// Created by chasingfar on 2021/7/23.
//
#include "asm_test_util.h"


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
			IF{1_u8,
				{imm(Reg::A,5)},
				{imm(Reg::A,6)}
			}
		);
		REQUIRE(_REG(A)==5);
	}
	SECTION("if false"){
		CPU cpu=run(
			IF{0_u8,
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
	RegVar a{Reg::A},b{Reg::B};
	CPU cpu=run({
		imm(Reg::A,0),imm(Reg::B,3),
		While{b,{{
			a.set(add(a,b)),
			b.set(sub(b,1_u8)),
		}}},
	});
	REQUIRE(_REG(A)==6);
	REQUIRE(cpu.isHalt()==true);
}

TEST_CASE("static variable","[asm][advance]"){
	StaticVars vars;
	auto [a]=vars.get<UInt8>({0});
	auto [b,c]=vars.get<UInt8,UInt8>({12},{34});
	Label main;
	CPU cpu=run({
		jmp(main),
		vars,
		main,
		a.set(56_u8),
		b.set(78_u8),
		c.set(90_u8),
	},{main});
	REQUIRE(_STATIC(vars,a.offset)==0);
	REQUIRE(_STATIC(vars,b.offset)==12);
	REQUIRE(_STATIC(vars,c.offset)==34);
	run(cpu);
	REQUIRE(_STATIC(vars,a.offset)==56);
	REQUIRE(_STATIC(vars,b.offset)==78);
	REQUIRE(_STATIC(vars,c.offset)==90);
}
TEST_CASE("static variable with custom type","[asm][advance]"){
	struct Vec:Struct<Vec,UInt8,UInt8,UInt8>{};
	StaticVars vars;
	auto [vec]=vars.get<Vec>({3,7,11});
	auto [x,y,z]=vec.extract();
	
	Label main;
	CPU cpu=run({
		jmp(main),
		vars,
		main,
		x.set(x+1_u8),
		y.set(y+2_u8),
		z.set(z+3_u8),
	},{main});
	REQUIRE(_STATIC(vars,x.offset)==3);
	REQUIRE(_STATIC(vars,y.offset)==7);
	REQUIRE(_STATIC(vars,z.offset)==11);
	run(cpu);
	REQUIRE(_STATIC(vars,x.offset)==4);
	REQUIRE(_STATIC(vars,y.offset)==9);
	REQUIRE(_STATIC(vars,z.offset)==14);
}

TEST_CASE("big variable","[asm][advance]"){
	CPU cpu=run({
		add(0x12f3_u16,0x32cc_u16),
		pop(Reg::B),
		pop(Reg::A),
	});
	REQUIRE(_REG(B)==0x45);
	REQUIRE(_REG(A)==0xbf);
}
TEST_CASE("if cmp","[asm][advance]"){
	op_t T=7,F=6;
	auto _if=[=](const Value<Bool>& cond){
		return IF{cond,
			{imm(Reg::A,T)},
	        {imm(Reg::A,F)}
		};
	};
	SECTION("if !") {
		SECTION("if !0") {
			CPU cpu = run(_if(!0_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if !123") {
			CPU cpu = run(_if(!123_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if !255") {
			CPU cpu = run(_if(!255_u8));
			REQUIRE(_REG(A) == F);
		}
	}
	SECTION("if !=") {
		SECTION("if 3!=5") {
			CPU cpu = run(_if(3_u8 != 5_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 3!=3") {
			CPU cpu = run(_if(3_u8 != 3_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 5!=3") {
			CPU cpu = run(_if(5_u8 != 3_u8));
			REQUIRE(_REG(A) == T);
		}
	}
	SECTION("if ==") {
		SECTION("if 3==5") {
			CPU cpu = run(_if(3_u8 == 5_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 3==3") {
			CPU cpu = run(_if(3_u8 == 3_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 5==3") {
			CPU cpu = run(_if(5_u8 == 3_u8));
			REQUIRE(_REG(A) == F);
		}
	}
	SECTION("if >=") {
		SECTION("if 3>=5") {
			CPU cpu = run(_if(3_u8 >= 5_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 3>=3") {
			CPU cpu = run(_if(3_u8 >= 3_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 5>=3") {
			CPU cpu = run(_if(5_u8 >= 3_u8));
			REQUIRE(_REG(A) == T);
		}
	}
	SECTION("if <") {
		SECTION("if 3<5") {
			CPU cpu = run(_if(3_u8 < 5_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 3<3") {
			CPU cpu = run(_if(3_u8 < 3_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 5<3") {
			CPU cpu = run(_if(5_u8 < 3_u8));
			REQUIRE(_REG(A) == F);
		}
	}
	SECTION("if <=") {
		SECTION("if 3<=5") {
			CPU cpu = run(_if(3_u8 <= 5_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 3<=3") {
			CPU cpu = run(_if(3_u8 <= 3_u8));
			REQUIRE(_REG(A) == T);
		}
		SECTION("if 5<=3") {
			CPU cpu = run(_if(5_u8 <= 3_u8));
			REQUIRE(_REG(A) == F);
		}
	}
	SECTION("if >") {
		SECTION("if 3>5") {
			CPU cpu = run(_if(3_u8 > 5_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 3>3") {
			CPU cpu = run(_if(3_u8 > 3_u8));
			REQUIRE(_REG(A) == F);
		}
		SECTION("if 5>3") {
			CPU cpu = run(_if(5_u8 > 3_u8));
			REQUIRE(_REG(A) == T);
		}
	}
}