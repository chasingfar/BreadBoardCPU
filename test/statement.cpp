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
	}).run_to_halt({a,b});
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 5);
	cpu.run_to_halt({a,b});
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 10);
	cpu.run_to_halt({a,b});
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 15);
}

TEST_CASE("if","[asm][statement]"){
	CPU cpu;
	SECTION("if true"){
		cpu.init();
		cpu.load({
			if_(1_u8).then({
				asm_(imm(Reg::A,5)),
			}).else_({
				asm_(imm(Reg::A,6)),
			}),
			halt(),
		}).run_to_halt();
		REQUIRE(cpu.get_reg(CPU::Reg::A) == 5);
	}
	SECTION("if false"){
		cpu.init();
		cpu.load({
			if_(0_u8).then({
				asm_(imm(Reg::A, 5)),
			}).else_({
				asm_(imm(Reg::A,6)),
			}),
			halt(),
		}).run_to_halt();
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
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 6);
	REQUIRE(cpu.is_halt()==true);
}
TEST_CASE("while with break and continue","[asm][statement]"){
	/*
	a=0,b=0;
	while(b!=4){
		b+=1;
		if(b==2){
			continue;
		}
		a+=b;
		if(b==3){
			break;
		}
	}
	*/
	CPU cpu;
	cpu.load({
		Reg_A=0_u8,Reg_B=0_u8,
		while_(Reg_B!=4_u8).do_([](auto _){return Stmt{
			Reg_B+=1_u8,
			if_(Reg_B==2_u8).then({
				_.continue_,
			}).end(),
			Reg_A+=Reg_B,
			if_(Reg_B==3_u8).then({
				_.break_,
			}).end(),
		};}).end(),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 3);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 4);
	REQUIRE(cpu.is_halt()==true);
}
TEST_CASE("do while","[asm][statement]"){
	/*
	a=0,b=0;
	do{
		a+=b;
		b+=1
	}while(b!=4);
	do{
		a+=b;
	}while(b!=4);
	*/
	CPU cpu;
	cpu.load({
		Reg_A=0_u8,Reg_B=0_u8,
		do_({
			Reg_A+=Reg_B,
			Reg_B+=1_u8,
		}).while_(Reg_B!=4_u8),
		do_({
			Reg_A+=Reg_B,
		}).while_(Reg_B!=4_u8),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 10);
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 4);
	REQUIRE(cpu.is_halt()==true);
}
TEST_CASE("do while with break and continue","[asm][statement]"){
	/*
	a=0,b=0;
	do{
		b+=1;
		if(b==2){
			continue;
		}
		a+=b;
		if(b==3){
			break;
		}
	}while(b!=4)
	*/
	CPU cpu;
	cpu.load({
		Reg_A=0_u8,Reg_B=0_u8,
		do_([](auto _){return Stmt{
			Reg_B+=1_u8,
			if_(Reg_B==2_u8).then({
				_.continue_,
			}).end(),
			Reg_A+=Reg_B,
			if_(Reg_B==3_u8).then({
				_.break_,
			}).end(),
		};}).while_(Reg_B!=4_u8),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 3);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 4);
	REQUIRE(cpu.is_halt()==true);
}
TEST_CASE("for","[asm][statement]"){
	/*
	a=0;
	for(b=0;b!=4;b+=1){
		a+=b;
	}
	*/
	CPU cpu;
	cpu.load({
		Reg_A=0_u8,
		for_(Reg_B=0_u8,Reg_B!=4_u8,Reg_B+=1_u8).do_({
			Reg_A+=Reg_B,
		}),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 4);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 6);
	REQUIRE(cpu.is_halt()==true);
}
TEST_CASE("for with break and continue","[asm][statement]"){
	/*
	a=0;
	for(b=0;b!=4;b+=1){
		if(b==2){
			continue;
		}
		a+=b;
		if(b==3){
			break;
		}
	}
	*/
	CPU cpu;
	cpu.load({
		Reg_A=0_u8,
		for_(Reg_B=0_u8,Reg_B!=4_u8,Reg_B+=1_u8).do_([](auto _){return Stmt{
			if_(Reg_B==2_u8).then({
				_.continue_,
			}).end(),
			Reg_A+=Reg_B,
			if_(Reg_B==3_u8).then({
				_.break_,
			}).end(),
		};}),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 3);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 4);
	REQUIRE(cpu.is_halt()==true);
}

