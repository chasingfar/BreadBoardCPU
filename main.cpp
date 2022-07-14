#include <cstddef>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
/*
#define CPU_DEBUG 0
#include "cpu/regset_sram/mctrl.h"
#include "include/cpu/regfile8x16/opcode.h"
#include "include/asm/asm.h"
#include "include/lang/lang.h"*/
#include "include/cpu/components.h"
#include "include/lang/lang.h"
/*struct B:std::enable_shared_from_this<B>{
	int i;
	B(){
		i=5;
		std::cout<<"B construct"<<std::endl;
	}
	virtual void c()=0;
	auto get(){
		return shared_from_this();
	}
};
struct C:B{
	C(){
		std::cout<<"C construct"<<std::endl;
	}
	void c() override{
		std::cout<<"i="<<i<<std::endl;
	}
};

template<template<typename> typename Ptr=std::shared_ptr>
struct A{
	//std::vector<B&> b{};
	Ptr<B> b;
	Ptr<C> make(){
		Ptr<C> c{new C()};
		b=c;
		return c;
	}
};*/
namespace Circuit{
	using namespace BBCPU;
	using namespace Regs;

	struct CU:CUBase<MARG::size,MCTRL::size,MARG::opcode::size>{
		Port<MCTRL::alu::size> CMS;
		Port<MCTRL::io::Bs::size> bs;
		Port<MCTRL::io::Rs::size> rs;
		Port<MCTRL::io::dir::size> dir;
		Enable rs_en;
		CU(std::string name=""):CUBase<MARG::size,MCTRL::size,MARG::opcode::size>(std::move(name)){
			CMS.wire(tbl.D.sub<MCTRL::alu::size>    (MCTRL::alu::low));
			 bs.wire(tbl.D.sub<MCTRL::io::Bs::size> (MCTRL::io::Bs::low));
			 rs.wire(tbl.D.sub<MCTRL::io::Rs::size> (MCTRL::io::Rs::low));
			dir.wire(tbl.D.sub<MCTRL::io::dir::size>(MCTRL::io::dir::low));
			rs_en.wire(tbl.D.sub<1>(MCTRL::io::dir::low));
		}
	};

	struct CPU:Circuit{
		Clock clk,clk_;
		Port<1> clr;

