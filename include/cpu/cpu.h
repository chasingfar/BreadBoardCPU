#ifndef BBCPU_CPU_H
#define BBCPU_CPU_H

#include "opcode.h"

namespace BBCPU::OpCode::Impl{
	struct CPU : CPU_ISA::Impl::CPU {
		using Base = CPU_ISA::Impl::CPU;

		CPU():Base(){
			auto tbl=genOpTable();
			std::copy(tbl.begin(), tbl.end(), cu.tbl.data);
			init();
		}
		bool is_halt() const{
			return get_op() == Ops::Halt::id::id;
		}
		Util::Printer print(const std::vector<Reg16>& reg_ptrs) const override{
			return [=](std::ostream& os){
				os<<"OP:"<<Ops::all::parse(get_op()).first<<std::endl;
				os<<Base::print(reg_ptrs);
			};
		}
	};
}
#endif //BBCPU_CPU_H
