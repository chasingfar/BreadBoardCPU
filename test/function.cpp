//
// Created by chasingfar on 2021/7/24.
//
#include "asm_test_util.h"

using namespace Function;

TEST_CASE("function dynamic args and vars","[asm][function]"){
	Fn<Void,UInt8,UInt8> fn{"fn(a,b)"};
	auto [a,b]=fn.args;
	auto [c,d]=fn.local<UInt8, UInt8>();
	Label main,aa,bb,cc,dd;
	CPU cpu=run({
		jmp(main),
		fn.impl({
			aa,
			RegVars::A.set(a),
			RegVars::B.set(b),
			bb,
			RegVars::C.set(add(a,b)),
			RegVars::D.set(sub(a,b)),
			cc,
			c.set(RegVars::C),
			d.set(RegVars::D),
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
TEST_CASE("function nest call","[asm][function]"){
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
	Fn<UInt8,UInt8> foo{"foo(a)"};
	Fn<UInt8,UInt8> bar{"bar(b)"};
	auto [a]=foo.args;
	auto [c]=foo.local<UInt8>();
	auto [b]=bar.args;
	auto [d]=bar.local<UInt8>();
	Label main,aa,bb,cc,dd;
	CPU cpu=run({
		jmp(main),
		foo.impl({
			c.set(a+2_u8),
			aa,
			foo._return(bar(c)+c)
		}),
		bar.impl({
			d.set(b+6_u8),
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

TEST_CASE("function recursion","[asm][function]"){
	/*
	fib(i){
		if(i<2){
			return i
		}else{
			return fib(i-1)+fib(i-2)
	    }
	}
	fib(6)
	*/
	Fn<UInt8,UInt8> fib{"fib(i)"};

	Label main;
	CPU cpu=run({
		jmp(main),
		fib.impl([&](auto i){
			return code_t{
				IF{i<2_u8, {
					fib._return(i),
				}, {
					fib._return(fib(i - 1_u8) + fib(i - 2_u8)),
				}},
			};
		}),
		main,
		fib(6_u8),
	});
	REQUIRE(_STACK_TOP==8);
}
TEST_CASE("function sum","[asm][function]"){
	/*
	sum(n){
	    s=0
	    while(n){
	        s=s+n
	        n=n-1
	    }
	    return s
	}
	sum(6)
	*/
	Fn<UInt8,UInt8> sum{"sum(n)"};

	Label main;
	CPU cpu=run({
		jmp(main),
		sum.impl<UInt8>([&](auto n,auto s){
			return code_t{
				s.set(0_u8),
				While{n,{{
					s.set(s+n),
					n.set(n-1_u8),
				}}},
				sum._return(s),
			};
		}),
		main,
		sum(6_u8),
	});
	REQUIRE(_STACK_TOP==21);
}
TEST_CASE("inplace function","[asm][function]"){
	CPU cpu=run({
		0x12f3_u16,
		0x32cc_u16,
		Fn<UInt16,UInt8,UInt8,UInt8,UInt8>::inplace(
			[](auto _return,auto a,auto b,auto c,auto d)->code_t{
			return {
				_return(Expr<UInt16>{{
					a,c,add(),
					b,d,adc(),
				}}),
			};
		}),
		pop(Reg::B),
		pop(Reg::A),
	});
	REQUIRE(_REG(B)==0x45);
	REQUIRE(_REG(A)==0xbf);
}
TEST_CASE("function with custom type","[asm][function]"){
	struct Vec:Struct<Vec,UInt8,UInt8,UInt8>{};
	Fn<Vec,Vec> fn{"fn(vec)"};
	auto [vec]=fn.args;
	auto [x,y,z]=vec.extract();
	
	Label main;
	CPU cpu=run({
		jmp(main),
		fn.impl({
			x.set(x+1_u8),
			y.set(y+2_u8),
			z.set(z+3_u8),
			fn._return(vec),
		}),
		main,
		fn(Vec::make(3_u8,7_u8,11_u8)),
		pop(Reg::A),
		pop(Reg::B),
		pop(Reg::C),
	});
	REQUIRE(_REG(C)==4);
	REQUIRE(_REG(B)==9);
	REQUIRE(_REG(A)==14);
}