//
// Created by chasingfar on 2021/7/24.
//
#include "asm_test_util.h"

using namespace DynamicFn;

TEST_CASE("function dynamic args and vars","[asm][function][dynamic]"){
	FnDecl fn{"fn(a,b)",2};
	auto [a,b]=fn.getVars<FnArg, FnArg>();//args
	auto [c,d]=fn.getVars<FnVar, FnVar>();//locals
	Label main,aa,bb,cc,dd;
	CPU cpu=run({
		jmp(main),
		fn.impl({
			aa,
			a.load(Reg::A),
			b.load(Reg::B),
			bb,
			add(Reg::C,Reg::A,Reg::B),
			sub(Reg::D,Reg::A,Reg::B),
			cc,
			c.save(Reg::C),
			d.save(Reg::D),
			dd,
			lev(),
		}),
		main,
		fn.call({imm(8),imm(3)}),
	}, {aa});

	REQUIRE(_LOCAL(a.offset)==8);
	REQUIRE(_LOCAL(b.offset)==3);

	run(cpu,{bb});
	REQUIRE(_REG(A)==8);
	REQUIRE(_REG(B)==3);

	run(cpu,{cc});
	REQUIRE(_REG(C)==11);
	REQUIRE(_REG(D)==5);

	run(cpu,{dd});
	REQUIRE(_LOCAL(c.offset)==11);
	REQUIRE(_LOCAL(d.offset)==5);

	run(cpu);
	REQUIRE(_REG(A)==8);
	REQUIRE(_REG(B)==3);
	REQUIRE(_REG(C)==11);
	REQUIRE(_REG(D)==5);
}
TEST_CASE("function nest call","[asm][function][dynamic]"){
	/*
	foo(a){
		c=a+2
		return bar(c)+c
	}
	bar(b){
		d=b+6
		return d
	}
	foo(8)
	*/
	FnDecl foo{"foo(a)",1};
	FnDecl bar{"bar(b)",1};
	auto [a,c]=foo.getVars<FnArg,FnVar>();
	auto [b,d]=bar.getVars<FnArg,FnVar>();
	Label main,aa,bb,cc,dd;
	CPU cpu=run({
		jmp(main),
		foo.impl({
			a.load(Reg::A),
			imm(Reg::B,2),
			add(Reg::A,Reg::A,Reg::B),
			c.save(Reg::A),
			aa,
			bar.call({Reg::A}),
			cc,
			c.load(Reg::B),
			add(Reg::A,Reg::A,Reg::B),
			dd,
			lev(),
		}),
		bar.impl({
			b.load(Reg::A),
			imm(Reg::B,2),
			add(Reg::A,Reg::A,Reg::B),
			d.save(Reg::A),
			bb,
			lev(),
		}),
		main,
		foo.call({imm(8)}),
	}, {aa});

	REQUIRE(_LOCAL(a.offset)==8);
	REQUIRE(_REG(A)==10);
	REQUIRE(_LOCAL(c.offset)==10);

	run(cpu,{bb});
	REQUIRE(_LOCAL(b.offset)==10);
	REQUIRE(_REG(A)==12);
	REQUIRE(_LOCAL(d.offset)==12);

	run(cpu,{cc});
	REQUIRE(_LOCAL(a.offset)==8);
	REQUIRE(_REG(A)==12);
	REQUIRE(_LOCAL(c.offset)==10);

	run(cpu,{dd});
	REQUIRE(_REG(B)==10);
	REQUIRE(_REG(A)==22);

	run(cpu);
	REQUIRE(_REG(A)==22);
}

TEST_CASE("function recursion","[asm][function][dynamic]"){
	/*
	fib(i){
		if(i!=0){
		}else{
			return 0
		}
		i=i-1
		if(i!=0){
		}else{
			return 1
		}
		a=fib(i)
		i=i-1
		return fib(i)+a
	}
	fib(6)
	*/
	FnDecl fib{"fib(i)",1};
	auto [i,a]=fib.getVars<FnArg,FnVar>();

	Label main;
	CPU cpu=run({
		jmp(main),
		fib.impl({
			i.load(Reg::B),
			IF{{push(Reg::B)},{},{{
				imm(Reg::A, 0),
				lev(),
			}}},
			imm(Reg::C, 1),
			sub(Reg::B,Reg::B,Reg::C),
			IF{{push(Reg::B)},{},{{
				imm(Reg::A, 1),
				lev(),
			}}},
			i.save(Reg::B),
			fib.call({Reg::B}),
			a.save(Reg::A),
			i.load(Reg::B),
			imm(Reg::C, 1),
			sub(Reg::B,Reg::B,Reg::C),
			fib.call({Reg::B}),
			a.load(Reg::B),
			add(Reg::A,Reg::A,Reg::B),
			lev(),
		}),
		main,
		fib.call({imm(6)}),
	});
	REQUIRE(_REG(A)==8);
}