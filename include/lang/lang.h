//
// Created by chasingfar on 2021/9/13.
//

#ifndef BBCPU_LANG_H
#define BBCPU_LANG_H

#include "library.h"

namespace BBCPU::Lang::Impl{
	struct CPU:ASM::Impl::CPU{
		using Base=ASM::Impl::CPU;
		word_t get_local(const auto& var) const{
			return read_ptr(CPU::ASM_BP,var.template as<LocalVar>()->offset);
		}
		word_t get_static(const CodeBlock& block,const auto& var) const{
			return mem.get_data(*block.start.addr+var.template as<StaticVar>()->offset).value();
		}
	};
}
#endif //BBCPU_LANG_H
