//
// Created by chasingfar on 2021/9/25.
//

#ifndef BBCPU_LIBRARY_H
#define BBCPU_LIBRARY_H
#include <unordered_map>
#include "operator.h"
namespace BBCPU::ASM::Library{
	struct Lib{
		std::unordered_map<void*,code_t> fns{};
		void clear(){
			fns.clear();
		}
		size_t size() const{
			return fns.size();
		}
		void add(void* fn,code_t impl){
			fns.try_emplace(fn,impl);
		}
		operator code_t() {
			Label end;
			code_t code{};
			for (const auto&[fn, impl] : fns) {
				code << impl;
			}
			return {jmp(end),code,end};
		}
	};
	inline static Lib stdlib;


	template<addr_t Size,bool Signed>
	inline Int<Size,Signed> mul(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		static Function::Fn<Int<Size,Signed>,Int<Size,Signed>,Int<Size,Signed>> fn{
			[](auto& _,const Int<Size,Signed>& ls,const Int<Size,Signed>& rs)->code_t{
				UInt8 rs1{std::dynamic_pointer_cast<MemVar>(rs.value)->shift(0,1)};
				auto ret_value=std::dynamic_pointer_cast<MemVar>(_.ret.value);
				return {
					([&]<size_t ...I>(std::index_sequence<I...>){
						if constexpr(Signed){
							Int8 ret[Size]{{ret_value->shift(I, 1)}...};
							return (code_t{}<<...<<ret[I].set(0_i8));
						}else{
							UInt8 ret[Size]{{ret_value->shift(I, 1)}...};
							return (code_t{}<<...<<ret[I].set(0_u8));
						}
					})(std::make_index_sequence<Size>{}),
					While{rs, {{
						IF{{rs1 & 1_u8},{
							_.ret += ls,
						}},
						ls <<= 1,
						rs >>= 1,
					}}},
					_._return(),
				};
			}
		};
		stdlib.add(&fn,fn);
		return fn(lhs,rhs);
	}
	template<addr_t Size,bool Signed>
	inline auto operator*(const Int<Size,Signed>& lhs,const Int<Size,Signed>& rhs){
		return mul(lhs,rhs);
	}
}
#endif //BBCPU_LIBRARY_H
