//
// Created by chasingfar on 2021/7/24.
//
#include "asm_test_util.h"

using namespace Function;

TEST_CASE("function dynamic args and vars","[asm][function]"){
	Fn<void_(u8, u8)> fn{"fn(a,b)"};
	auto [a,b]=fn.args;
	auto [c,d]=fn.vars<u8, u8>();
	Label main,aa,bb,cc,dd;
	CPU cpu;
	load_run(cpu,{
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
		halt(),
	}, {aa});

	REQUIRE(LOCAL_(a) == 8);
	REQUIRE(LOCAL_(b) == 3);

	run(cpu,{bb});
	REQUIRE(REG_(A) == 8);
	REQUIRE(REG_(B) == 3);

	run(cpu,{cc});
	REQUIRE(REG_(C) == 11);
	REQUIRE(REG_(D) == 5);

	run(cpu,{dd});
	REQUIRE(LOCAL_(c) == 11);
	REQUIRE(LOCAL_(d) == 5);

	run(cpu);
	REQUIRE(REG_(A) == 8);
	REQUIRE(REG_(B) == 3);
	REQUIRE(REG_(C) == 11);
	REQUIRE(REG_(D) == 5);
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
	Fn<u8(u8)> foo{"foo(a)"};
	Fn<u8(u8)> bar{"bar(b)"};
	auto [a]=foo.args;
	auto [c]=foo.vars<u8>();
	auto [b]=bar.args;
	auto [d]=bar.vars<u8>();
	Label main,aa,bb,cc,dd;
	CPU cpu;
	load_run(cpu,{
		jmp(main),
		foo.impl({
			c.set(a+2_u8),
			aa,
			foo.return_(bar(c) + c)
		}),
		bar.impl({
			d.set(b+6_u8),
			bb,
			bar.return_(d),
		}),
		main,
		foo(8_u8),
		halt(),
	}, {aa});

	REQUIRE(LOCAL_(a) == 8);
	REQUIRE(LOCAL_(c) == 10);

	run(cpu,{bb});
	REQUIRE(LOCAL_(b) == 10);
	REQUIRE(LOCAL_(d) == 16);

	run(cpu);
	REQUIRE(STACK_TOP_ == 26);
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
	Fn<u8(u8)> fib{"fib(i)"};

	Label main;
	CPU cpu;
	load_run(cpu,{
		jmp(main),
		fib.impl([&](auto& _,auto i){
			return code_t{
				IF{i<2_u8, {
					_.return_(i),
				}, {
					_.return_(fib(i - 1_u8) + fib(i - 2_u8)),
				}},
			};
		}),
		main,
		fib(6_u8),
		halt(),
	});
	REQUIRE(STACK_TOP_ == 8);
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
	Fn<u8(u8)> sum{"sum(n)"};

	Label main;
	CPU cpu;
	load_run(cpu,{
		jmp(main),
		sum.impl([&](auto& _,u8 n){
			u8 s{_};
			return code_t{
				s.set(0_u8),
				While{n,{{
					s+=n,
					n-=1_u8,
				}}},
				sum.return_(s),
			};
		}),
		main,
		sum(6_u8),
		halt(),
	});
	REQUIRE(STACK_TOP_ == 21);
}
TEST_CASE("inplace function","[asm][function]"){
	CPU cpu;
	load_run(cpu,{
		0x12f3_u16,
		0x32cc_u16,
		InplaceFn<u16(u8,u8,u8,u8)>{
			[](auto& _,auto a,auto b,auto c,auto d)->code_t{
			return {
				_.return_(u16::make(add(a, c), adc(b, d))),
			};
		}},
		pop(Reg::B),
		pop(Reg::A),
	});
	REQUIRE(REG_(B) == 0x45);
	REQUIRE(REG_(A) == 0xbf);
}
TEST_CASE("inplace function 2","[asm][function]"){
	CPU cpu;
	load_run(cpu,{
		0x12f3_u16,
		0x32cc_u16,
		InplaceFn<u16(u8,u8,u8,u8)>{
			[](auto& _,auto a,auto b,auto c,auto d)->code_t{
			return {
				_.ret=u16::make(add(a,c),adc(b,d)),
			};
		}},
		pop(Reg::B),
		pop(Reg::A),
	});
	REQUIRE(REG_(B) == 0x45);
	REQUIRE(REG_(A) == 0xbf);
}
TEST_CASE("function with custom type","[asm][function]"){
	using Vec=Struct<u8,u8,u8>;
	Fn<Vec(Vec)> fn{"fn(vec)"};
	auto [vec]=fn.args;
	auto [x,y,z]=vec.extract();
	
	Label main;
	CPU cpu;
	load_run(cpu,{
		jmp(main),
		fn.impl({
			x+=1_u8,
			y+=2_u8,
			vec.get<2>()+=3_u8,
			fn.return_(vec),
		}),
		main,
		fn(Vec::make(3_u8,7_u8,11_u8)),
		pop(Reg::A),
		pop(Reg::B),
		pop(Reg::C),
		halt(),
	});
	REQUIRE(REG_(C) == 4);
	REQUIRE(REG_(B) == 9);
	REQUIRE(REG_(A) == 14);
}
TEST_CASE("function with union","[asm][function]"){
	using T=Union<u8, u16, Array<u8, 2>>;
	Fn<u16(T)> fn{"fn(t)"};
	auto [t]=fn.args;
	auto [t_u8,t_u16,t_arr]=t.extract();
	
	Label main;
	CPU cpu;
	load_run(cpu,{
		jmp(main),
		fn.impl({
			RegVars::C=t_u8,
			RegVars::D=t_arr[0],
			RegVars::E=t_arr[1],
			fn.return_(t_u16),
		}),
		main,
		fn(T::make(0x1234_u16)),
		pop(Reg::A),
		pop(Reg::B),
		halt(),
	});
	REQUIRE(REG_(A) == 0x12);
	REQUIRE(REG_(B) == 0x34);
	REQUIRE(REG_(C) == 0x34);
	REQUIRE(REG_(D) == 0x34);
	REQUIRE(REG_(E) == 0x12);
}
TEST_CASE("function with array","[asm][function]"){
	Fn<Array<u8,3>(Array<u8,3>)> fn{"fn(vec)"};
	auto [arr]=fn.args;
	
	Label main;
	CPU cpu;
	load_run(cpu,{
		jmp(main),
		fn.impl({
			arr[0]+=1_u8,
			arr[1]+=2_u8,
			arr[2]+=3_u8,
			fn.return_(arr),
		}),
		main,
		fn(Array<u8,3>::make(3_u8,7_u8,11_u8)),
		pop(Reg::A),
		pop(Reg::B),
		pop(Reg::C),
		halt(),
	});
	REQUIRE(REG_(C) == 4);
	REQUIRE(REG_(B) == 9);
	REQUIRE(REG_(A) == 14);
}
TEST_CASE("function pointer","[asm][function]"){
	/*
	main();
	void* malloc(u16 size){
		static next_ptr=[heap];
	    u16 ret_ptr=next_ptr;
	    next_ptr=next_ptr+size;
	    return (void*)ret_ptr;
	}
	void fn(i8* i){
		(*i)=(*i)+1;
		return;
	}
	u16 main(){
		i=(i8*)malloc(1);
		(*i)=3;
		fn(i);
		return (u16)i;
	}
	*/
	StaticVars global;
	Label heap,aa;

	Fn<ptr<void_>(usize)> malloc{[&](auto& _, auto size)->code_t{
		usize next_ptr{global.preset({heap.get_lazy(0),heap.get_lazy(1)})};
		return {
			_.ret=((ptr<void_>)next_ptr),
			next_ptr+=size,
			_.return_(),
		};
	}};
	Fn<void_(ptr<i8>)> fn{[](auto& _, auto i)->code_t{
		return {
			(*i)+=1_i8,
			_.return_(Val::none),
		};
	}};
	Fn<usize()> main{[&](auto& _)->code_t{
		ptr<i8> i{_};
		return {
			i=((ptr<i8>)malloc(1_u16)),
			(*i)=3_i8,
			aa,
			fn(i),
			_.return_((usize) i),
		};
	}};

	CPU cpu;
	LOAD_TO_(MEM::ram_min, (code_t{global, heap}));

	load_run(cpu,{
		main(),
		pop(Reg::B),
		pop(Reg::A),
		halt(),
		malloc,
		fn,
		main,
	},{aa});
	REQUIRE(cpu.mem.get_data(*heap).value_or(0) == 3);
	run(cpu);
	REQUIRE(cpu.mem.get_data(*heap).value_or(0) == 4);
	REQUIRE(REG16_(BA) == *heap);
}
TEST_CASE("library function","[asm][function]"){
	using namespace Library;
	Label main;
	stdlib.clear();
	CPU cpu;
	load_run(cpu,{
		3_u8*5_u8+6_u8*7_u8,
		halt(),
		stdlib,
	});

	REQUIRE(stdlib.fns.size()==1);
	REQUIRE(STACK_TOP_ == 3 * 5 + 6 * 7);
}