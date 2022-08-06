#ifndef BBCPU_CPU_H
#define BBCPU_CPU_H

#ifndef CPU_ISA
#define CPU_ISA RegSet_SRAM
#endif

#include "regfile8x16/cpu.h"
#include "regset_sram/cpu.h"

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
		Util::Printer print() const override{
			return [&](std::ostream& os){
				os<<"OP:"<<Ops::all::parse(get_op()).first<<std::endl;
				os<<Base::print();
			};
		}
	};
}
#endif //BBCPU_CPU_H
