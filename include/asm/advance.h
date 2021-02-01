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
}
#endif //BREADBOARDCPU_ADVANCE_H
