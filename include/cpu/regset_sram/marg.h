//
// Created by chasingfar on 2020/11/23.
//

#ifndef BBCPU_CPU_REGSET_SRAM_MARG_H
#define BBCPU_CPU_REGSET_SRAM_MARG_H

#include "regs.h"
#include "../alu.h"
namespace BBCPU::RegSet_SRAM {
	BITFILEDBASE(9) struct STATE : Base {
		using index  = BitField<Size-3,Base,FollowMode::innerLow>;
		using TCF     = BitField<1,index>;
		using CF     = BitField<1,TCF>;
		using IF    = BitField<1,CF>;
	};

	BITFILEDBASE(19) struct MARG_ : Base {
		using carry  = BitField<1, Base,FollowMode::innerLow>;
		using state  = STATE<9, carry>;
		using INT    = BitField<1, state >;
		using opcode = BitField<8, INT >;
		static ALU74181::Carry getCarry(auto o){
			return static_cast<ALU74181::Carry>(carry::get(o));
		}
		static ALU74181::Carry getTCF(auto o){
			return static_cast<ALU74181::Carry>(state::TCF::get(o));
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
	using MARG=MARG_<>;
}
#endif //BBCPU_CPU_REGSET_SRAM_MARG_H
