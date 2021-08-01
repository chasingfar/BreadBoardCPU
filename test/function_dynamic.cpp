//
// Created by chasingfar on 2021/7/24.
//
#include "asm_test_util.h"

using namespace DynamicFn;

TEST_CASE("function dynamic args and vars","[asm][function][dynamic]"){
	FnDecl fn{"fn(a,b)",0,2};
	auto [a,b]=fn.getVars<FnU8, FnU8>();//args
	auto [c,d]=fn.getVars<FnU8, FnU8>();//locals
	Label main,aa,bb,cc,dd;
	CPU cpu=run({
		jmp(main),
		fn.impl({
			aa,
			Var(Reg::A).set(a),
			Var(Reg::B).set(b),
			bb,
			Var(Reg::C).set(add(a,b)),
			Var(Reg::D).set(sub(a,b)),
			cc,
			c.set(Var(Reg::C)),
			d.set(Var(Reg::D)),
			dd,
			lev(),
		}),
		main,
		fn(8_u8,3_u8),
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
	FnDecl foo{"foo(a)",1,1};
	FnDecl bar{"bar(b)",1,1};
	auto [a,c]=foo.getVars<FnU8,FnU8>();
	auto [b,d]=bar.getVars<FnU8,FnU8>();
	Label main,aa,bb,cc,dd;
	CPU cpu=run({
		jmp(main),
		foo.impl({
			c.set(add(a,2_u8)),
			aa,
			foo._return(add(bar(c),c))
		}),
		bar.impl({
			d.set(add(b,6_u8)),
			bb,
			bar._return(d),
		}),
		main,
		foo(8_u8),
	}, {aa});

	REQUIRE(_LOCAL(a.offset)==8);
	REQUIRE(_LOCAL(c.offset)==10);

	run(cpu,{bb});
	REQUIRE(_LOCAL(b.offset)==10);
	REQUIRE(_LOCAL(d.offset)==16);

	run(cpu);
	REQUIRE(_STACK_TOP==26);
}

TEST_CASE("function recursion","[asm][function][dynamic]"){
	/*
	fib(i){
		if(i!=0){
			if(i-1!=0){
				return fib(i-1)+fib(i-2)
			}else{
				return 1
			}
		}else{
			return 0
		}
	}
	fib(6)
	*/
	FnDecl fib{"fib(i)",1,1};
	auto [i,a]=fib.getVars<FnU8,FnU8>();

	Label main;
	CPU cpu=run({
		jmp(main),
		fib.impl({
			IF{i,{{
				IF{sub(i,1_u8),{{
					fib._return(add(fib(sub(i,1_u8)),fib(sub(i,2_u8)))),
				}},{{
					fib._return(1_u8),
				}}},
			}},{{
				fib._return(0_u8),
			}}},
		}),
		main,
		fib(6_u8),
	});
	REQUIRE(_STACK_TOP==8);
}