TEST_CASE("static variable","[asm][statement]"){
	StaticVars vars;

	auto a=vars=0_u8;
	auto b=vars=12_u8;
	auto c=vars=34_u8;

	CPU cpu;
	cpu.load(vars,MEM::ram_min);

	Label aa;
	cpu.load({
		vars.init,
		aa,
		a.set(56_u8),
		b.set(78_u8),
		c.set(90_u8),
		halt(),
	}).run_to_halt({aa});

	REQUIRE(cpu.get_static(vars, a) == 0);
	REQUIRE(cpu.get_static(vars, b) == 12);
	REQUIRE(cpu.get_static(vars, c) == 34);

	cpu.run_to_halt();

	REQUIRE(cpu.get_static(vars, a) == 56);
	REQUIRE(cpu.get_static(vars, b) == 78);
	REQUIRE(cpu.get_static(vars, c) == 90);
}
struct Vec:Struct<u8,u8,u8>{
	#define THIS Vec
	#define BASE Struct<u8,u8,u8>
	#define members M(x) M(y) M(z)
	#include "lang/define_members.h"
};
TEST_CASE("static variable with custom type","[asm][statement]"){
	StaticVars vars;
	
	auto vec=vars=Vec(3_u8,7_u8,11_u8);
	auto [x,y,z]=vec.extract();
	
	CPU cpu;
	cpu.load(vars,MEM::ram_min);

	Label aa;
	cpu.load({
		vars.init,
		aa,
		x.set(x+1_u8),
		y.set(y+2_u8),
		z.set(z+3_u8),
		halt(),
	}).run_to_halt({aa});

	REQUIRE(cpu.get_static(vars, x) == 3);
	REQUIRE(cpu.get_static(vars, y) == 7);
	REQUIRE(cpu.get_static(vars, z) == 11);

	cpu.run_to_halt();
	
	REQUIRE(cpu.get_static(vars, x) == 4);
	REQUIRE(cpu.get_static(vars, y) == 9);
	REQUIRE(cpu.get_static(vars, z) == 14);
}

TEST_CASE("readonly variable","[asm][statement]"){
	ReadOnlyVars rovar;
	
	auto a=rovar=12_u8;
	auto b=rovar=34_u8;

	CPU cpu;

	Label aa;
	cpu.load({
		aa,
		Reg_A=a+b,
		halt(),
		rovar,
	}).run_to_halt({aa});

	REQUIRE(cpu.get_static(rovar, a) == 12);
	REQUIRE(cpu.get_static(rovar, b) == 34);

	cpu.run_to_halt();
	
	REQUIRE(cpu.get_static(rovar, a) == 12);
	REQUIRE(cpu.get_static(rovar, b) == 34);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 12+34);
}

