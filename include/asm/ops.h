//
// Created by chasingfar on 2021/8/7.
//

#ifndef BREADBOARDCPU_OPS_H
#define BREADBOARDCPU_OPS_H

#include "basic.h"
namespace BreadBoardCPU::ASM {

#define OP0(op) op::id::setAs<MARG::opcode,op_t>()
#define OP1(op, n1, v1) op::n1::setAs<MARG::opcode>(OP0(op),v1)
#define OP2(op, n1, v1, n2, v2) op::n2::setAs<MARG::opcode>(OP1(op,n1,v1),v2)
#define GET_H(v) static_cast<op_t>(((v) >> 8) & 0xff)
#define GET_L(v) static_cast<op_t>((v)  & 0xff)
#define GET_HL(v) GET_H(v),GET_L(v)
#define GET_LH(v) GET_L(v),GET_H(v)
#define LAZY(fn) [=](addr_t pc){return fn;}
#define LAZY_H(addr) LAZY(GET_H(*addr))
#define LAZY_L(addr) LAZY(GET_L(*addr))
#define ADDR_HL(addr) LAZY_H(addr),LAZY_L(addr)
#define ADDR_LH(addr) LAZY_L(addr),LAZY_H(addr)
	namespace Ops {
		inline code_t load(Reg16 addr, offset_t offset=0)  {return {OP1(Load, from, addr),GET_HL(offset)};}
		inline code_t save(Reg16 addr, offset_t offset=0)  {return {OP1(Save, to, addr),GET_HL(offset)};}
		inline code_t push(Reg fromReg)       {return {OP1(Push, from, fromReg)};}
		inline code_t pop (Reg toReg)         {return {OP1(Pop, to, toReg)};}
		inline code_t imm (op_t value)        {return {OP0(ImmVal), value};}
		inline code_t imm (const Label& addr) {return {OP0(ImmVal), LAZY_L(addr),OP0(ImmVal), LAZY_H(addr)};}
		inline code_t brz (const Label& addr) {return {OP0(BranchZero), ADDR_HL(addr)};}
		inline code_t brc (const Label& addr) {return {OP0(BranchCF), ADDR_HL(addr)};}
		inline code_t jmp (const Label& addr) {return {OP0(Jump), ADDR_HL(addr)};}
		inline code_t call(const Label& addr) {return {OP0(Call), ADDR_HL(addr)};}
		inline code_t ret ()                  {return {OP0(Return)};}
		inline code_t halt()                  {return {OP0(Halt)};}
		inline code_t adj (offset_t offset)   {return {OP0(Adjust), GET_HL(offset)};}
		inline code_t pushSP()                {return {OP0(PushSP)};}
		inline code_t popSP ()                {return {OP0(PopSP)};}

		inline code_t shl(){return {OP1(Calc,fn,Calc::FN::SHL)};}
		inline code_t shr(){return {OP1(Calc,fn,Calc::FN::SHR)};}
		inline code_t rcl(){return {OP1(Calc,fn,Calc::FN::RCL)};}
		inline code_t rcr(){return {OP1(Calc,fn,Calc::FN::RCR)};}
		inline code_t add(){return {OP1(Calc,fn,Calc::FN::ADD)};}
		inline code_t sub(){return {OP1(Calc,fn,Calc::FN::SUB)};}
		inline code_t adc(){return {OP1(Calc,fn,Calc::FN::ADC)};}
		inline code_t suc(){return {OP1(Calc,fn,Calc::FN::SUC)};}

		inline code_t NOT(){return {OP1(Logic,fn,Logic::FN::NOT)};}
		inline code_t AND(){return {OP1(Logic,fn,Logic::FN::AND)};}
		inline code_t  OR(){return {OP1(Logic,fn,Logic::FN:: OR)};}
		inline code_t XOR(){return {OP1(Logic,fn,Logic::FN::XOR)};}

#undef OP0
#undef OP1
#undef OP2
#undef LAZY
#undef LAZY_H
#undef LAZY_L
#undef ADDR_HL
#undef ADDR_LH

