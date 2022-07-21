//
// Created by chasingfar on 2021/8/4.
//

#ifndef BBCPU_OPERATOR_H
#define BBCPU_OPERATOR_H
#include "function.h"
namespace BBCPU::ASM {

	template<addr_t Size,bool Signed,auto fn,auto fnc=fn>
	inline Int<Size,Signed> calc_(const Int<Size,Signed>& lhs) {
		code_t tmp{lhs};
		if constexpr (Size > 1) {
			tmp << []<size_t ...I>(std::index_sequence<I...>){
				return Function::InplaceFn<Int<Size,Signed>(Int<I-I+1,Signed>...)>{
					[](auto& _,Int<I-I+1,Signed>...ls)->code_t{
						return {
							ls.set(calc_<1,Signed,I==0?fn:fnc,fnc>(ls))...
						};
					}
				};
			}(std::make_index_sequence<Size>{});
		} else if constexpr (Size == 1) {
			tmp << fn();
		}
		return tmp;
	}

	template<addr_t Size,bool Signed,auto fn,auto fnc=fn>
	inline Int<Size,Signed> calc_(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {
		code_t tmp{lhs};
		tmp << rhs;
		if constexpr (Size > 1) {
			tmp << []<size_t ...I>(std::index_sequence<I...>){
				return Function::InplaceFn<Int<Size,Signed>(Int<I-I+1,Signed>...,Int<I-I+1,Signed>...)>{
					[](auto& _,Int<I-I+1,Signed>...ls,Int<I-I+1,Signed>...rs)->code_t{
						return {
							ls.set(calc_<1,Signed,I==0?fn:fnc,fnc>(ls,rs))...
						};
					}
				};
			}(std::make_index_sequence<Size>{});
		} else if constexpr (Size == 1) {
			tmp << fn();
		}
		return Int<Size,Signed>{tmp};
	}

	template<addr_t Size,bool Signed> inline auto shl(const Int<Size,Signed>& lhs)                      {return calc_<Size,Signed,Ops::shl,Ops::rcl>(lhs);}
	template<addr_t Size,bool Signed> inline auto rcl(const Int<Size,Signed>& lhs)                      {return calc_<Size,Signed,Ops::rcl,Ops::rcl>(lhs);}
	template<addr_t Size,bool Signed> inline auto shr(const Int<Size,Signed>& lhs)                      {return calc_<Size,Signed,Ops::shr,Ops::rcr>(lhs);}
	template<addr_t Size,bool Signed> inline auto rcr(const Int<Size,Signed>& lhs)                      {return calc_<Size,Signed,Ops::rcr,Ops::rcr>(lhs);}
	template<addr_t Size,bool Signed> inline auto add(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Ops::add,Ops::adc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto adc(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Ops::adc,Ops::adc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto sub(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Ops::sub,Ops::suc>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto suc(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Ops::suc,Ops::suc>(lhs,rhs);}

	template<addr_t Size,bool Signed> inline auto NOT(const Int<Size,Signed>& lhs)                      {return calc_<Size,Signed,Ops::NOT,Ops::NOT>(lhs);}
	template<addr_t Size,bool Signed> inline auto AND(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Ops::AND,Ops::AND>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto  OR(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Ops:: OR,Ops:: OR>(lhs,rhs);}
	template<addr_t Size,bool Signed> inline auto XOR(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs) {return calc_<Size,Signed,Ops::XOR,Ops::XOR>(lhs,rhs);}

	template<typename To,typename From>
	concept CanCastTo = requires(From from) {TypeCaster<To,From>::to(from);};
	template<typename To,typename From> requires CanCastTo<To, From>
	inline static auto to(From from){return TypeCaster<To,From>::to(from);}

	template<addr_t Size,bool Signed>
	inline auto operator!(const Int<Size,Signed>& lhs){
		return bool_{code_t{
			IF{lhs,
				{Val::false_},
				{Val::true_},
			}
		}};
	}
	template<addr_t Size,bool Signed>
	inline auto operator!=(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return ((bool_)lhs - rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator==(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return !(lhs!=rhs);
	}

	template<addr_t Size,bool Signed>
	inline auto operator>=(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return bool_{code_t{
			IFC{(void_)(lhs - rhs),
				{Val::false_},
				{Val::true_},
			}
		}};
	}
	template<addr_t Size,bool Signed>
	inline auto operator<(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return bool_{code_t{
			IFC{(void_)(lhs - rhs),
				{Val::true_},
				{Val::false_},
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
