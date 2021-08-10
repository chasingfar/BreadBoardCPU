//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_STATEMENT_H
#define BREADBOARDCPU_STATEMENT_H

#include <utility>

#include "operator.h"

namespace BreadBoardCPU::ASM {
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
		const RValue& cond;
		Block if_true;
		Block if_false{};
		operator code_t(){
			return {
				cond.push,
				brz(if_false.start),
				if_true,
				jmp(if_false.end),
				if_false,
			};
		}
	};
	struct While{
		const RValue& cond;
		Block body{};
		operator code_t(){
			Label start,end;
			return {
				start,
				cond.push,
				brz(end),
				body,
				jmp(start),
				end,
			};
		}
	};

	template<typename Type=op_t>
	struct StaticVar: Var{
		using type = Type;
		offset_t offset=0;
		explicit StaticVar(Block& block,type value)
				:offset(block.body.size()),Var{sizeof(Type),{},{},std::is_signed_v<Type>}{
			for(addr_t i = 0; i < size; ++i) {
				push<<Ops::load(block.start,offset+i);
				pop<<Ops::save(block.start,offset+(size-1-i));
				block<<code_t{(value>>i*8)&0xFF};
			}
		}
	};
	struct StaticVars{
		Block block;

		template<typename Var,typename ...Rest>
		auto getVars(Var value, Rest ... rest){
			std::tuple<StaticVar<Var>> var{StaticVar<Var>{block, value}};
			if constexpr (sizeof...(Rest)==0){
				return var;
			}else{
				return std::tuple_cat(var, getVars<Rest...>(rest...));
			}
		}
		operator code_t(){
			return {block};
		}
	};
}
#endif //BREADBOARDCPU_STATEMENT_H
