//
// Created by chasingfar on 2020/11/23.
//

#ifndef BBCPU_CPU_REGSET_SRAM_MCTRL_H
#define BBCPU_CPU_REGSET_SRAM_MCTRL_H
#include "marg.h"
namespace BBCPU::RegSet_SRAM {

	BITFILEDBASE(2) struct IODIR : Base {
		using is_write = BitField<1, Base, FollowMode::innerLow>;
		using is_mem  = BitField<1, is_write>;
	};
	enum struct DirMode : IODIR<>::type {
		Br = 0b00,
		Bw = 0b01,
		Mr = 0b10,
		Mw = 0b11,
	};
	BITFILEDBASE(8) struct IO : Base {
		using Bs  = BitField<4, Base,FollowMode::innerLow >;
		using Rs  = BitField<2, Bs>;
		using dir = IODIR<2, Rs >;
		static auto set(auto o,Reg bs,RegSet rs,DirMode dirMode){
			o=Bs::set(o,bs);
			o=Rs::set(o,rs);
			o=dir::set(o,dirMode);
			return o;
		}
		static auto setBr(auto o,Reg bs,RegSet rs){
			return set(o,bs,rs,DirMode::Br);
		}
		static auto setRs(auto o,RegSet rs){
			return set(o,Reg::OPR,rs,DirMode::Br);
		}
		static auto setBw(auto o,Reg bs){
			return set(o,bs,RegSet::A,DirMode::Bw);
		}
		static auto setMr(auto o,RegSet toReg){
			return set(o,Reg::OPR,toReg,DirMode::Mr);
		}
		static auto setMw(auto o){
			return set(o,Reg::OPR,RegSet::A,DirMode::Mw);
		}
		static std::tuple<std::string,std::string,std::string> decode(auto o,std::string addr=""){
			std::string L="RS["+RegSet::A.str()+"]",R,O,
				REG="Reg["+Reg(Bs::get(o)).str()+"]",
				RS="RS["+RegSet(Rs::get(o)).str()+"]",
				MEM="MEM["+addr+"]";
			switch (static_cast<DirMode>(dir::get(o))) {
				case DirMode::Br:
					R=REG;
					O=RS;
					break;
				case DirMode::Bw:
					R=RS;
					O=REG;
					break;
				case DirMode::Mr:
					R=MEM;
					O=RS;
					break;
				case DirMode::Mw:
					R=RS;
					O=MEM;
					break;
			}
			return {L,R,O};
		}
		static auto decode(auto o,size_t addr){
			return decode(o,std::to_string(addr));
		}
	};
	struct MCTRL : BitField<24,StartAt<0> > {
		using state  = STATE<9,StartAt<0> >;
		using INTA_  = BitField<1, state>;
		using alu    = ALU<6, INTA_>;
		using io     = IO<8, alu>;
		static auto noOp(auto o){
			o=alu::passA(o);
			o=io::setRs(o,RegSet::A);
			return o;
		}
		static auto setIndex(auto o, size_t index){
			o=state::index::set(o, index);
			return o;
		}
		static auto BtoReg(auto o, Reg from, RegSet to){
			LOG(from, to);
			o=alu::passB(o);
			o=io::setBr(o, from, to);
			return o;
		}
		static auto AtoB(auto o, Reg to){
			LOG(from, to);
			o=alu::passA(o);
			o=io::setBw(o, to);
			return o;
		}
		static auto MtoReg(auto o, RegSet to){
			LOG(from, to);
			o=alu::passB(o);
			o=io::setMr(o,to);
			return o;
		}
		static auto AtoM(auto o){
			LOG(from, to);
			o=alu::passA(o);
			o=io::setMw(o);
			return o;
		}

		template <auto fn>
		static auto calc(auto o,Reg rhs, auto carry){
			LOG(rhs,carry);
			o=fn(o,carry);
			o=io::setBr(o, rhs, RegSet::A);
			return o;
		}
		template <auto fn>
		static auto calc(auto o,Reg rhs){
			LOG(rhs);
			o=fn(o);
			o=io::setBr(o, rhs, RegSet::A);
			return o;
		}


		template <auto fn>
		static auto logic(auto o,Reg rhs){
			LOG(rhs);
			o=fn(o);
			o=io::setBr(o, rhs, RegSet::A);
			return o;
		}

		static auto zero(auto o,Reg reg){
			o=alu::zero(o);
			o=io::setBw(o,reg);
			return o;
		}
		static auto zero(auto o,RegSet reg){
			o=alu::zero(o);
			o=io::setBr(o, Reg::OPR, reg);
			return o;
		}
		static auto minusOne(auto o,Reg reg){
			o=alu::minusOne(o);
			o=io::setBw(o,reg);
			return o;
		}

		static auto inc(auto o){
			LOG(from,to);
			o=alu::inc(o);
			o=io::setRs(o,RegSet::A);
			return o;
		}

		static auto inc(auto o, alu::Carry carry){
			LOG(from,to,carry);
			o=alu::inc(o,carry);
			o=io::setRs(o,RegSet::A);
			return o;
		}

		static auto dec(auto o){
			LOG(from,to);
			o=alu::dec(o);
			o=io::setRs(o,RegSet::A);
			return o;
		}

		static auto dec(auto o, alu::Carry carry){
			LOG(from,to,carry);
			o=alu::dec(o,carry);
			o=io::setRs(o,RegSet::A);
			return o;
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
	static auto saveTCF(auto marg,auto mctrl){
		return MCTRL::state::TCF::set(mctrl, MARG::carry::get(marg));
	}
}
#endif //BBCPU_CPU_REGSET_SRAM_MCTRL_H
