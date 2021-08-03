//
// Created by chasingfar on 2021/8/4.
//

#ifndef BREADBOARDCPU_OPERATOR_H
#define BREADBOARDCPU_OPERATOR_H
#include "basic.h"
namespace BreadBoardCPU::ASM {

#define ZERO_SHRINK                         \
	for (addr_t i = 1; i < tmp.size; ++i) { \
		tmp.push<<OR();                     \
	}
#define ZERO_REVERSE                        \
	Label if_zero,end;                      \
	tmp.push<<code_t{                       \
		brz(if_zero),                       \
		imm(0),                             \
		jmp(end),                           \
		if_zero,                            \
		imm(1),                             \
		end,                                \
	}
#define SUB_CMP(ge_zero,lt_zero)            \
	Label if_ge_zero,end;                   \
	tmp.push<<code_t{                       \
		adj(tmp.size),                      \
		brc(if_ge_zero),                    \
		imm(lt_zero),                       \
		jmp(end),                           \
		if_ge_zero,                         \
		imm(ge_zero),                       \
		end,                                \
	}
	inline RValue operator!(const RValue& lhs){
		RValue tmp{lhs};
		ZERO_SHRINK;
		ZERO_REVERSE;
		return tmp;
	}
	inline RValue operator!=(const RValue& lhs,const RValue& rhs){
		RValue tmp{sub(lhs,rhs)};
		ZERO_SHRINK;
		return tmp;
	}
	inline RValue operator==(const RValue& lhs,const RValue& rhs){
		RValue tmp{lhs!=rhs};
		ZERO_REVERSE;
		return tmp;
	}
	inline RValue operator>=(const RValue& lhs,const RValue& rhs){
		RValue tmp{sub(lhs,rhs)};
		SUB_CMP(1,0);
		return tmp;
	}
	inline RValue operator<(const RValue& lhs,const RValue& rhs){
		RValue tmp{sub(lhs,rhs)};
		SUB_CMP(0,1);
		return tmp;
	}
	inline RValue operator<=(const RValue& lhs,const RValue& rhs){
		return rhs>=lhs;
	}
	inline RValue operator>(const RValue& lhs,const RValue& rhs){
		return rhs<lhs;
	}
	inline RValue operator+(const RValue& lhs,const RValue& rhs){
		return add(lhs,rhs);
	}
	inline RValue operator-(const RValue& lhs,const RValue& rhs){
		return sub(lhs,rhs);
	}
	inline RValue operator<<(const RValue& lhs,size_t n){
		RValue tmp{lhs};
		for (size_t i = 0; i < n; ++i) {
			tmp=shl(tmp);
		}
		return tmp;
	}
	inline RValue operator>>(const RValue& lhs,size_t n){
		RValue tmp{lhs};
		for (size_t i = 0; i < n; ++i) {
			tmp=shr(tmp);
		}
		return tmp;
	}
	inline RValue operator&(const RValue& lhs,const RValue& rhs){
		return AND(lhs,rhs);
	}
	inline RValue operator|(const RValue& lhs,const RValue& rhs){
		return OR(lhs,rhs);
	}
	inline RValue operator^(const RValue& lhs,const RValue& rhs){
		return XOR(lhs,rhs);
	}
	inline RValue operator~(const RValue& lhs){
		return NOT(lhs);
	}
#undef ZERO_SHRINK
#undef ZERO_REVERSE
#undef SUB_CMP
}
#endif //BREADBOARDCPU_OPERATOR_H
