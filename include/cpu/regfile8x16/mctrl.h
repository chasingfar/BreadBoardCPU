//
// Created by chasingfar on 2020/11/23.
//

#ifndef BBCPU_CPU_REGFILE8X16_MCTRL_H
#define BBCPU_CPU_REGFILE8X16_MCTRL_H
#include "marg.h"
namespace BBCPU::RegFile8x16 {

	BITFILEDBASE(2) struct IODIR : Base {
		using mem_write_ = BitField<1, Base, FollowMode::innerLow>;
		using mem_read_  = BitField<1, mem_write_>;
	};
	enum struct DirMode : IODIR<>::type {
		MemToMem = 0b00,/*as RegToReg*/
		MemToReg = 0b01,
		RegToMem = 0b10,
		RegToReg = 0b11,
	};
	BITFILEDBASE(14) struct IO : Base {
		using to     = BitField<4, Base,FollowMode::innerLow >;
		using fromA  = BitField<4, to >;
		using fromB  = BitField<4, fromA >;
		using dir    = IODIR<2, fromB >;

		static auto regToReg(auto o, Reg dest, Reg lhs, Reg rhs = Reg::OPR){
			LOG(dest, lhs, rhs);
			o=to::set(o,dest);
			o=fromA::set(o,lhs);
			o=fromB::set(o,rhs);
			o=dir::set(o, DirMode::RegToReg);
			return o;
		}
		static auto memToReg(auto o, Reg to, Reg16 from){
			LOG(from, to);
			o=to::set(o,to);
			o=fromA::set(o,from.L());
			o=fromB::set(o,from.H());
			o=dir::set(o, DirMode::MemToReg);
			return o;
		}
		static auto regToMem(auto o, Reg16 to, Reg from){
			LOG(from, to);
			o=to::set(o,to.L());
			o=fromA::set(o,from);
			o=fromB::set(o,to.H());
			o=dir::set(o, DirMode::RegToMem);
			return o;
		}

		static std::tuple<std::string,std::string,std::string> decode(auto o,std::string addr=""){
			std::string 
				O="Reg["+Reg(   to::get(o)).str()+"]",
				A="Reg["+Reg(fromA::get(o)).str()+"]",
				B="Reg["+Reg(fromB::get(o)).str()+"]",
				MEM="MEM["+addr+"]";
			switch (static_cast<DirMode>(dir::get(o))) {
				case DirMode::MemToMem:
				case DirMode::RegToReg:
					return {A,B,O};
				case DirMode::MemToReg:
					return {MEM+"("+B+"|"+A+")","",O};
				case DirMode::RegToMem:
					return {A,"",MEM+"("+B+"|"+O+")"};
			}
		}
		static auto decode(auto o,size_t addr){
			return decode(o,std::to_string(addr));
		}
	};
	BITFILEDBASE(3) struct SIGNAL : Base {
		using HALT   = BitField<1, Base,FollowMode::innerLow >;
		using SIG    = BitField<1, HALT>;
		static auto  halt(auto o){
			o=HALT::set(o,1);
			return o;
		}
		static bool isHalt(auto o){
			return HALT::get(o)==1;
		}
	};
	struct MCTRL : BitField<32,StartAt<0> > {
		using state  = STATE<9,StartAt<0> >;
		using INTA_  = BitField<1, state>;
		using alu    = ALU<6, INTA_>;
		using io     = IO<14, alu>;
		using sig    = SIGNAL<2, io>;
		static auto noOp(auto o){
			o=copy(o, Reg::OPR,Reg::OPR);
			return o;
		}
		static auto setIndex(auto o, size_t index){
			o=state::index::set(o, index);
			return o;
		}
		static auto copy(auto o, Reg from, Reg to){
			LOG(from, to);
			o=alu::passA(o);
			o=io::regToReg(o, to, from);
			return o;
		}
		static auto load(auto o, Reg16 from, Reg to){
			LOG(from, to);
			o=alu::passA(o);
			o=io::memToReg(o, to, from);
			return o;
		}
		static auto save(auto o, Reg from, Reg16 to){
			LOG(from, to);
			o=alu::passA(o);
			o=io::regToMem(o, to, from);
			return o;
		}

		template <auto fn>
		static auto calc(auto o,Reg lhs,Reg rhs,Reg dest, auto carry){
			LOG(lhs,rhs,dest,carry);
			o=fn(o,carry);
			o=io::regToReg(o,dest,lhs,rhs);
			return o;
		}
		template <auto fn>
		static auto calc(auto o,Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			o=fn(o);
			o=io::regToReg(o,dest,lhs,rhs);
			return o;
		}


		template <auto fn>
		static auto logic(auto o,Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,dest);
			o=fn(o);
			o=io::regToReg(o,dest,lhs,rhs);
			return o;
		}

		static auto zero(auto o,Reg reg){
			o=alu::zero(o);
			o=io::regToReg(o,reg,reg);
			return o;
		}
		static auto minusOne(auto o,Reg reg){
			o=alu::minusOne(o);
			o=io::regToReg(o,reg,reg);
			return o;
		}

		static auto inc(auto o, Reg from, Reg to){
			LOG(from,to);
			o=alu::inc(o);
			o=io::regToReg(o,to,from);
			return o;
		}
		static auto inc(auto o, Reg reg){
			return inc(o, reg, reg);
		}

		static auto inc(auto o, Reg from, Reg to, alu::Carry carry){
			LOG(from,to,carry);
			o=alu::inc(o,carry);
			o=io::regToReg(o,to,from);
			return o;
		}
		static auto inc(auto o, Reg reg, alu::Carry carry){
			return inc(o, reg, reg, carry);
		}

		static auto dec(auto o, Reg from, Reg to){
			LOG(from,to);
			o=alu::dec(o);
			o=io::regToReg(o,to,from);
			return o;
		}
		static auto dec(auto o, Reg reg){
			return dec(o, reg, reg);
		}

		static auto dec(auto o, Reg from, Reg to, alu::Carry carry){
			LOG(from,to,carry);
			o=alu::dec(o,carry);
			o=io::regToReg(o,to,from);
			return o;
		}
		static auto dec(auto o, Reg reg, alu::Carry carry){
			return dec(o, reg, reg, carry);
		}

		static std::string decode(auto o,std::string addr){
			auto [L,R,O]=MCTRL::io::decode(o,addr);
			auto fn_str=MCTRL::alu::get_fn_str(o, L, R);
			return O+"="+fn_str;
		}
		static auto decode(auto o,size_t addr){
			return decode(o,std::to_string(addr));
		}
	};
	static auto incIndex(auto marg,auto mctrl){
		return MCTRL::setIndex(mctrl, MARG::getIndex(marg) + 1);
	}
}
#endif //BBCPU_CPU_REGFILE8X16_MCTRL_H
