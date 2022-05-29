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
	struct StaticVars:Block,PresetAllocator<code_t>{
		std::shared_ptr<MemVar> alloc_preset(addr_t size,code_t value) override {
			auto var=StaticVar::make(size, start, static_cast<offset_t>(data_size(body)));
			body.insert(body.end(),value.begin(),value.end());
			for(addr_t i=data_size(value);i<size;++i){
				body.emplace_back(static_cast<op_t>(0));
			}
			return var;
		}
	};
}
#endif //BBCPU_STATEMENT_H
