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
	cpu.load({
		jmp(main),
		fn.impl({
			aa,
			Reg_A.set(a),
			Reg_B.set(b),
			bb,
			Reg_C.set(add(a,b)),
			Reg_D.set(sub(a,b)),
			cc,
			c.set(Reg_C),
			d.set(Reg_D),
			dd,
			void_{lev()},
		}),
		main,
		fn(8_u8,3_u8),
		halt(),
	}).run_to_halt({aa});

	REQUIRE(cpu.get_local(a) == 8);
	REQUIRE(cpu.get_local(b) == 3);

	cpu.run_to_halt({bb});
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 8);
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 3);

	cpu.run_to_halt({cc});
	REQUIRE(cpu.get_reg(CPU::Reg::C) == 11);
	REQUIRE(cpu.get_reg(CPU::Reg::D) == 5);

	cpu.run_to_halt({dd});
	REQUIRE(cpu.get_local(c) == 11);
	REQUIRE(cpu.get_local(d) == 5);

	cpu.run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 8);
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 3);
	REQUIRE(cpu.get_reg(CPU::Reg::C) == 11);
	REQUIRE(cpu.get_reg(CPU::Reg::D) == 5);
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
	cpu.load({
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
	}).run_to_halt({aa});

	REQUIRE(cpu.get_local(a) == 8);
	REQUIRE(cpu.get_local(c) == 10);

	cpu.run_to_halt({bb});
	REQUIRE(cpu.get_local(b) == 10);
	REQUIRE(cpu.get_local(d) == 16);

	cpu.run_to_halt();
	REQUIRE(cpu.get_stack_top() == 26);
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
	cpu.load({
		jmp(main),
		fib.impl([&](auto& _,auto i)->Stmt{
			return {
				if_(i < 2_u8).then({
					_.return_(i),
				}).else_({
					_.return_(fib(i - 1_u8) + fib(i - 2_u8)),
				}).end(),
			};
		}),
		main,
		fib(6_u8),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_stack_top() == 8);
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
	cpu.load({
		jmp(main),
		sum.impl([&](auto& _,u8 n)->Stmt{
			u8 s{_};
			return {
				s.set(0_u8),
				while_(n).do_({
					s+=n,
					n-=1_u8,
				}).end(),
				sum.return_(s),
			};
		}),
		main,
		sum(6_u8),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_stack_top() == 21);
}
TEST_CASE("inplace function","[asm][function]"){
	CPU cpu;
	cpu.load({
		0x12f3_u16,
		0x32cc_u16,
		InplaceFn<u16(u8,u8,u8,u8)>{
			[](auto& _,auto a,auto b,auto c,auto d)->Stmt{
			return {
				_.return_(u16::make(add(a, c), adc(b, d))),
			};
		}},
		pop(Reg::B),
		pop(Reg::A),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 0x45);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 0xbf);
}
TEST_CASE("inplace function 2","[asm][function]"){
	CPU cpu;
	cpu.load({
		0x12f3_u16,
		0x32cc_u16,
		InplaceFn<u16(u8,u8,u8,u8)>{
			[](auto& _,auto a,auto b,auto c,auto d)->Stmt{
			return {
				_.ret=u16::make(add(a,c),adc(b,d)),
			};
		}},
		pop(Reg::B),
		pop(Reg::A),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 0x45);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 0xbf);
}
TEST_CASE("function with custom type","[asm][function]"){
	using Vec=Struct<u8,u8,u8>;
	Fn<Vec(Vec)> fn{"fn(vec)"};
	auto [vec]=fn.args;
	auto [x,y,z]=vec.extract();
	
	Label main;
	CPU cpu;
	cpu.load({
		jmp(main),
		fn.impl({
			x+=1_u8,
			y+=2_u8,
			vec.get<2>()+=3_u8,
			fn.return_(vec),
		}),
		main,
		fn(Vec(3_u8,7_u8,11_u8)),
		pop(Reg::A),
		pop(Reg::B),
		pop(Reg::C),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::C) == 4);
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 9);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 14);
}
struct Vec:Struct<u8,u8,u8>{
	#define THIS Vec
	#define BASE Struct<u8,u8,u8>
	#define members M(x) M(y) M(z)
	#include "lang/define_members.h"
};
struct Ball:Struct<Vec,u8>{
	#define THIS Ball
	#define BASE Struct<Vec,u8>
	#define members M(pos) M(r)
	#include "lang/define_members.h"
};
TEST_CASE("function with custom composite type","[asm][function]"){
	Fn<Ball(Ball)> fn{"fn(ball)"};
	
	Label main;
	CPU cpu;
	cpu.load({
		jmp(main),
		fn.impl([](auto& _,Ball ball)->Stmt{
			return {
				ball.pos.x+=9_u8,
				ball.pos.y+=2_u8,
				ball.r+=5_u8,
				_.return_(ball),
			};
		}),
		main,
		fn(Ball(Vec(3_u8,7_u8,11_u8),1_u8)),
		pop(Reg::A),
		pop(Reg::B),
		pop(Reg::C),
		pop(Reg::D),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::D) == 12);
	REQUIRE(cpu.get_reg(CPU::Reg::C) == 9);
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 11);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 6);
}
TEST_CASE("function with union","[asm][function]"){
	using T=Union<u8, u16, Array<u8, 2>>;
	Fn<u16(T)> fn{"fn(t)"};
	auto [t]=fn.args;
	auto [t_u8,t_u16,t_arr]=t.extract();
	
	Label main;
	CPU cpu;
	cpu.load({
		jmp(main),
		fn.impl({
			Reg_C=t_u8,
			Reg_D=t_arr[0],
			Reg_E=t_arr[1],
			fn.return_(t_u16),
		}),
		main,
		fn(T(0x1234_u16)),
		pop(Reg::A),
		pop(Reg::B),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 0x12);
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 0x34);
	REQUIRE(cpu.get_reg(CPU::Reg::C) == 0x34);
	REQUIRE(cpu.get_reg(CPU::Reg::D) == 0x34);
	REQUIRE(cpu.get_reg(CPU::Reg::E) == 0x12);
}
TEST_CASE("function with array","[asm][function]"){
	Fn<Array<u8,3>(Array<u8,3>)> fn{"fn(vec)"};
	auto [arr]=fn.args;
	
	Label main;
	CPU cpu;
	cpu.load({
		jmp(main),
		fn.impl({
			arr[0]+=1_u8,
			arr[1]+=2_u8,
			arr[2]+=3_u8,
			fn.return_(arr),
		}),
		main,
		fn(Array<u8,3>(3_u8,7_u8,11_u8)),
		pop(Reg::A),
		pop(Reg::B),
		pop(Reg::C),
		halt(),
	}).run_to_halt();
	REQUIRE(cpu.get_reg(CPU::Reg::C) == 4);
	REQUIRE(cpu.get_reg(CPU::Reg::B) == 9);
	REQUIRE(cpu.get_reg(CPU::Reg::A) == 14);
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

	Fn<ptr<void_>(usize)> malloc{[&](auto& _, auto size)->Stmt{
		auto next_ptr=global=ptr<u8>(heap);
		return {
			_.ret=(ptr<void_>)next_ptr,
			next_ptr+=size,
			_.return_(),
		};
	}};
	Fn<void_(ptr<i8>)> fn{[](auto& _, auto i)->Stmt{
		return {
			(*i)+=1_i8,
			_.return_(Val::none),
		};
	}};
	Fn<usize()> main{[&](auto& _)->Stmt{
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
	cpu.load({global, heap},MEM::ram_min);

	cpu.load({
		global.init,
		main(),
		pop(Reg::B),
		pop(Reg::A),
		halt(),
		malloc,
		fn,
		main,
	}).run_to_halt({aa});
	REQUIRE(cpu.mem.get_data(*heap).value_or(0) == 3);
	cpu.run_to_halt();
	REQUIRE(cpu.mem.get_data(*heap).value_or(0) == 4);
	REQUIRE(cpu.get_reg16(CPU::Reg16::BA) == *heap);
}
TEST_CASE("library function","[asm][function]"){
	using namespace Library;
	Label main;
	stdlib.clear();
	CPU cpu;
	cpu.load({
		3_u8*5_u8+6_u8*7_u8,
		halt(),
		stdlib,
	}).run_to_halt();

	REQUIRE(stdlib.fns.size()==1);
	REQUIRE(cpu.get_stack_top() == 3 * 5 + 6 * 7);
}