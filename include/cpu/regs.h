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
			Reg toReg() const { return Reg(v+(base_t)Reg::A);}
			explicit operator Reg() const {return toReg();}
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
			Reg toReg() const { return Reg(v<<1u);}
			explicit operator Reg() const {return toReg();}
			Reg L() const {return Reg(toReg()+0);}
			Reg H() const {return Reg(toReg()+1);}
			friend std::ostream &operator<<(std::ostream &os, Reg16 reg16) {
				return os<<std::string(reg16);
			}
		};
		struct UReg16 {
			using base_t=unsigned;
			#define TABLE \
				X(BA ) \
				X(DC ) \
				X(FE ) \
				X(HL ) \

			//0->0->8 1->2->10 2->4->12 3->6->14
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
			UReg16() = default;
			constexpr UReg16(Value v) : v(v) { }
			explicit constexpr UReg16(base_t v) : v(static_cast<Value>(v)) { }
			//constexpr bool operator==(UReg16 a) const { return v == a.v; }
			//constexpr bool operator!=(UReg16 a) const { return v != a.v; }
			explicit operator std::string() const {return str[v];}
			operator base_t() const {return v;}
			UReg toUReg() const { return UReg(v<<1u);}
			Reg16 toReg16() const { return Reg16(v+(base_t)Reg16::BA);}
			Reg toReg() const { return toUReg().toReg();}
			explicit operator UReg() const {return toUReg();}
			explicit operator Reg() const {return toReg();}
			explicit operator Reg16() const {return toReg16();}
			UReg L() const {return UReg(toUReg()+0);}
			UReg H() const {return UReg(toUReg()+1);}
			friend std::ostream &operator<<(std::ostream &os, UReg16 ureg16) {
				return os<<std::string(ureg16);
			}
		};
		static Reg user(UReg ureg){
			return ureg.toReg();
		}
		static Reg pair(Reg16 reg16){
			return reg16.toReg();
		}
		static Reg upair(UReg16 ureg16){
			return ureg16.toReg();
		}
	}
	using namespace Regs;
}
#endif //BREADBOARDCPU_CPU_REGS_H
