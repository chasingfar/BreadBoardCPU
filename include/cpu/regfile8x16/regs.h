//
// Created by chasingfar on 2021/2/1.
//

#ifndef BBCPU_CPU_REGS_H
#define BBCPU_CPU_REGS_H
#include "../common.h"
namespace BBCPU{
	namespace Regs{
		#define ENUM_NAME(...) Reg##__VA_ARGS__
		#define ENUM_TABLE \
			X(OPR) X(TMA) \
			X(TML) X(TMH) \
			X(SPL) X(SPH) \
			X(PCL) X(PCH) \
			X(A  ) X(B  ) \
			X(C  ) X(D  ) \
			X(E  ) X(F  ) \
			X(L  ) X(H  ) \

		#include "../define_enum_x.h"


		#define ENUM_NAME(...) UReg##__VA_ARGS__
		#define ENUM_TABLE \
			X(A  ) X(B  ) \
			X(C  ) X(D  ) \
			X(E  ) X(F  ) \
			X(L  ) X(H  ) \

		#define ENUM_OTHER \
		Reg toReg() const { return Reg(v()+(Reg::A).v());} \
		explicit operator Reg() const {return toReg();}    \

		#include "../define_enum_x.h"

		#define ENUM_NAME(...) Reg16##__VA_ARGS__
		#define ENUM_TABLE \
			X(IMM) \
			X(TMP) \
			X(SP ) \
			X(PC ) \
			X(BA ) \
			X(DC ) \
			X(FE ) \
			X(HL ) \

		#define ENUM_OTHER \
		Reg toReg() const { return Reg(v()<<1u);} \
		explicit operator Reg() const {return toReg();} \
		Reg L() const {return Reg(toReg().v()+0);} \
		Reg H() const {return Reg(toReg().v()+1);} \

		#include "../define_enum_x.h"


		#define ENUM_NAME(...) UReg16##__VA_ARGS__
		#define ENUM_TABLE \
			X(BA ) \
			X(DC ) \
			X(FE ) \
			X(HL ) \

		#define ENUM_OTHER \
		UReg toUReg() const { return UReg(v()<<1u);} \
		Reg16 toReg16() const { return Reg16(v()+(Reg16::BA).v());} \
		Reg toReg() const { return toUReg().toReg();} \
		explicit operator UReg() const {return toUReg();} \
		explicit operator Reg() const {return toReg();} \
		explicit operator Reg16() const {return toReg16();} \
		UReg L() const {return UReg(toUReg().v()+0);} \
		UReg H() const {return UReg(toUReg().v()+1);} \

		#include "../define_enum_x.h"


		#define ENUM_NAME(...) RegSet##__VA_ARGS__
		#define ENUM_TABLE \
			X(I ) \
			X(A ) \
			X(L ) \
			X(H ) \

		#include "../define_enum_x.h"

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
#endif //BBCPU_CPU_REGS_H
