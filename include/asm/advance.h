//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_ADVANCE_H
#define BREADBOARDCPU_ADVANCE_H

#include "basic.h"

namespace BreadBoardCPU::ASM {
	struct Block {
		Label label;
		code_t body{};

		Block &operator<<(code_t code) {
			body << std::move(code);
			return *this;
		}

		friend ASM &operator<<(ASM &asm_, Block block) {
			return asm_ >> block.label << block.body;
		}
	};
	struct IF{
		code_t cond;
		code_t if_true;
		code_t if_false={};
		friend ASM &operator<<(ASM &asm_, IF if_) {
			Label label_false{"if_false"},label_end{"if_end"};
			auto [cond,if_true,if_false]=if_;
			return asm_
				<<cond
				<<brz(label_false)
				<<if_true
				<<jmp(label_end)
				>>label_false
				<<if_false
				>>label_end
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
