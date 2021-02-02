//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_ADVANCE_H
#define BREADBOARDCPU_ADVANCE_H

#include <utility>

#include "basic.h"

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

		friend ASM &operator<<(ASM &asm_, const Block& block) {
			return asm_ >> block.start << block.body >> block.end;
		}
	};
	struct IF{
		code_t cond;
		Block if_true;
		Block if_false{};
		friend ASM &operator<<(ASM &asm_, IF if_) {
			auto [cond,if_true,if_false]=std::move(if_);
			return asm_
				<<cond
				<<brz(if_false.start)
				<<if_true
				<<jmp(if_false.end)
				<<if_false
			;
		}
	};
	struct While{
		Block cond;
		Block body{};
		friend ASM &operator<<(ASM &asm_, While while_) {
			auto [cond,body]=std::move(while_);
			return asm_
					<<cond
					<<brz(body.end)
					<<body
					<<jmp(cond.start)
					;
		}
	};

	auto test_if(){
		ASM program{};
		return program
			<<IF{{imm(0)},
			     {imm(Reg::A,1)},
			     {imm(Reg::A,2)}
			}
			<<imm(Reg::B,3)
			<<halt()
			<<ASM::END
			;
	}
}
#endif //BREADBOARDCPU_ADVANCE_H
