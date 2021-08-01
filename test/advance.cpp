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
	Var a{Reg::A},b{Reg::B};
	CPU cpu=run({
		imm(Reg::A,0),imm(Reg::B,3),
		While{b,{{
			a.set(add(a,b)),
			b.set(sub(b,1_u8)),
		}}},
	});
	REQUIRE(_REG(A)==6);
}
TEST_CASE("static variable","[asm][advance]"){
	StaticVars vars;
	auto [a]=vars.getVars<uint8_t>(0);
	auto [b,c]=vars.getVars<uint8_t,uint8_t>(12,34);
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
TEST_CASE("big variable","[asm][advance]"){
	/*CPU cpu=run({
		add2(code_t{imm(0xf3),imm(0x12)},code_t{imm(0xcc),imm(0x32)}),
		pop(Reg::B),
		pop(Reg::A),
	});
	REQUIRE(_REG(B)==0x45);
	REQUIRE(_REG(A)==0xbf);*/
}