//
// Created by chasingfar on 2021/7/24.
//
#include "asm_test_util.h"

using namespace DynamicFn;

TEST_CASE("function dynamic args and vars","[asm][function][dynamic]"){
	FnDecl fn{"fn(a,b)",{"a","b"}};
	auto a=fn["a"],b=fn["b"];//args
	auto c=fn["c"],d=fn["d"];//vars
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
