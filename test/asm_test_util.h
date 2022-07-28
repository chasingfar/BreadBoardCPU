//
// Created by chasingfar on 2021/7/25.
//

#ifndef BBCPU_ASM_TEST_UTIL_H
#define BBCPU_ASM_TEST_UTIL_H

#include "catch.hpp"
#include "lang/lang.h"

using namespace BBCPU::Lang;
using BBCPU::Lang::Impl::CPU;
using ALU74181::Carry;
using MEM=decltype(CPU{}.mem);

inline op_t operator "" _op(unsigned long long value) {
    return static_cast<op_t>(value);
}

#endif //BBCPU_ASM_TEST_UTIL_H
