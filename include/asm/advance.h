//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_ADVANCE_H
#define BREADBOARDCPU_ADVANCE_H

#include "basic.h"

namespace BreadBoardCPU::ASM {
	struct Block {
		Label &label;
		code_t body{};

		Block &operator<<(code_t code) {
			body << std::move(code);
			return *this;
		}

		friend ASM &operator<<(ASM &asm_, const Block &block) {
			return asm_ >> block.label << block.body;
		}
	};
	code_t If(code_t cond,code_t if_true,code_t if_false={}){
		Label label_false,label_end;
		return {
			cond,
			brz(label_false),
			if_true,
			jmp(label_end),
			label_false,
			if_false,
			label_end,
		};
	}

	auto test_if(){
		ASM program{};
		return program
			<<If({imm(0)},
			     {imm(Reg::A,1)},
			     {imm(Reg::A,2)}
			)
			<<imm(Reg::B,3)
			<<halt()
			<<ASM::END
			;
	}
}
#endif //BREADBOARDCPU_ADVANCE_H
