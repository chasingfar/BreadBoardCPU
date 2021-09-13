//
// Created by chasingfar on 2021/8/4.
//

#ifndef BBCPU_OPERATOR_H
#define BBCPU_OPERATOR_H
#include "function.h"
namespace BBCPU::ASM {

	template<addr_t Size,bool Signed,auto fn,auto fnc=fn>
	inline Int<Size,Signed> _calc(const Int<Size,Signed>& lhs) {
		code_t tmp{lhs};
		if constexpr (Size > 1) {
			tmp << []<size_t ...I>(std::index_sequence<I...>){
				return Function::Fn<Int<Size,Signed>,Int<I-I+1,Signed>...>::inplace(
					[](const Label& end,Int<Size,Signed> ret,Int<I-I+1,Signed>...ls)->code_t{
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
	inline Int<Size,Signed> _calc(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {
		code_t tmp{lhs};
		tmp << rhs;
		if constexpr (Size > 1) {
			tmp << []<size_t ...I>(std::index_sequence<I...>){
				return Function::Fn<Int<Size,Signed>,Int<I-I+1,Signed>...,Int<I-I+1,Signed>...>::inplace(
					[](const Label& end,Int<Size,Signed> ret,Int<I-I+1,Signed>...ls,Int<I-I+1,Signed>...rs)->code_t{
						return {
							ls.set(_calc<1,Signed,I==0?fn:fnc,fnc>(ls,rs))...
						};
					}
				);
			}(std::make_index_sequence<Size>{});
		} else if constexpr (Size == 1) {
			tmp << fn();
		}
		return Int<Size,Signed>{tmp};
	}

	template<addr_t Size,bool Signed> inline auto shl(const Int<Size,Signed>& lhs)                      {return _calc<Size,Signed,Ops::shl,Ops::rcl>(lhs);}
	template<addr_t Size,bool Signed> inline auto rcl(const Int<Size,Signed>& lhs)                      {return _calc<Size,Signed,Ops::rcl,Ops::rcl>(lhs);}
	template<addr_t Size,bool Signed> inline auto shr(const Int<Size,Signed>& lhs)                      {return _calc<Size,Signed,Ops::shr,Ops::rcr>(lhs);}
	template<addr_t Size,bool Signed> inline auto rcr(const Int<Size,Signed>& lhs)                      {return _calc<Size,Signed,Ops::rcr,Ops::rcr>(lhs);}
	template<addr_t Size,bool Signed> inline auto add(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return _calc<Size,Signed,Ops::add,Ops::adc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto adc(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return _calc<Size,Signed,Ops::adc,Ops::adc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto sub(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return _calc<Size,Signed,Ops::sub,Ops::suc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto suc(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return _calc<Size,Signed,Ops::suc,Ops::suc>(lhs,rhs);}

	template<addr_t Size,bool Signed> inline auto NOT(const Int<Size,Signed>& lhs)                      {return _calc<Size,Signed,Ops::NOT,Ops::NOT>(lhs);}
	template<addr_t Size,bool Signed> inline auto AND(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return _calc<Size,Signed,Ops::AND,Ops::AND>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto  OR(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return _calc<Size,Signed,Ops:: OR,Ops:: OR>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto XOR(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return _calc<Size,Signed,Ops::XOR,Ops::XOR>(lhs,rhs);}

	template<typename To,typename From>
	concept CanCastTo = requires(From from) {TypeCaster<To,From>::to(from);};
	template<typename To,typename From> requires CanCastTo<To, From>
	inline static auto to(From from){return TypeCaster<To,From>::to(from);}

	template<addr_t Size,bool Signed>
	inline auto operator!(const Int<Size,Signed>& lhs){
		return Bool{code_t{
			IF{lhs,
				{Val::_false},
				{Val::_true},
			}
		}};
	}
	template<addr_t Size,bool Signed>
	inline auto operator!=(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return ((Bool)lhs-rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator==(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return !(lhs!=rhs);
	}

	template<addr_t Size,bool Signed>
	inline auto operator>=(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return Bool{code_t{
			IFC{(Void)(lhs - rhs),
				{Val::_false},
				{Val::_true},
			}
		}};
	}
	template<addr_t Size,bool Signed>
	inline auto operator<(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return Bool{code_t{
			IFC{(Void)(lhs - rhs),
				{Val::_true},
				{Val::_false},
			}
		}};
	}
	template<addr_t Size,bool Signed>
	inline auto operator<=(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return rhs>=lhs;
	}
	template<addr_t Size,bool Signed>
	inline auto operator>(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return rhs<lhs;
	}

	template<addr_t Size,bool Signed>
	inline auto operator+(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return add(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator-(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return sub(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator<<(const Int<Size,Signed>& lhs,size_t n){
		code_t tmp{lhs};
		for (size_t i = 0; i < n; ++i) {
			tmp<<shl();
		}
		return Int<Size,Signed>{tmp};
	}
	template<addr_t Size,bool Signed>
	inline auto operator>>(const Int<Size,Signed>& lhs,size_t n){
		code_t tmp{lhs};
		for (size_t i = 0; i < n; ++i) {
			tmp<<shr();
		}
		return Int<Size,Signed>{tmp};
	}
	template<addr_t Size,bool Signed>
	inline auto operator&(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return AND(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator|(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return OR(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator^(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return XOR(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator~(const Int<Size,Signed>& lhs){
		return NOT(lhs);
	}

	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator+=(const T& var,const U&value){
		return var.set(var+value);
	}
	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator-=(const T& var,const U&value){
		return var.set(var-value);
	}
	template<typename T>
	inline auto operator<<=(const T& var,size_t value){
		return var.set(var<<value);
	}
	template<typename T>
	inline auto operator>>=(const T& var,size_t value){
		return var.set(var>>value);
	}
	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator&=(const T& var,const U&value){
		return var.set(var&value);
	}
	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator|=(const T& var,const U&value){
		return var.set(var|value);
	}
	template<typename T,typename U> requires std::is_convertible_v<U,T>
	inline auto operator^=(const T& var,const U&value){
		return var.set(var^value);
	}
}
#endif //BBCPU_OPERATOR_H
