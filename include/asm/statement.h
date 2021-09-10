//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_STATEMENT_H
#define BBCPU_STATEMENT_H

#include <utility>

#include "type.h"

namespace BBCPU::ASM {
	struct Block {
		Label start;
		code_t body{};
		Label end;
		Block()=default;
		Block(code_t code):body(std::move(code)){}
		Block(Label start,code_t code):start(std::move(start)),body(std::move(code)){}
		Block(Label start,code_t code,Label end):start(std::move(start)),body(std::move(code)),end(std::move(end)){}
		Block &operator<<(code_t code) {
			body << std::move(code);
			return *this;
		}
		operator code_t(){
			return {start,body,end};
		}
	};
	struct IF{
		Bool cond;
		Block if_true;
		Block if_false{};
		operator code_t(){
			return {
				cond,
				brz(if_false.start),
				if_true,
				jmp(if_false.end),
				if_false,
			};
		}
	};
	struct IFC{
		Void cond;
		Block if_no_carry;
		Block if_carry{};
		operator code_t(){
			return {
				cond,
				brc(if_carry.start),
				if_no_carry,
				jmp(if_carry.end),
				if_carry,
			};
		}
	};
	struct While{
		Bool cond;
		Block body{};
		operator code_t(){
			Label start,end;
			return {
				start,
				cond,
				brz(end),
				body,
				jmp(start),
				end,
			};
		}
	};
	struct StaticVars:Block{
		template<typename Var,typename ...Rest>
		auto get(code_t value,typename std::pair<Rest,code_t>::second_type ... rest){
			Var var{StaticVar::make(Var::size, start, static_cast<offset_t>(data_size(body)))};
			body.insert(body.end(),value.begin(),value.end());
			for(addr_t i=data_size(value);i<Var::size;++i){
				body.emplace_back(0);
			}
			if constexpr (sizeof...(Rest)==0){
				return std::tuple{var};
			}else{
				return std::tuple_cat(std::tuple{var}, get<Rest...>(rest...));
			}
		}
	};
}
#endif //BBCPU_STATEMENT_H
