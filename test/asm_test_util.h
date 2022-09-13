//
// Created by chasingfar on 2021/7/25.
//

#ifndef BBCPU_ASM_TEST_UTIL_H
#define BBCPU_ASM_TEST_UTIL_H

#include "catch.hpp"
//#define CPU_IMPL RegFile8x16
#define CPU_IMPL RegSet_SRAM
#include "lang/lang.h"

using namespace BBCPU::Lang;
using BBCPU::Lang::Impl::CPU;
using ALU74181::Carry;
using MEM=decltype(CPU{}.mem);

#endif //BBCPU_ASM_TEST_UTIL_H