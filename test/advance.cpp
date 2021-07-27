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
TEST_CASE("static variable","[asm][advance]"){
	StaticVars vars;
	auto [a]=vars.getVars<StaticVar>(0);
	auto [b,c]=vars.getVars<StaticVar,StaticVar>(12,34);
	Label main;
	CPU cpu=run({
		jmp(main),
		vars,
		main,
		push(56),a.pop,
		push(78),b.pop,
		push(90),c.pop,
	},{main});
	REQUIRE(_STATIC(vars,a.offset)==0);
	REQUIRE(_STATIC(vars,b.offset)==12);
	REQUIRE(_STATIC(vars,c.offset)==34);
	run(cpu);
	REQUIRE(_STATIC(vars,a.offset)==56);
	REQUIRE(_STATIC(vars,b.offset)==78);
	REQUIRE(_STATIC(vars,c.offset)==90);

}