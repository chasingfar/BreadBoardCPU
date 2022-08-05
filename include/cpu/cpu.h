#ifndef BBCPU_CPU_H
#define BBCPU_CPU_H

#ifdef Regfile8x16
	#define CPU_ISA RegFile8x16
	#include "regfile8x16/cpu.h"
#else
	#define CPU_ISA RegSet_SRAM
	#include "regset_sram/cpu.h"
#endif

#endif //BBCPU_CPU_H
