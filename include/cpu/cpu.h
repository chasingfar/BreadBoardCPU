//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_CPU_CPU_H
#define BBCPU_CPU_CPU_H
#include "opcode.h"
namespace BBCPU{
	struct CPU{
		using Reg = Regs::Reg;
		using Reg16 = Regs::Reg16;
		using op_t = uint8_t;
		using addr_t = uint16_t;

		op_t REG[16]{};
		op_t REGSET[4]{};
		op_t RAM[1u<<16u]{};
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
		void load(auto data,addr_t addr=0){
			for (auto op:data){
				RAM[addr]=op;
				addr++;
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
			size_t sp=get_addr((size_t)pair(Reg16::SP))+1;
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
		std::string get_regset_str(size_t i){
			return "REGSET["+std::to_string(i)+"]("+std::to_string(REGSET[i])+")";
		}
		std::string get_ram_str(size_t i){
			return "RAM["+std::to_string(i)+"]("+std::to_string(RAM[i])+")";
		}
		addr_t get_addr(auto v){
			return (static_cast<addr_t>(REG[v+1])<<8u)|REG[v];
		}
		addr_t get_pair(Reg16 reg16){
			return get_addr(pair(reg16).v());
		}
		op_t read_pair(Reg16 reg16, int16_t offset= 0){
			return RAM[get_pair(reg16)+offset];
		}
		void load_op(auto op){
			load(op, get_pair(Reg16::PC));
			REGSET[RegSet::I.v()]=op[0];
			marg=MARG::opcode::set(marg,op[0]);
		}
		bool isHalt(){
			return MARG::opcode::get(marg)==OpCode::Ops::Halt::id::id;
		}
		void tick(bool debug=false){
			if(debug){std::cout<<std::bitset<19>(marg)<<std::endl;}
			mctrl=OpCode::Ops::all::gen(marg);
			if(debug){std::cout<<std::endl;}
			auto dir=static_cast<DirMode>(MCTRL::io::dir::get(mctrl));
			op_t A=0,B=0,F=0;
			std::string A_str="",B_str="",F_str="",fn_str="";
			std::tie(A,A_str)=std::pair{REGSET[RegSet::A.v()],get_regset_str(RegSet::A.v())};
			auto addr=(static_cast<addr_t>(REGSET[RegSet::H.v()])<<8)|REGSET[RegSet::L.v()];
			auto Bs=MCTRL::io::Bs::get(mctrl);
			auto Rs=MCTRL::io::Rs::get(mctrl);

			if (dir==DirMode::Br){
				std::tie(B,B_str)=std::pair{REG[Bs],get_reg_str(Bs)};
			}else if(dir==DirMode::Mr){
				std::tie(B,B_str)=std::pair{RAM[addr],get_ram_str(addr)};
			}

			auto [carry,O]=MCTRL::alu::run<8>(mctrl,A,B);
			fn_str=MCTRL::alu::get_fn_str(mctrl,A_str,B_str)+"="+std::to_string(O);
			switch (dir) {
				case DirMode::Br:
				case DirMode::Mr:
					F_str=get_reg_str(Rs);
					REGSET[Rs]=O;
					break;
				case DirMode::Bw:
					F_str=get_reg_str(Bs);
					REG[Bs]=O;
					break;
				case DirMode::Mw:
					F_str=get_ram_str(addr);
					RAM[addr]=O;
					break;
			}

			if(debug){
				std::cout<<F_str<<"="<<fn_str<<" carry="<<std::boolalpha<<(carry==MCTRL::alu::Carry::yes)<<std::endl;
			}

			marg=MARG::carry::set(0,static_cast<MARG::type>(carry));
			marg=MARG::state::set(marg,MCTRL::state::get(mctrl));
			marg=MARG::opcode::set(marg,REGSET[RegSet::I.v()]);
		}
		void tick_op(bool debug=false){
			do{
				tick(debug);
			}while(MARG::getIndex(marg)!=0);
			if(debug){
				std::cout<<std::endl;
			}
		}
	};
}
#endif //BBCPU_CPU_H
