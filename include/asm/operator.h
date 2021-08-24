//
// Created by chasingfar on 2021/8/4.
//

#ifndef BREADBOARDCPU_OPERATOR_H
#define BREADBOARDCPU_OPERATOR_H
#include "function.h"
namespace BreadBoardCPU::ASM {

	template<addr_t Size,bool Signed,auto fn,auto fnc=fn>
	inline auto _calc(const Value<Int<Size,Signed>>& lhs) {
		Expr<Int<Size,Signed>> tmp{lhs};
		if (Size > 1) {
			tmp << saveBP();
			for (addr_t i = 0; i < Size; ++i) {
				tmp << load_local(Size + 2 - i)
				    << (i == 0 ? fn() : fnc())
				    << save_local(Size + 2 - i);
			}
			tmp << loadBP();
		} else if (Size == 1) {
			tmp << fn();
		}
		return tmp;
	}

	template<addr_t Size,bool Signed,auto fn,auto fnc=fn>
	inline auto _calc(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {
		Expr<Int<Size,Signed>> tmp{lhs};
		tmp << rhs;
		if (Size > 1) {
			tmp << saveBP();
			for (addr_t i = 0; i < Size; ++i) {
				tmp << load_local(2 * Size + 2 - i)
				    << load_local(Size + 2 - i)
				    << (i == 0 ? fn() : fnc())
				    << save_local(2 * Size + 2 - i);
			}
			tmp << loadBP() << adj(Size);
		} else if (Size == 1) {
			tmp << fn();
		}
		return tmp;
	}

	template<addr_t Size,bool Signed> inline auto shl(const Value<Int<Size,Signed>>& lhs)                                    {return _calc<Size,Signed,Ops::shl,Ops::rcl>(lhs);}
	template<addr_t Size,bool Signed> inline auto shr(const Value<Int<Size,Signed>>& lhs)                                    {return _calc<Size,Signed,Ops::shr,Ops::rcr>(lhs);}
	template<addr_t Size,bool Signed> inline auto add(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::add,Ops::adc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto sub(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::sub,Ops::suc>(lhs,rhs);}

	template<addr_t Size,bool Signed> inline auto NOT(const Value<Int<Size,Signed>>& lhs)                                    {return _calc<Size,Signed,Ops::NOT,Ops::NOT>(lhs);}
	template<addr_t Size,bool Signed> inline auto AND(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::AND,Ops::AND>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto  OR(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops:: OR,Ops:: OR>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto XOR(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::XOR,Ops::XOR>(lhs,rhs);}

	template<typename U>
	struct to{
		template<typename T>
		Expr<U> operator()(const Value<T>& from){
			Expr<U> tmp{from};
			return tmp;
		}
	};
	template<>
	struct to<Bool>{
		template<addr_t Size,bool Signed>
		Expr<Bool> operator()(const Value<Int<Size,Signed>>& from){
			Expr<Bool> tmp{from};
			for (addr_t i = 1; i < Size; ++i) {
				tmp << OR();
			}
			return tmp;
		}
	};

	template<addr_t Size,bool Signed>
	inline auto operator!(const Value<Int<Size,Signed>>& lhs){
		Expr<Bool> tmp{to<Bool>{}(lhs)};
		Label if_zero, end;
		tmp << code_t{
			brz(if_zero),
			imm(0),
			jmp(end),
			if_zero,
			imm(1),
			end,
		};
		return tmp;
	}
	template<addr_t Size,bool Signed>
	inline auto operator!=(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return to<Bool>{}(lhs-rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator==(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return !(lhs!=rhs);
	}

	template<addr_t Size,bool Signed>
	inline auto operator>=(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		Expr<Bool> tmp{};
		Label if_ge_zero, end;
		tmp << code_t{
			lhs-rhs,
			adj(Size),
			brc(if_ge_zero),
			imm(0),
			jmp(end),
			if_ge_zero,
			imm(1),
			end,
		};
		return tmp;
	}
	template<addr_t Size,bool Signed>
	inline auto operator<(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		Expr<Bool> tmp{};
		Label if_ge_zero, end;
		tmp << code_t{
			lhs-rhs,
			adj(Size),
			brc(if_ge_zero),
			imm(1),
			jmp(end),
			if_ge_zero,
			imm(0),
			end,
		};
		return tmp;
	}
	template<addr_t Size,bool Signed>
	inline auto operator<=(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return rhs>=lhs;
	}
	template<addr_t Size,bool Signed>
	inline auto operator>(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return rhs<lhs;
	}

	template<addr_t Size,bool Signed>
	inline auto operator+(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return add(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator-(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return sub(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator<<(const Value<Int<Size,Signed>>& lhs,size_t n){
		Expr<Int<Size,Signed>> tmp{lhs};
		for (size_t i = 0; i < n; ++i) {
			tmp<<shl(tmp);
		}
		return tmp;
	}
	template<addr_t Size,bool Signed>
	inline auto operator>>(const Value<Int<Size,Signed>>& lhs,size_t n){
		Expr<Int<Size,Signed>> tmp{lhs};
		for (size_t i = 0; i < n; ++i) {
			tmp<<shr(tmp);
		}
		return tmp;
	}
	template<addr_t Size,bool Signed>
	inline auto operator&(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return AND(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator|(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return OR(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator^(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return XOR(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator~(const Value<Int<Size,Signed>>& lhs){
		return NOT(lhs);
	}
}
#endif //BREADBOARDCPU_OPERATOR_H
