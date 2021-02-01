//
// Created by chasingfar on 2021/2/1.
//

#ifndef BREADBOARDCPU_CPU_REGS_H
#define BREADBOARDCPU_CPU_REGS_H
#include "common.h"
namespace BreadBoardCPU{
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
		static Reg user(UReg ureg){
			return Reg(ureg + Reg::A);
		}
		static Reg pair(Reg16 reg16){
			return Reg(reg16 << 1u );
		}
	}
	using namespace Regs;
}
#endif //BREADBOARDCPU_CPU_REGS_H
