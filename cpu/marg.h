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
		struct Reg {
			using base_t=unsigned;
			#define TABLE \
				X(OPR) X(TMA) \
				X(TML) X(TMH) \
				X(SPL) X(SPH) \
				X(PCL) X(PCH) \
				X(A  ) X(B  ) \
				X(C  ) X(D  ) \
				X(E  ) X(F  ) \
				X(L  ) X(H  ) \

			enum Value: base_t{
				#define X(a) a,
					TABLE
				#undef X
			};
			inline static const std::string str[]={
				#define X(a) #a,
					TABLE
				#undef X
			};
			#undef TABLE
			Value v;
			Reg() = default;
			constexpr Reg(Value v) : v(v) { }
			explicit constexpr Reg(base_t v) : v(static_cast<Value>(v)) { }
			//constexpr bool operator==(Reg a) const { return v == a.v; }
			//constexpr bool operator!=(Reg a) const { return v != a.v; }
			explicit operator std::string() const {return str[v];}
			operator base_t() const {return v;}
			friend std::ostream &operator<<(std::ostream &os, Reg reg) {
				return os<<std::string(reg);
			}
		};
		struct UReg {
			using base_t=unsigned ;
			#define TABLE \
				X(A  ) X(B  ) \
				X(C  ) X(D  ) \
				X(E  ) X(F  ) \
				X(L  ) X(H  ) \

			enum Value:base_t{
				#define X(a) a,
					TABLE
				#undef X
			};
			inline static const std::string str[]={
				#define X(a) #a,
					TABLE
				#undef X
			};
			#undef TABLE
			Value v;
			UReg() = default;
			constexpr UReg(Value v) : v(v) { }
			explicit constexpr UReg(base_t v) : v(static_cast<Value>(v)) { }
			//constexpr bool operator==(UReg a) const { return v == a.v; }
			//constexpr bool operator!=(UReg a) const { return v != a.v; }
			explicit operator std::string() const {return str[v];}
			operator base_t() const {return v;}
			explicit operator Reg() const {return Reg((base_t)v+(base_t)Reg::A);}
			friend std::ostream &operator<<(std::ostream &os, UReg ureg) {
				return os<<std::string(ureg);
			}
		};
		struct Reg16 {
			using base_t=unsigned;
			#define TABLE \
				X(IMM) \
				X(TMP) \
				X(SP ) \
				X(PC ) \
				X(BA ) \
				X(DC ) \
				X(FE ) \
				X(HL ) \

			enum Value:base_t {
				#define X(a) a,
					TABLE
				#undef X
			};
			inline static const std::string str[]={
				#define X(a) #a,
					TABLE
				#undef X
			};
			#undef TABLE
			Value v;
			Reg16() = default;
			constexpr Reg16(Value v) : v(v) { }
			explicit constexpr Reg16(base_t v) : v(static_cast<Value>(v)) { }
			//constexpr bool operator==(Reg16 a) const { return v == a.v; }
			//constexpr bool operator!=(Reg16 a) const { return v != a.v; }
			explicit operator std::string() const {return str[v];}
			operator base_t() const {return v;}
			explicit operator Reg() const {return Reg(v<<1u);}
			Reg L(){return Reg((v<<1u)+0);}
			Reg H(){return Reg((v<<1u)+1);}
			friend std::ostream &operator<<(std::ostream &os, Reg16 reg16) {
				return os<<std::string(reg16);
			}
		};
	}
	using namespace Regs;
}
#endif //BREADBOARDCPU_MARG_H
