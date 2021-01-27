//
// Created by chasingfar on 2020/11/23.
//

#ifndef BREADBOARDCPU_MARG_H
#define BREADBOARDCPU_MARG_H
#include "common.h"
namespace BreadBoardCPU {
	BITFILEDBASE(9) struct STATE : Base {
		using index  = BitField<Size-2,Base,FollowMode::innerLow>;
		using CF     = BitField<1,index>;
		using IF    = BitField<1,CF>;
	};

	struct MARG : BitField<19,StartAt<0> > {
		using carry  = BitField<1, StartAt<0> >;
		using state  = STATE<9, carry>;
		using INT    = BitField<1, state >;
		using opcode = BitField<8, INT >;
		static ALU74181::Carry getCarry(auto o){
			return static_cast<ALU74181::Carry>(carry::get(o));
		}
		static ALU74181::Carry getCF(auto o){
			return static_cast<ALU74181::Carry>(state::CF::get(o));
		}
		static bool isINT(auto o){
			return INT::get(o)==1;
		}
		static size_t getIndex(auto o){
			return static_cast<size_t>(state::index::get(o));
		}
	};
	namespace Regs{
		enum struct Reg {
			OPR,TMA,
			TML,TMH,
			SPL,SPH,
			PCL,PCH,
			A,B,
			C,D,
			E,F,
			L,H,
		};
		enum struct UReg {
			A,B,
			C,D,
			E,F,
			L,H,
		};
		enum struct Reg16 {
			IMM,
			TMP,
			SP,
			PC,
			BA,
			DC,
			FE,
			HL,
		};
		static Reg user(UReg reg){
			return static_cast<Reg>(static_cast<unsigned>(reg) + static_cast<unsigned>(Reg::A));
		}
		static Reg pair(Reg16 reg16){
			return static_cast<Reg>(static_cast<unsigned>(reg16) << 1u );
		}
		static Reg toL(Reg16 reg16){
			return static_cast<Reg>( (static_cast<unsigned>(reg16) << 1u) + 0);
		}
		static Reg toH(Reg16 reg16){
			return static_cast<Reg>( (static_cast<unsigned>(reg16) << 1u) + 1);
		}
		std::ostream &operator<<(std::ostream &os, Reg reg) {
			return os<<static_cast<unsigned>(reg);
		}
		std::ostream &operator<<(std::ostream &os, UReg reg) {
			return os<<static_cast<unsigned>(reg);
		}
		std::ostream &operator<<(std::ostream &os, Reg16 reg) {
			return os<<static_cast<unsigned>(reg);
		}
	}
	using namespace Regs;
}
#endif //BREADBOARDCPU_MARG_H