		Nand<1> nand{"[ClkNot]"};
		Memory<16,8,8> mem{8,1,"[Memory]"};
		CU cu{"[CU]"};
		ALU<8> alu{"[ALU]"};
		IOControl ioctl{"[IOctl]"};
		RegENSet<MCTRL::io::Rs::size,8> regset{"[RegSet]"};
		RAM<MCTRL::io::Bs::size,8> reg{"[RegFile]"};
		CPU(std::string name=""):Circuit(std::move(name)){
			add_comps(nand,mem,cu,alu,ioctl,regset,reg);

			clk.wire(nand.A,nand.B,cu.clk,regset.clk);
			clk_.wire(nand.Y,cu.clk_,reg.ce);
			clr.wire(cu.clr);
			alu.Co.wire(cu.Ci);
			regset.output[RegSet::I.v()].wire(cu.op);
			regset.output[RegSet::A.v()].wire(alu.A);
			regset.output[RegSet::L.v()].wire(mem.addr.sub<8>(0));
			regset.output[RegSet::H.v()].wire(mem.addr.sub<8>(8));
			ioctl.B.wire(alu.B);
			ioctl.F.wire(alu.O,regset.input);
			ioctl.R.wire(reg.D);
			ioctl.M.wire(mem.data);
			cu.CMS.wire(alu.CMS);
			cu.bs.wire(reg.A);
			cu.rs.wire(regset.sel);
			cu.rs_en.wire(regset.en);
			cu.dir.wire(ioctl.dir);
			ioctl.mem_oe.wire(mem.oe);
			ioctl.mem_we.wire(mem.we);
			ioctl.reg_oe.wire(reg.oe);
			ioctl.reg_we.wire(reg.we);

			auto tbl=BBCPU::OpCode::genOpTable();
			std::copy(tbl.begin(), tbl.end(), cu.tbl.data);
		}
	};
}
int main() {
	BBCPU::ASM::ops_t program;
	{
		using namespace BBCPU::ASM;
		UInt8 a{RegVar::make(Reg::A)},b{RegVar::make(Reg::B)};
		program=ASM::parse({
			imm(Reg::A,0),imm(Reg::B,3),
			While{b,{{
				a.set(add(a,b)),
				b.set(sub(b,1_u8)),
			}}},
			halt(),
		});
	}
	using namespace Circuit;
	CPU cpu{"[CPU]"};
	std::copy(program.begin(), program.end(), cpu.mem.rom.data);
	::Circuit::Circuit::ignoreReadFloating=true;
	cpu.clr.set(0);
	cpu.clk.set(0);
	cpu.update();
	cpu.clr.set(1);
std::cout<<std::endl;
	for(int i=0;i<500;i++){
		cpu.clk.clock();
		cpu.update();
		std::cout<<std::endl;
		std::cout<<"OP:"<<OpCode::Ops::all::parse(cpu.cu.op.get()).first<<std::endl;
		std::cout<<"MCTRL:"<<MCTRL::decode(cpu.cu.tbl.D.get(),cpu.mem.addr.get())<<std::endl;
		std::cout<<"INDEX:"<<MCTRL::state::index::get(cpu.cu.tbl.D.get())<<std::endl;
		for(size_t i=0;i<4;++i){
			std::cout<<"RS["<<RegSet(i).str()<<"]="<<cpu.regset.output[i].get()<<" ";
		}
		std::cout<<std::endl;
		for(size_t i=0;i<16;++i){
			std::cout<<"Reg["<<BBCPU::Reg(i).str()<<"]="<<cpu.reg.data[i];
			if(i%8==7){
				std::cout<<std::endl;
			}else{
				std::cout<<" ";
			}
		}
		std::cout<<std::endl;
		//std::cout<<cpu.alu<<std::endl;
	}
	/*Counter<8> cnt;
	for(int i=0;i<10;i++){
		cnt.update();
		std::cout<<cnt<<std::endl;
		cnt.clk.clock();
	}*/
	/*
	Circuit::CPU cpu(program);
	try {
		for(int i=0;i<10;i++){
			cpu.update();
			std::cout<<cpu.OP<<" "<<cpu.state<<" "<<cpu.A<<" "<<cpu.B<<" "<<cpu.F<<" ";
			std::cout<<std::endl;
			cpu.clk.data.flip();
		}
	} catch (const Circuit::Error& err) {
		std::cout<<err.what()<<std::endl;
	}*/
	/*auto [a,b,o]=sim.make_wires<8>(0,1,0);
	auto [clk,en]=sim.make_wires<1>(0,0);
	auto adder=sim.make_circuit<Adder>(a,b,o);
	auto reg=sim.make_circuit<Reg>(clk,en,o,a);*/
	/*using namespace Circuit;
	Simulation sim{};
	auto [adder,reg]=sim.make<Adder,Reg<8>>();
	auto clk=sim.constant<1>(reg->clk,0);
	auto en=sim.constant<1>(reg->en,0);
	auto a=sim.wire<8>(adder->A,reg->output);
	auto b=sim.constant<8>(adder->B,1);
	auto o=sim.wire<8>(adder->O,reg->input);

	//sim.update();
	try {
		for(int i=0;i<10;i++){
			sim.step();
			std::cout<<*clk<<std::endl;
			std::cout<<*reg<<std::endl;
			std::cout<<*adder<<std::endl;
			std::cout<<std::endl;
			clk->data.flip();
		}
	} catch (const Error& err) {
		std::cout<<err.what()<<std::endl;
	}*/

	//std::cout<<d<<std::endl;
	//std::cout<<d.get()<<std::endl;
	//using namespace BBCPU;


	//MCode mctx(0,0,0);
	//mctx.inc16(Reg16::PC);
	//std::cout<<std::bitset<16>(-1);
	//RegPair::generateRPROM();
	//BBCPU::OpCode::generateOPROM("oprom_regfile8x16.txt",32,0);
	//BBCPU::OpCode::generateOPROM("oprom_v3_0.txt",8,0);
	//BBCPU::OpCode::generateOPROM("oprom_v3_1.txt",8,1);
	//BBCPU::OpCode::generateOPROM("oprom_v3_2.txt",8,2);

	//ASM::generateASMROM();
	/*
	{
		using namespace BBCPU::ASM;
		UInt8 a{RegVar::make(Reg::A)},b{RegVar::make(Reg::B)};
		simulate("program.txt",ASM::parse({
			imm(Reg::A,0),imm(Reg::B,3),
			While{b,{{
				a.set(add(a,b)),
				b.set(sub(b,1_u8)),
			}}},
			halt(),
		}));
	}
	*/
	//ASM::testASM();
	/*{
		using namespace ALU74181;
		auto [c,f]=run<4>(Carry::no,Method::arithmetic,Arith::fn::AornotBplusAandB,0b0100u,0b1010u);
		std::cout<<std::bitset<4>(f)<<std::endl;
		std::cout<<std::boolalpha<<(c==Carry::yes)<<std::endl;
	}*/

	/*
	std::ofstream fout("a.txt");
	if(!fout) {return 1;}
	LogicGenerator::generateROM(fout,std::function{[](OpCode::StackBased::IN::type in){
		std::cout<<std::bitset<19>(in);
		auto out=OpCode::StackBased::opcode::run(in);
		std::cout<<"=>"<<std::bitset<32>(out)<<std::endl;
		return out;
	}});
	*/
	/*
	std::ofstream fout("b.txt");
	if(!fout) {return 1;}
	LogicGenerator::generateROM(fout,std::function{[](RegPair::IN::type in){
		RegPair::OUT::type out{0};
		std::cout<<std::bitset<9>(in);
		out=RegPair::OUT::run(out,in);
		std::cout<<"=>"<<std::bitset<8>(out)<<std::endl;
		return out;
	}});*/
	/*Util::bitset_wrap<5> a{3};
	Util::bitset_wrap<5> b{3};
	std::cout << (a==b) << std::endl;*/

	return 0;
}
