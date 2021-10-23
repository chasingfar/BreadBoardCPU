//
// Created by chasingfar on 2020/11/23.
//

#ifndef BBCPU_CPU_MCTRL_H
#define BBCPU_CPU_MCTRL_H
#include "marg.h"
namespace BBCPU {

	BITFILEDBASE(2) struct IODIR : Base {
		using mem_write = BitField<1, Base, FollowMode::innerLow>;
		using mem_read  = BitField<1, mem_write>;
	};
	enum struct DirMode : IODIR<>::type {
		RegToReg = 0b00,
		RegToMem = 0b01,
		MemToReg = 0b10,
		MemToMem = 0b11,/*as RegToReg*/
	};
	BITFILEDBASE(14) struct IO : Base {
		using fromB  = BitField<4, Base,FollowMode::innerLow >;
		using fromA  = BitField<4, fromB >;
		using to     = BitField<4, fromA >;
		using dir    = IODIR<2, to >;
		static auto set(auto o,Reg lhs,Reg rhs,Reg dest,DirMode dirMode){
			o=fromA::set(o,lhs);
			o=fromB::set(o,rhs);
			o=to::set(o,dest);
			o=dir::set(o,dirMode);
			return o;
		}
		static auto set2(auto o,Reg lhs,Reg dest,DirMode dirMode){
			o=fromA::set(o,lhs);
			o=fromB::set(o,0);
			o=to::set(o,dest);
			o=dir::set(o,dirMode);
			return o;
		}
	};
	BITFILEDBASE(3) struct SIGNAL : Base {
		using INTA_  = BitField<1, Base,FollowMode::innerLow>;
		using HALT   = BitField<1, INTA_>;
		using SIG    = BitField<1, HALT>;
		static auto  halt(auto o){
			o=HALT::set(o,0);
			return o;
		}
		static bool isHalt(auto o){
			return HALT::get(o)==0;
		}
	};
	struct MCTRL : BitField<32,StartAt<0> > {
		using state  = STATE<9,StartAt<0> >;
		using sig    = SIGNAL<3, state>;
		using io     = IO<14, sig>;
		using alu    = ALU<6, io>;
		static auto noOp(auto o){
			o=regToReg(o, Reg::OPR,Reg::OPR);
			return o;
		}
		static auto setIndex(auto o, size_t index){
			o=state::index::set(o, index);
			return o;
		}
		static auto regToReg(auto o, Reg from, Reg to){
			LOG(from, to);
			o=alu::pass(o);
			o=io::set2(o, from, to, DirMode::RegToReg);
			return o;
		}
		static auto memToReg(auto o, Reg16 from, Reg to){
			LOG(from, to);
			o=alu::pass(o);
			o=io::set2(o, pair(from), to, DirMode::MemToReg);
			return o;
		}
		static auto regToMem(auto o, Reg from, Reg16 to){
			LOG(from, to);
			o=alu::pass(o);
			o=io::set2(o, from, pair(to), DirMode::RegToMem);
			return o;
		}

		template <auto fn>
		static auto calc(auto o,Reg lhs,Reg rhs,Reg dest, auto carry){
			LOG(lhs,rhs,dest,carry);
			o=fn(o,carry);
			o=io::set(o,lhs,rhs,dest,DirMode::RegToReg);
			return o;
		}
		template <auto fn>
		static auto calc(auto o,Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			o=fn(o);
			o=io::set(o,lhs,rhs,dest,DirMode::RegToReg);
			return o;
		}


		template <auto fn>
		static auto logic(auto o,Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,dest);
			o=fn(o);
			o=io::set(o,lhs,rhs,dest,DirMode::RegToReg);
			return o;
		}

		static auto inc(auto o, Reg from, Reg to){
			LOG(from,to);
			o=alu::inc(o);
			o=io::set2(o,from,to,DirMode::RegToReg);
			return o;
		}
		static auto inc(auto o, Reg reg){
			return inc(o, reg, reg);
		}

		static auto inc(auto o, Reg from, Reg to, alu::Carry carry){
			LOG(from,to,carry);
			o=alu::inc(o,carry);
			o=io::set2(o,from,to,DirMode::RegToReg);
			return o;
		}
		static auto inc(auto o, Reg reg, alu::Carry carry){
			return inc(o, reg, reg, carry);
		}

		static auto dec(auto o, Reg from, Reg to){
			LOG(from,to);
			o=alu::dec(o);
			o=io::set2(o,from,to,DirMode::RegToReg);
			return o;
		}
		static auto dec(auto o, Reg reg){
			return dec(o, reg, reg);
		}

		static auto dec(auto o, Reg from, Reg to, alu::Carry carry){
			LOG(from,to,carry);
			o=alu::dec(o,carry);
			o=io::set2(o,from,to,DirMode::RegToReg);
			return o;
		}
		static auto dec(auto o, Reg reg, alu::Carry carry){
			return dec(o, reg, reg, carry);
		}
	};
	static auto incIndex(auto marg,auto mctrl){
		return MCTRL::setIndex(mctrl, MARG::getIndex(marg) + 1);
	}
}
#endif //BBCPU_CPU_MCTRL_H
