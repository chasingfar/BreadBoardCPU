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
	struct if_{
		bool_ cond;
		Block if_true{};
		Block if_false{};
		if_(const bool_& cond):cond(cond){}
		if_& then(const code_t& t_code){
			if_true=t_code;
			return *this;
		}
		if_& else_(const code_t& f_code){
			if_false=f_code;
			return *this;
		}
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
	struct ifc{
		void_ cond;
		Block if_no_carry{};
		Block if_carry{};
		ifc(const void_& cond):cond(cond){}
		ifc& then(const code_t& no_carry){
			if_no_carry=no_carry;
			return *this;
		}
		ifc& else_(const code_t& carry){
			if_carry=carry;
			return *this;
		}
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
	struct while_{
		bool_ cond;
		Block body{};
		while_(const bool_& cond):cond(cond){}
		while_& do_(const code_t& code){
			body=code;
			return *this;
		}
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
