//
// Created by chasingfar on 2020/11/23.
//

#ifndef BREADBOARDCPU_ASM_ASM_H
#define BREADBOARDCPU_ASM_ASM_H

#include "function_dynamic.h"
#include "function_static.h"


namespace BreadBoardCPU::ASM {
	using namespace StaticFn;
	void generate(const std::string& name,auto program) {
		std::ofstream fout{name};
		if (!fout) { return; }
		fout<<ROM(program);
	}
	void simulate(const std::string& name,auto program) {
		std::cout<<name<<std::endl;
		CPU cpu;
		cpu.load(program);
		for (int i = 0; i < 50 && !cpu.isHalt(); ++i) {
			std::cout<<i<<std::endl;
			cpu.tick_op(true);
			cpu.print_reg();
			cpu.print_stack();
			std::cout<<std::endl;
		}
	}
	ops_t test_loop_sum() {
		ASM program{};
		Label a{"a"},b{"b"};
		return program
			<<imm(Reg::A,1)
			<<imm(Reg::B,3)
			>>a
			<<add(Reg::A,Reg::A,Reg::B)
			<<sub(Reg::B,Reg::B,1)
			<<brz(b,Reg::B)
			<<jmp(a)
			>>b
			<<halt()
			<<ASM::END
		;
	}
	ops_t test_save_load() {
		ASM program{};
		Label b,c{0xFFFF-5};
		return program
			<<load(b,Reg::A)
			<<imm(Reg::B,16)
			<<add(Reg::A,Reg::A,Reg::B)
			<<save(c,Reg::A)
			<<halt()
			>>b
			<<code_t{42}
			<<ASM::END
		;
	}
	ops_t test_call_ret() {
		ASM program{};
		Label start,fn_start;
		return program
			<<jmp(start)
			>>fn_start
			<<sub(Reg::A,Reg::A,Reg::B)
			<<ret()
			>>start
			<<imm(Reg::A,5)
			<<imm(Reg::B,3)
			<<call(fn_start)
			<<halt()
			<<ASM::END
		;
	}
	ops_t test_function() {
		ASM program{};
		Label main{"main"};
		FnDecl<2> fn{"fn(c,d)"};
		return program
			<<jmp(main)
			<<fn.impl([](auto& vars,auto c,auto d)->code_t{
				auto e=vars["e"];
				auto f=vars["f"];
				return {
					c.load(Reg::C),
					d.load(Reg::D),
					add(Reg::C,Reg::D,Reg::C),
					e.save(Reg::C),
					lev(),
				};
			})
			>>main
			<<imm(Reg::A,5)
			<<imm(Reg::B,3)
			<<fn.call({Reg::A,Reg::B})
			<<halt()
			<<ASM::END
		;

	}
	void generateASMROM() {
		//simulate("test_loop_sum",test_loop_sum());
		std::cout<<test_function();
		simulate("test_function",test_function());
		//std::cout<<test_if();
		//simulate("test_if",test_if());
		//generate("test_save_load",test_save_load());
		//generate("test_call_ret",test_call_ret());
	}
}
#endif //BREADBOARDCPU_ASM_ASM_H
