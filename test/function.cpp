//
// Created by chasingfar on 2021/7/24.
//
#include "asm/type.h"
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

	REQUIRE(_LOCAL(a)==8);
	REQUIRE(_LOCAL(b)==3);

	run(cpu,{bb});
	REQUIRE(_REG(A)==8);
	REQUIRE(_REG(B)==3);

	run(cpu,{cc});
	REQUIRE(_REG(C)==11);
	REQUIRE(_REG(D)==5);

	run(cpu,{dd});
	REQUIRE(_LOCAL(c)==11);
	REQUIRE(_LOCAL(d)==5);

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

	REQUIRE(_LOCAL(a)==8);
	REQUIRE(_LOCAL(c)==10);

	run(cpu,{bb});
	REQUIRE(_LOCAL(b)==10);
	REQUIRE(_LOCAL(d)==16);

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
					s+=n,
					n-=1_u8,
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
				_return(UInt16::make(add(a,c),adc(b,d))),
			};
		}),
		pop(Reg::B),
		pop(Reg::A),
	});
	REQUIRE(_REG(B)==0x45);
	REQUIRE(_REG(A)==0xbf);
}
TEST_CASE("inplace function 2","[asm][function]"){
	CPU cpu=run({
		0x12f3_u16,
		0x32cc_u16,
		Fn<UInt16,UInt8,UInt8,UInt8,UInt8>::inplace(
			[](const Label& end,auto ret,auto a,auto b,auto c,auto d)->code_t{
			return {
				ret=UInt16::make(add(a,c),adc(b,d)),
			};
		}),
		pop(Reg::B),
		pop(Reg::A),
	});
	REQUIRE(_REG(B)==0x45);
	REQUIRE(_REG(A)==0xbf);
}
TEST_CASE("function with custom type","[asm][function]"){
	using Vec=Struct<UInt8,UInt8,UInt8>;
	Fn<Vec,Vec> fn{"fn(vec)"};
	auto [vec]=fn.args;
	auto [x,y,z]=vec.extract();
	
	Label main;
	CPU cpu=run({
		jmp(main),
		fn.impl({
			x+=1_u8,
			y+=2_u8,
			vec.get<2>()+=3_u8,
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
TEST_CASE("function with union","[asm][function]"){
	using T=Union<UInt8, UInt16, Array<UInt8, 2>>;
	Fn<UInt16,T> fn{"fn(t)"};
	auto [t]=fn.args;
	auto [t_u8,t_u16,t_arr]=t.extract();
	
	Label main;
	CPU cpu=run({
		jmp(main),
		fn.impl({
			RegVars::C=t_u8,
			RegVars::D=t_arr[0],
			RegVars::E=t_arr[1],
			fn._return(t_u16),
		}),
		main,
		fn(T::make(0x1234_u16)),
		pop(Reg::A),
		pop(Reg::B),
	});
	REQUIRE(_REG(A)==0x12);
	REQUIRE(_REG(B)==0x34);
	REQUIRE(_REG(C)==0x34);
	REQUIRE(_REG(D)==0x34);
	REQUIRE(_REG(E)==0x12);
}
TEST_CASE("function with array","[asm][function]"){
	Fn<Array<UInt8,3>,Array<UInt8,3>> fn{"fn(vec)"};
	auto [arr]=fn.args;
	
	Label main;
	CPU cpu=run({
		jmp(main),
		fn.impl({
			arr[0]+=1_u8,
			arr[1]+=2_u8,
			arr[2]+=3_u8,
			fn._return(arr),
		}),
		main,
		fn(Array<UInt8,3>::make(3_u8,7_u8,11_u8)),
		pop(Reg::A),
		pop(Reg::B),
		pop(Reg::C),
	});
	REQUIRE(_REG(C)==4);
	REQUIRE(_REG(B)==9);
	REQUIRE(_REG(A)==14);
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

	Fn<Ptr<Void>,UInt16> malloc{"malloc(size)"};
	Fn<Void,Ptr<Int8>> fn{"fn(i)"};
	Fn<UInt16> main{"main()"};
	Label heap,aa;
	code_t p{
		main(),
		pop(Reg::B),
		pop(Reg::A),
		halt(),
		malloc.impl<UInt16>([&](auto _return,auto size,auto ret_ptr)->code_t{
			auto [next_ptr]=global.get<UInt16>({heap.get_lazy(0),heap.get_lazy(1)});
			return {
				ret_ptr=next_ptr,
				next_ptr+=size,
				_return(to<Ptr<Void>>(ret_ptr)),
			};
		}),
		fn.impl([&](auto _return,auto i)->code_t{
			return {
				(*i)+=1_i8,
				_return(Val::_void),
			};
		}),
		main.impl<Ptr<Int8>>([&](auto _return,auto i)->code_t{
			return {
				i=to<Ptr<Int8>>(malloc(1_u16)),
				(*i)=3_i8,
				aa,
				fn(i),
				_return(to<UInt16>(i)),
			};
		}),
		global,
		heap
	};
	ops_t ops=(ASM{}<<p<<ASM::END);
	CPU cpu;
	cpu.load(ops);
	run(cpu,{aa});
	REQUIRE(cpu.RAM[*heap] == 3);
	run(cpu);
	REQUIRE(cpu.RAM[*heap] == 4);
	REQUIRE(_REG16(BA) == *heap);
}