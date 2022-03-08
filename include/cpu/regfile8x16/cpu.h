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
		using src_t = std::pair<op_t,std::string>;

		op_t REG[16]{};
		op_t RAM[1u<<16u]{};
		bool CLK{false};
		MARG::type marg{0};
		MCTRL::type mctrl{0};
		bool MW_{false},MR_{false},C{false},C_reg{false};
		size_t FS,LS,RS;
		src_t L{},R{},AH{},AL{},F{};

		void clear(){
			for (auto& reg:REG){
				reg=0;
			}
			for (auto& ram:RAM){
				ram=0;
			}
			marg=0;
		}
		std::string get_reg_str(size_t i){
			return "REG["+std::to_string(i)+"]("+std::to_string(REG[i])+")";
		}
		std::string get_ram_str(size_t i){
			return "RAM["+std::to_string(i)+"]("+std::to_string(RAM[i])+")";
		}
		src_t get_reg(size_t s){
			return {REG[s],get_reg_str(s)};
		}
		src_t get_ram(size_t addr){
			return {RAM[addr],get_ram_str(addr)};
		}
		addr_t get_addr(){
			return (static_cast<addr_t>(AH.first)<<8u)|AL.first;
		}
		void do_marg(){
			mctrl=OpCode::Ops::all::gen(marg);
			do_mctrl();
		}
		void do_mctrl(){
			FS=MCTRL::io::to::get(mctrl);
			LS=MCTRL::io::fromA::get(mctrl);
			RS=MCTRL::io::fromB::get(mctrl);

			MW_=static_cast<bool>(MCTRL::io::dir::mem_write_::get(mctrl));
			MR_=static_cast<bool>(MCTRL::io::dir::mem_read_::get(mctrl));
			do_regfile();
			do_ram_bus();
			do_alu();
		}
		void do_regfile(){
			auto X=get_reg(FS);
			auto Y=get_reg(LS);
			auto Z=get_reg(RS);

			if(!MW_ == false){L=Y;}
			R=Z;
			if(MW_ == false){AL=X;}
			if(MR_ == false){AL=Y;}
			if(MW_ && MR_ == false){AH=Z;}
		}
		void do_ram_bus(){
			if(MR_ == false){L=get_ram(get_addr());}
			if(MW_ == false){RAM[get_addr()]=F.first;}
		}
		void do_alu(){
			auto [carry,O]=MCTRL::alu::run<8>(mctrl,L.first,R.first);
			F={O,MCTRL::alu::get_fn_str(mctrl,L.second,R.second)+"="+std::to_string(O)};
			C=(static_cast<MARG::type>(carry)==1);
			do_ram_bus();
		}
		void clk_up(){
			C_reg=C;
			if(CLK && MW_ == true){REG[FS]=F.first;}
		}
		void clk_down(){
			marg=MARG::carry::set(0,static_cast<MARG::type>(C_reg));
			marg=MARG::state::set(marg,MCTRL::state::get(mctrl));
			marg=MARG::opcode::set(marg,REG[0]);
			do_marg();
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
			size_t sp=get_addr(Reg::SPL.v(),Reg::SPH.v())+1;
			size_t i;
			for (i=sp; i < std::min(sp+10,addr_max); ++i) {
				std::cout<<std::to_string(RAM[i])<<" ";
			}
			if(i<addr_max){
				std::cout<<"...";
			}
			std::cout<<std::endl;
		}
		addr_t get_addr(auto a,auto b){
			return (static_cast<addr_t>(REG[b])<<8u)|REG[a];
		}
		addr_t get_pair(Reg16 reg16){
			return get_addr(reg16.L().v(),reg16.H().v());
		}
		op_t read_pair(Reg16 reg16, int16_t offset= 0){
			return RAM[get_pair(reg16)+offset];
		}
		void load_op(auto op){
			load(op, get_pair(Reg16::PC));
			REG[Reg::OPR.v()]=op[0];
			marg=MARG::opcode::set(marg,op[0]);
		}
		bool isHalt(){
			return MARG::opcode::get(marg)==OpCode::Ops::Halt::id::id;
		}
		void tick(bool debug=false){
			std::string A_str,B_str,fn_str;
			if(debug){std::cout<<std::bitset<19>(marg)<<std::endl;}
			mctrl=OpCode::Ops::all::gen(marg);
			if(debug){std::cout<<std::endl;}
			auto dir=static_cast<DirMode>(MCTRL::io::dir::get(mctrl));

			op_t A;
			if(dir==DirMode::MemToReg){
				A=RAM[get_addr(MCTRL::io::fromA::get(mctrl),MCTRL::io::fromB::get(mctrl))];
				A_str=get_ram_str(get_addr(MCTRL::io::fromA::get(mctrl),MCTRL::io::fromB::get(mctrl)));
			}else if(dir==DirMode::RegToMem){
				A=REG[Reg::TMA.v()];
				A_str=get_reg_str(Reg::TMA.v());
			}else{
				A=REG[MCTRL::io::fromA::get(mctrl)];
				A_str=get_reg_str(MCTRL::io::fromA::get(mctrl));
			}

			op_t B=REG[MCTRL::io::fromB::get(mctrl)];
			B_str=get_reg_str(MCTRL::io::fromB::get(mctrl));

			auto [carry,O]=MCTRL::alu::run<8>(mctrl,A,B);
			fn_str=MCTRL::alu::get_fn_str(mctrl,A_str,B_str)+"="+std::to_string(O);

			if(dir==DirMode::RegToMem){
				fn_str=get_ram_str(get_addr(MCTRL::io::fromA::get(mctrl),MCTRL::io::fromB::get(mctrl)))+"="+fn_str;
				RAM[get_addr(MCTRL::io::fromA::get(mctrl),MCTRL::io::fromB::get(mctrl))]=O;
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
			if(debug){
				std::cout<<std::endl;
			}
		}
	};
}
#endif //BBCPU_CPU_H
