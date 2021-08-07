//
// Created by chasingfar on 2021/8/4.
//

#ifndef BREADBOARDCPU_OPERATOR_H
#define BREADBOARDCPU_OPERATOR_H
#include "var.h"
namespace BreadBoardCPU::ASM {

	inline RValue operator!(const RValue& lhs){
		RValue tmp{lhs};
		for (addr_t i = 1; i < tmp.size; ++i) {
			tmp.push << OR();
		}
		Label if_zero, end;
		tmp.push << code_t{
			brz(if_zero),
			imm(0),
			jmp(end),
			if_zero,
			imm(1),
			end,
		};
		return {1,tmp.push,false};
	}
	inline RValue operator!=(const RValue& lhs,const RValue& rhs){
		RValue tmp{sub(lhs,rhs)};
		for (addr_t i = 1; i < tmp.size; ++i) {
			tmp.push << OR();
		}
		return tmp;
	}
	inline RValue operator==(const RValue& lhs,const RValue& rhs){
		return !(lhs!=rhs);
	}

	inline RValue operator>=(const RValue& lhs,const RValue& rhs){
		RValue tmp{sub(lhs,rhs)};
		Label if_ge_zero, end;
		tmp.push << code_t{
			adj(tmp.size),
			brc(if_ge_zero),
			imm(0),
			jmp(end),
			if_ge_zero,
			imm(1),
			end,
		};
		return {1,tmp.push,false};
	}
	inline RValue operator<(const RValue& lhs,const RValue& rhs){
		RValue tmp{sub(lhs,rhs)};
		Label if_ge_zero, end;
		tmp.push << code_t{
			adj(tmp.size),
			brc(if_ge_zero),
			imm(1),
			jmp(end),
			if_ge_zero,
			imm(0),
			end,
		};
		return {1,tmp.push,false};
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
}
#endif //BREADBOARDCPU_OPERATOR_H
