//
// Created by chasingfar on 2021/8/4.
//

#ifndef BBCPU_OPERATOR_H
#define BBCPU_OPERATOR_H
#include "function.h"
#include <cstddef>
namespace BBCPU::ASM {

	template<addr_t Size,bool Signed,auto fn,auto fnc=fn>
	inline auto _calc(const Value<Int<Size,Signed>>& lhs) {
		Expr<Int<Size,Signed>> tmp{lhs};
		if constexpr (Size > 1) {
			tmp << []<size_t ...I>(std::index_sequence<I...>){
				return Function::Fn<Int<Size,Signed>,Int<I-I+1,Signed>...>::inplace(
					[](const Label& end,LocalVar<Int<Size,Signed>> ret,LocalVar<Int<I-I+1,Signed>>...ls)->code_t{
						return {
							ls.set(_calc<1,Signed,I==0?fn:fnc,fnc>(ls))...
						};
					}
				);
			}(std::make_index_sequence<Size>{});
		} else if constexpr (Size == 1) {
			tmp << fn();
		}
		return tmp;
	}

	template<addr_t Size,bool Signed,auto fn,auto fnc=fn>
	inline auto _calc(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {
		Expr<Int<Size,Signed>> tmp{lhs};
		tmp << rhs;
		if constexpr (Size > 1) {
			tmp << []<size_t ...I>(std::index_sequence<I...>){
				return Function::Fn<Int<Size,Signed>,Int<I-I+1,Signed>...,Int<I-I+1,Signed>...>::inplace(
					[](const Label& end,LocalVar<Int<Size,Signed>> ret,LocalVar<Int<I-I+1,Signed>>...ls,LocalVar<Int<I-I+1,Signed>>...rs)->code_t{
						return {
							ls.set(_calc<1,Signed,I==0?fn:fnc,fnc>(ls,rs))...
						};
					}
				);
			}(std::make_index_sequence<Size>{});
		} else if constexpr (Size == 1) {
			tmp << fn();
		}
		return tmp;
	}

	template<addr_t Size,bool Signed> inline auto shl(const Value<Int<Size,Signed>>& lhs)                                    {return _calc<Size,Signed,Ops::shl,Ops::rcl>(lhs);}
	template<addr_t Size,bool Signed> inline auto rcl(const Value<Int<Size,Signed>>& lhs)                                    {return _calc<Size,Signed,Ops::rcl,Ops::rcl>(lhs);}
	template<addr_t Size,bool Signed> inline auto shr(const Value<Int<Size,Signed>>& lhs)                                    {return _calc<Size,Signed,Ops::shr,Ops::rcr>(lhs);}
	template<addr_t Size,bool Signed> inline auto rcr(const Value<Int<Size,Signed>>& lhs)                                    {return _calc<Size,Signed,Ops::rcr,Ops::rcr>(lhs);}
	template<addr_t Size,bool Signed> inline auto add(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::add,Ops::adc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto adc(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::adc,Ops::adc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto sub(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::sub,Ops::suc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto suc(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::suc,Ops::suc>(lhs,rhs);}

	template<addr_t Size,bool Signed> inline auto NOT(const Value<Int<Size,Signed>>& lhs)                                    {return _calc<Size,Signed,Ops::NOT,Ops::NOT>(lhs);}
	template<addr_t Size,bool Signed> inline auto AND(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::AND,Ops::AND>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto  OR(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops:: OR,Ops:: OR>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto XOR(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs) {return _calc<Size,Signed,Ops::XOR,Ops::XOR>(lhs,rhs);}

	template<addr_t Size,bool Signed>
	struct TypeCaster<Bool,Int<Size,Signed>>{
		static Expr<Bool> to(const Value<Int<Size,Signed>>& from){
			Expr<Bool> tmp{from};
			for (addr_t i = 1; i < Size; ++i) {
				tmp << OR();
			}
			return tmp;
		}
	};

	template<typename To,typename From>
	concept CanCastTo = requires(const Value<From>& from) {TypeCaster<To,From>::to(from);};
	template<typename To,typename From> requires CanCastTo<To, From>
	inline static auto to(const Value<From>& from){return TypeCaster<To,From>::to(from);}

	template<addr_t Size,bool Signed>
	inline auto operator!(const Value<Int<Size,Signed>>& lhs){
		return Expr<Bool>{IF{to<Bool>(lhs),
			{imm(0)},
			{imm(1)}
		}};
	}
	template<addr_t Size,bool Signed>
	inline auto operator!=(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return to<Bool>(lhs-rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator==(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return !(lhs!=rhs);
	}

	template<addr_t Size,bool Signed>
	inline auto operator>=(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return Expr<Bool>{IFC{Stmt{lhs - rhs},
			{imm(0)},
			{imm(1)}
		}};
	}
	template<addr_t Size,bool Signed>
	inline auto operator<(const Value<Int<Size,Signed>>& lhs,const Value<Int<Size,Signed>>& rhs){
		return Expr<Bool>{IFC{Stmt{lhs - rhs},
			{imm(1)},
			{imm(0)}
		}};
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

	template<typename T>
	inline auto operator+=(const Var<T>& var,const Value<T>& value){
		return var.set(var+value);
	}
	template<typename T>
	inline auto operator-=(const Var<T>& var,const Value<T>& value){
		return var.set(var-value);
	}
	template<typename T>
	inline auto operator<<=(const Var<T>& var,size_t value){
		return var.set(var<<value);
	}
	template<typename T>
	inline auto operator>>=(const Var<T>& var,size_t value){
		return var.set(var>>value);
	}
	template<typename T>
	inline auto operator&=(const Var<T>& var,const Value<T>& value){
		return var.set(var&value);
	}
	template<typename T>
	inline auto operator|=(const Var<T>& var,const Value<T>& value){
		return var.set(var|value);
	}
	template<typename T>
	inline auto operator^=(const Var<T>& var,const Value<T>& value){
		return var.set(var^value);
	}
}
#endif //BBCPU_OPERATOR_H