TEST_CASE("big variable","[asm][statement]"){
	CPU cpu;
	cpu.load({
		add(0x12f3_u16,0x32cc_u16),
		pop(Reg::A),
		pop(Reg::B),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 0x45);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 0xbf);
}
TEST_CASE("if cmp","[asm][statement]"){
	op_t T=7,F=6;
	auto test_if=[=](bool_ cond)->Code{
		return {
			if_(cond).then({
				asm_(imm(Reg::A,T)),
			}).else_({
				asm_(imm(Reg::A,F)),
			}),
			halt(),
		};
	};
	CPU cpu;
	SECTION("if !") {
		SECTION("if !0") {
			cpu.init();
			cpu.load(test_if(!0_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if !123") {
			cpu.init();
			cpu.load(test_if(!123_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if !255") {
			cpu.init();
			cpu.load(test_if(!255_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
	}
	SECTION("if !=") {
		SECTION("if 3!=5") {
			cpu.init();
			cpu.load(test_if(3_u8 != 5_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 3!=3") {
			cpu.init();
			cpu.load(test_if(3_u8 != 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 5!=3") {
			cpu.init();
			cpu.load(test_if(5_u8 != 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
	}
	SECTION("if ==") {
		SECTION("if 3==5") {
			cpu.init();
			cpu.load(test_if(3_u8 == 5_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 3==3") {
			cpu.init();
			cpu.load(test_if(3_u8 == 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 5==3") {
			cpu.init();
			cpu.load(test_if(5_u8 == 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
	}
	SECTION("if >=") {
		SECTION("if 3>=5") {
			cpu.init();
			cpu.load(test_if(3_u8 >= 5_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 3>=3") {
			cpu.init();
			cpu.load(test_if(3_u8 >= 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 5>=3") {
			cpu.init();
			cpu.load(test_if(5_u8 >= 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
	}
	SECTION("if <") {
		SECTION("if 3<5") {
			cpu.init();
			cpu.load(test_if(3_u8 < 5_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 3<3") {
			cpu.init();
			cpu.load(test_if(3_u8 < 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 5<3") {
			cpu.init();
			cpu.load(test_if(5_u8 < 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
	}
	SECTION("if <=") {
		SECTION("if 3<=5") {
			cpu.init();
			cpu.load(test_if(3_u8 <= 5_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 3<=3") {
			cpu.init();
			cpu.load(test_if(3_u8 <= 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
		SECTION("if 5<=3") {
			cpu.init();
			cpu.load(test_if(5_u8 <= 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
	}
	SECTION("if >") {
		SECTION("if 3>5") {
			cpu.init();
			cpu.load(test_if(3_u8 > 5_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 3>3") {
			cpu.init();
			cpu.load(test_if(3_u8 > 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == F);
		}
		SECTION("if 5>3") {
			cpu.init();
			cpu.load(test_if(5_u8 > 3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::A) == T);
		}
	}
}
TEST_CASE("if elif else","[asm][statement]"){
	CPU cpu;
	SECTION("without else") {
		auto if_elif=[](u8 sel)->Code{
			return {
				Reg_A=sel,
				Reg_B=13_u8,
				if_(Reg_A==1_u8).then({
					Reg_B=11_u8,
				}).elif(Reg_A==2_u8).then({
					Reg_B=12_u8,
				}).end(),
				halt(),
			};
		};
		SECTION("elif 1") {
			cpu.init();
			cpu.load(if_elif(1_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::B) == 11);
		}
		SECTION("elif 2") {
			cpu.init();
			cpu.load(if_elif(2_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::B) == 12);
		}
		SECTION("elif 3") {
			cpu.init();
			cpu.load(if_elif(3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::B) == 13);
		}
	}
	SECTION("with else") {
		auto if_elif_else=[](u8 sel)->Code{
			return {
				Reg_A=sel,
				if_(Reg_A==1_u8).then({
					Reg_B=11_u8,
				}).elif(Reg_A==2_u8).then({
					Reg_B=12_u8,
				}).else_({
					Reg_B=13_u8,
				}),
				halt(),
			};
		};
		SECTION("else 1") {
			cpu.init();
			cpu.load(if_elif_else(1_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::B) == 11);
		}
		SECTION("else 2") {
			cpu.init();
			cpu.load(if_elif_else(2_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::B) == 12);
		}
		SECTION("else 3") {
			cpu.init();
			cpu.load(if_elif_else(3_u8)).run_to_halt();
			REQUIRE(cpu.get_reg(CPU::Reg::B) == 13);
		}
	}
}