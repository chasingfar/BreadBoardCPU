//
// Created by chasingfar on 2020/11/23.
//

#ifndef BREADBOARDCPU_MARG_H
#define BREADBOARDCPU_MARG_H
#include "regs.h"
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
}
#endif //BREADBOARDCPU_MARG_H
