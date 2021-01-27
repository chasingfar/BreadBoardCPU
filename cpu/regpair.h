//
// Created by user on 2020/10/31.
//

#ifndef BREADBOARDCPU_REGPAIR_H
#define BREADBOARDCPU_REGPAIR_H

#include "mctrl.h"

namespace BreadBoardCPU::RegPair {

	BITFILEDBASE(2) struct REGSEL : Base {
		using reg_sel_ = BitField<1, Base, FollowMode::innerLow>;
		using reg_H    = BitField<1, reg_sel_>;

		static bool isM(auto o) {
			return !reg_sel_::get(o);
		}

		static bool isH(auto o) {
			return isM(o) && reg_H::get(o);
		}

		static bool isL(auto o) {
			return isM(o) && (!reg_H::get(o));
		}
	};

	struct RPARG : BitField<9, StartAt<0> > {
		using lhs     = REGSEL<2, StartAt<0> >;
		using rhs     = REGSEL<2, lhs>;
		using res     = REGSEL<2, rhs>;
		using mem     = IODIR<2, res>;
		using CLK     = BitField<1, mem>;
	};
	BITFILEDBASE(4) struct REGCTRL : Base {
		using clk      = BitField<1, Base, FollowMode::innerLow>;
		using lhs_     = BitField<1, clk>;
		using rhs_     = BitField<1, lhs_>;
		using mem_     = BitField<1, rhs_>;
	};

	struct RPCTRL : BitField<8, StartAt<0> > {
		using reg_H  = REGCTRL<4, StartAt<0> >;
		using reg_L  = REGCTRL<4, reg_H>;

		static auto run(auto rpctrl, auto rparg) {

			rpctrl = reg_L::rhs_::set(rpctrl, !RPARG::rhs::isL(rparg));
			rpctrl = reg_H::rhs_::set(rpctrl, !RPARG::rhs::isH(rparg));
			switch ((DirMode) RPARG::mem::get(rparg)) {
				case DirMode::MemToReg:
					rpctrl = reg_L::mem_::set(rpctrl, !RPARG::lhs::isM(rparg));
					rpctrl = reg_H::mem_::set(rpctrl, !RPARG::lhs::isM(rparg));

					rpctrl = reg_L::clk::set(rpctrl, RPARG::res::isL(rparg) && RPARG::CLK::get(rparg));
					rpctrl = reg_H::clk::set(rpctrl, RPARG::res::isH(rparg) && RPARG::CLK::get(rparg));
					rpctrl = reg_L::lhs_::set(rpctrl, 1);
					rpctrl = reg_H::lhs_::set(rpctrl, 1);
					break;
				case DirMode::RegToMem:
					rpctrl = reg_L::lhs_::set(rpctrl, !RPARG::lhs::isL(rparg));
					rpctrl = reg_H::lhs_::set(rpctrl, !RPARG::lhs::isH(rparg));

					rpctrl = reg_L::mem_::set(rpctrl, !RPARG::res::isM(rparg));
					rpctrl = reg_H::mem_::set(rpctrl, !RPARG::res::isM(rparg));
					break;
				case DirMode::RegToReg:
				case DirMode::MemToMem:
					rpctrl = reg_L::lhs_::set(rpctrl, !RPARG::lhs::isL(rparg));
					rpctrl = reg_H::lhs_::set(rpctrl, !RPARG::lhs::isH(rparg));
					rpctrl = reg_L::clk::set(rpctrl, RPARG::res::isL(rparg) && RPARG::CLK::get(rparg));
					rpctrl = reg_H::clk::set(rpctrl, RPARG::res::isH(rparg) && RPARG::CLK::get(rparg));
					rpctrl = reg_L::mem_::set(rpctrl, 1);
					rpctrl = reg_H::mem_::set(rpctrl, 1);
					break;
			}
			return rpctrl;
		}
	};
	void generateRPROM(){
		std::ofstream fout("rprom.txt");
		if(!fout) {return;}
		fout<<ROM(TruthTable<MARG::type,MCTRL::type,MARG::size>(std::function{[](RPARG::type in){
			RPCTRL::type out{0};
			std::cout<<std::bitset<9>(in);
			out=RPCTRL::run(out, in);
			std::cout<<"=>"<<std::bitset<8>(out)<<std::endl;
			return out;
		}}));
	}
}
#endif //BREADBOARDCPU_REGPAIR_H
