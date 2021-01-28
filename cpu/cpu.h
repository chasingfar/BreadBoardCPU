//
// Created by chasingfar on 2021/1/23.
//

#ifndef BREADBOARDCPU_CPU_H
#define BREADBOARDCPU_CPU_H

#include "opcode.h"
namespace BreadBoardCPU{
	struct CPU{
		uint8_t REG[16]{};
		uint8_t RAM[1u<<16u]{};
		MARG::type marg=0;
		MCTRL::type mctrl=0;
		void clear(){
			for (auto& reg:REG){
				reg=0;
			}
			for (auto& ram:RAM){
				ram=0;
			}
			marg=0;
		}
		void load(auto data){
			uint16_t i=0;
			for (auto op:data){
				RAM[i]=op;
				i++;
			}
		}
		void print_reg(){
			for (int i = 0; i < 16; ++i) {
				std::cout<<std::to_string(REG[i]);
				if(i%8==8-1)std::cout<<std::endl;
				else if(i%4==4-1)std::cout<<"|";
				else if(i%2==2-1)std::cout<<",";
				else std::cout<<" ";
			}
		}
		void print_stack(){
			size_t addr_max=1u<<16u;
			size_t sp=get_addr((size_t)Reg(Reg16::SP))+1;
			size_t i;
			for (i=sp; i < std::min(sp+10,addr_max); ++i) {
				std::cout<<std::to_string(RAM[i])<<" ";
			}
			if(i<addr_max){
				std::cout<<"...";
			}
			std::cout<<std::endl;
		}
		std::string get_reg_str(size_t i){
			return "REG["+std::to_string(i)+"]("+std::to_string(REG[i])+")";
		}
		std::string get_ram_str(size_t i){
			return "RAM["+std::to_string(i)+"]("+std::to_string(RAM[i])+")";
		}
		uint16_t get_addr(auto v){
			return (((uint16_t)REG[v+1])<<8u)|((uint16_t)REG[v]);
		}
		bool isHalt(){
			return MCTRL::sig::isHalt(mctrl);
		}
		void tick(bool debug=false){
			std::string A_str,B_str,fn_str;
			if(debug){std::cout<<std::bitset<19>(marg)<<std::endl;}
			mctrl=OpCode::Ops::all::gen(marg);
			if(debug){std::cout<<std::endl;}
			auto dir=static_cast<DirMode>(MCTRL::io::dir::get(mctrl));

			uint8_t A;
			if(dir==DirMode::MemToReg){
				A=RAM[get_addr(MCTRL::io::fromA::get(mctrl))];
				A_str=get_ram_str(get_addr(MCTRL::io::fromA::get(mctrl)));
			}else{
				A=REG[MCTRL::io::fromA::get(mctrl)];
				A_str=get_reg_str(MCTRL::io::fromA::get(mctrl));
			}

			uint8_t B=REG[MCTRL::io::fromB::get(mctrl)];
			B_str=get_reg_str(MCTRL::io::fromB::get(mctrl));

			auto [carry,O]=MCTRL::alu::run<8>(mctrl,A,B);
			fn_str=MCTRL::alu::get_fn_str(mctrl,A_str,B_str)+"="+std::to_string(O);

			if(dir==DirMode::RegToMem){
				fn_str=get_ram_str(get_addr(MCTRL::io::to::get(mctrl)))+"="+fn_str;
				RAM[get_addr(MCTRL::io::to::get(mctrl))]=O;
			}else{
				fn_str=get_reg_str(MCTRL::io::to::get(mctrl))+"="+fn_str;
				REG[MCTRL::io::to::get(mctrl)]=O;
			}
			if(debug){
				std::cout<<fn_str<<" carry="<<std::boolalpha<<(carry==MCTRL::alu::Carry::yes)<<std::endl;
			}

			marg=MARG::carry::set(0,static_cast<MARG::type>(carry));
			marg=MARG::state::set(marg,MCTRL::state::get(mctrl));
			marg=MARG::opcode::set(marg,REG[0]);
		}
		void tick_op(bool debug=false){
			do{
				tick(debug);
			}while(MARG::getIndex(marg)!=0);
		}
	};
}
#endif //BREADBOARDCPU_CPU_H
