//
// Created by chasingfar on 2021/9/25.
//

#ifndef BBCPU_LIBRARY_H
#define BBCPU_LIBRARY_H
#include <unordered_map>
#include "operator.h"
namespace BBCPU::Lang::Library{
	struct Lib{
		std::unordered_map<void*,Code> fns{};
		void clear(){
			fns.clear();
		}
		size_t size() const{
			return fns.size();
		}
		void add(void* fn,Code impl){
			fns.try_emplace(fn,impl);
		}
		Code to_code() const{
			Label end;
			Code code{};
			for (const auto&[fn, impl] : fns) {
				code << impl;
			}
			return {jmp(end),code,end};
		}
	};
	inline static Lib stdlib;


	template<addr_t Size,bool Signed>
	inline Int<Size,Signed> mul(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		static Fn<Int<Size,Signed>(Int<Size,Signed>,Int<Size,Signed>)> fn{
			[](auto& _,const Int<Size,Signed>& ls,const Int<Size,Signed>& rs)->Stmt{
				u8 rs1{rs.as_mem_var()->shift(0,1)};
				auto ret_value=_.ret.as_mem_var();
				return {
					([&]<size_t ...I>(std::index_sequence<I...>)->Stmt{
						if constexpr(Signed){
							i8 ret[Size]{i8{ret_value->shift(I, 1)}...};
							return {ret[I].set(0_i8)...};
						}else{
							u8 ret[Size]{u8{ret_value->shift(I, 1)}...};
							return {ret[I].set(0_u8)...};
						}
					})(std::make_index_sequence<Size>{}),
					while_(rs).do_({
						if_(rs1 & 1_u8).then({
							_.ret += ls,
						}).end(),
						ls <<= 1,
						rs >>= 1,
					}).end(),
					_.return_(),
				};
			}
		};
		stdlib.add(std::addressof(fn),fn.to_code());
		return fn(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator*(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return mul(lhs,rhs);
	}
}
#endif //BBCPU_LIBRARY_H
