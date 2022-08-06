//
// Created by chasingfar on 2020/11/23.
//

#ifndef BBCPU_ASM_ASM_H
#define BBCPU_ASM_ASM_H

#include "ops.h"
namespace BBCPU::ASM::Impl{
	struct CPU:OpCode::Impl::CPU{
		using Base=OpCode::Impl::CPU;
		using Base::load;
		CPU& load(Code code,addr_t start=0){
			Base::load(code.assemble(start),start);
			return *this;
		}
		CPU& run_op(Code code){
			load_op(code.assemble());
			tick_op();
			return *this;
		}
		CPU& run_to_halt(const std::vector<Label>& pause_at={},size_t max_iter=1024){
			for (size_t i = 0; i < max_iter; ++i) {
				tick_op();
				if(is_halt()){
					return *this;
				}
				for (const auto& label:pause_at){
					if (get_reg16(Base::Reg16::PC) == *label){
						return *this;
					}
				}
			}
			std::cout<<"run over max_iter="<<max_iter<<std::endl;
			return *this;
		}
	};
}
#endif //BBCPU_ASM_ASM_H
