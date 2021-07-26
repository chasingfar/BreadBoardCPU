//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_ADVANCE_H
#define BREADBOARDCPU_ADVANCE_H

#include <utility>

#include "basic.h"

namespace BreadBoardCPU::ASM {
	struct Var{
		code_t push;
		code_t pop;
		code_t load(Reg to) const{
			return {push,Ops::pop(to)};
		}
		code_t save(Reg value) const{
			return {Ops::push(value),pop};
		}
	};
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
		code_t cond;
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
	struct While{
		Block cond;
		Block body{};
		operator code_t(){
			return {
				cond,
				brz(body.end),
				body,
				jmp(cond.start),
			};
		}
	};
}
#endif //BREADBOARDCPU_ADVANCE_H