		inline code_t push(Reg16 from)        {return {push(from.L()),push(from.H())};}
		inline code_t pop (Reg16 to)          {return {pop(to.H()),pop(to.L())};}
		inline code_t push(op_t v)            {return imm(v);}
		inline code_t push(const Label& v)    {return imm(v);}
		inline code_t push(code_t from)       {return from;}
		inline code_t pop (code_t to)         {return to;}
		inline code_t load(Reg16 addr, Reg value, offset_t offset=0)  {return {load(addr,offset), pop(value)};}
		inline code_t save(Reg16 addr, Reg value, offset_t offset=0)  {return {push(value), save(addr,offset)};}
		inline code_t load(Reg16 tmp, const Label& addr, offset_t offset=0) {return {imm(addr), pop(tmp), load(tmp,offset)};}
		inline code_t save(Reg16 tmp, const Label& addr, offset_t offset=0) {return {imm(addr), pop(tmp), save(tmp,offset)};}
		inline code_t load(Reg16 tmp, const Label& addr, Reg value, offset_t offset=0) {return {load(tmp,addr,offset), pop(value)};}
		inline code_t save(Reg16 tmp, const Label& addr, Reg value, offset_t offset=0) {return {push(value), save(tmp,addr,offset)};}
		inline code_t imm (Reg reg, op_t value)          {return {imm(value), pop(reg)};}
		inline code_t imm (Reg16 reg, const Label& addr) {return {imm(addr), pop(reg)};}
		inline code_t brz (const Label& addr, Reg reg)   {return {push(reg), brz(addr)};}

		inline code_t saveBP(Reg16 BP)            {return {push(BP),pushSP(),pop(BP)};}
		inline code_t loadBP(Reg16 BP)            {return {push(BP),popSP(),pop(BP)};}
		inline code_t ent (Reg16 BP, op_t size)   {return {saveBP(BP),adj(-size)};}
		inline code_t lev (Reg16 BP)              {return {loadBP(BP),ret()};}
		inline code_t load_local(Reg16 BP, offset_t offset)            {return load(BP,offset);}
		inline code_t save_local(Reg16 BP, offset_t offset)            {return save(BP,offset);}
		inline code_t load_local(Reg16 BP, offset_t offset, Reg to)    {return {load_local(BP,offset), pop(to)};}
		inline code_t save_local(Reg16 BP, offset_t offset, Reg value) {return {push(value), save_local(BP,offset)};}
#define ASM_BP Reg16::HL
#define ASM_TMP Reg16::FE
		inline code_t saveBP()                               {return saveBP(ASM_BP);}
		inline code_t loadBP()                               {return loadBP(ASM_BP);}
		inline code_t ent (op_t size)                        {return ent(ASM_BP,size);}
		inline code_t lev ()                                 {return lev(ASM_BP);}
		inline code_t load_local(offset_t offset)            {return load_local(ASM_BP,offset);}
		inline code_t save_local(offset_t offset)            {return save_local(ASM_BP,offset);}
		inline code_t load_local(offset_t offset, Reg to)    {return load_local(ASM_BP,offset,to);}
		inline code_t save_local(offset_t offset, Reg value) {return save_local(ASM_BP,offset,value);}
		inline code_t load(const Label& addr, offset_t offset=0) {return load(ASM_TMP,addr,offset);}
		inline code_t save(const Label& addr, offset_t offset=0) {return save(ASM_TMP,addr,offset);}
		inline code_t load(const Label& addr, Reg value, offset_t offset=0) {return load(ASM_TMP,addr,value,offset);}
		inline code_t save(const Label& addr, Reg value, offset_t offset=0) {return save(ASM_TMP,addr,value,offset);}
	}
	using namespace Ops;
}
#endif //BREADBOARDCPU_OPS_H
