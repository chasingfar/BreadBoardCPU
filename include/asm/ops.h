//
// Created by chasingfar on 2021/8/7.
//

#ifndef BBCPU_OPS_H
#define BBCPU_OPS_H

#include "basic.h"
namespace BBCPU::ASM {

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
		inline Code load(Reg16 addr, offset_t offset=0)  {return {OP1(Load, from, addr),GET_HL(offset)};}
		inline Code save(Reg16 addr, offset_t offset=0)  {return {OP1(Save, to, addr),GET_HL(offset)};}
		inline Code push(Reg fromReg)       {return {OP1(Push, from, fromReg)};}
		inline Code pop (Reg toReg)         {return {OP1(Pop, to, toReg)};}
		inline Code imm (op_t value)        {return {OP0(ImmVal), value};}
		inline Code imm (lazy_t value)      {return {OP0(ImmVal), value};}
		inline Code imm (const Label& addr) {return {OP0(ImmVal), LAZY_L(addr),OP0(ImmVal), LAZY_H(addr)};}
		inline Code brz (const Label& addr) {return {OP0(BranchZero), ADDR_HL(addr)};}
		inline Code brc (const Label& addr) {return {OP0(BranchCF), ADDR_HL(addr)};}
		inline Code jmp (const Label& addr) {return {OP0(Jump), ADDR_HL(addr)};}
		inline Code call(const Label& addr) {return {OP0(Call), ADDR_HL(addr)};}
		inline Code ret ()                  {return {OP0(Return)};}
		inline Code halt()                  {return {OP0(Halt)};}
		inline Code adj (offset_t offset)   {return {OP0(Adjust), GET_HL(offset)};}
		inline Code pushSP()                {return {OP0(PushSP)};}
		inline Code popSP ()                {return {OP0(PopSP)};}

		inline Code shl(){return {OP1(Calc,fn,Calc::FN::SHL)};}
		inline Code shr(){return {OP1(Calc,fn,Calc::FN::SHR)};}
		inline Code rcl(){return {OP1(Calc,fn,Calc::FN::RCL)};}
		inline Code rcr(){return {OP1(Calc,fn,Calc::FN::RCR)};}
		inline Code add(){return {OP1(Calc,fn,Calc::FN::ADD)};}
		inline Code sub(){return {OP1(Calc,fn,Calc::FN::SUB)};}
		inline Code adc(){return {OP1(Calc,fn,Calc::FN::ADC)};}
		inline Code suc(){return {OP1(Calc,fn,Calc::FN::SUC)};}

		inline Code NOT(){return {OP1(Logic,fn,Logic::FN::NOT)};}
		inline Code AND(){return {OP1(Logic,fn,Logic::FN::AND)};}
		inline Code  OR(){return {OP1(Logic,fn,Logic::FN:: OR)};}
		inline Code XOR(){return {OP1(Logic,fn,Logic::FN::XOR)};}

#undef OP0
#undef OP1
#undef OP2
#undef LAZY
#undef LAZY_H
#undef LAZY_L
#undef ADDR_HL
#undef ADDR_LH

		inline Code push(Reg16 from)        {return {push(from.L()),push(from.H())};}
		inline Code pop (Reg16 to)          {return {pop(to.H()),pop(to.L())};}
		inline Code push(op_t v)            {return imm(v);}
		inline Code push(const Label& v)    {return imm(v);}
		inline Code push(Code from)         {return from;}
		inline Code pop (Code to)           {return to;}
		inline Code load(Reg16 addr, Reg value, offset_t offset=0)  {return {load(addr,offset), pop(value)};}
		inline Code save(Reg16 addr, Reg value, offset_t offset=0)  {return {push(value), save(addr,offset)};}
		inline Code load(Reg16 tmp, const Label& addr, offset_t offset=0) {return {imm(addr), pop(tmp), load(tmp,offset)};}
		inline Code save(Reg16 tmp, const Label& addr, offset_t offset=0) {return {imm(addr), pop(tmp), save(tmp,offset)};}
		inline Code load(Reg16 tmp, const Label& addr, Reg value, offset_t offset=0) {return {load(tmp,addr,offset), pop(value)};}
		inline Code save(Reg16 tmp, const Label& addr, Reg value, offset_t offset=0) {return {push(value), save(tmp,addr,offset)};}
		inline Code imm (Reg reg, op_t value)          {return {imm(value), pop(reg)};}
		inline Code imm (Reg16 reg, const Label& addr) {return {imm(addr), pop(reg)};}
		inline Code brz (const Label& addr, Reg reg)   {return {push(reg), brz(addr)};}

		inline Code saveBP(Reg16 BP)            {return {push(BP),pushSP(),pop(BP)};}
		inline Code loadBP(Reg16 BP)            {return {push(BP),popSP(),pop(BP)};}
		inline Code ent (Reg16 BP, op_t size)   {return {saveBP(BP),adj(-size)};}
		inline Code lev (Reg16 BP)              {return {loadBP(BP),ret()};}
		inline Code load_local(Reg16 BP, offset_t offset)            {return load(BP,offset);}
		inline Code save_local(Reg16 BP, offset_t offset)            {return save(BP,offset);}
		inline Code load_local(Reg16 BP, offset_t offset, Reg to)    {return {load_local(BP,offset), pop(to)};}
		inline Code save_local(Reg16 BP, offset_t offset, Reg value) {return {push(value), save_local(BP,offset)};}
#define ASM_BP Reg16::HL
#define ASM_PTR Reg16::FE
		inline Code saveBP()                               {return saveBP(ASM_BP);}
		inline Code loadBP()                               {return loadBP(ASM_BP);}
		inline Code ent (op_t size)                        {return ent(ASM_BP,size);}
		inline Code lev ()                                 {return lev(ASM_BP);}
		inline Code load_local(offset_t offset)            {return load_local(ASM_BP,offset);}
		inline Code save_local(offset_t offset)            {return save_local(ASM_BP,offset);}
		inline Code load_local(offset_t offset, Reg to)    {return load_local(ASM_BP,offset,to);}
		inline Code save_local(offset_t offset, Reg value) {return save_local(ASM_BP,offset,value);}
		inline Code load(offset_t offset=0)                    {return {pop(ASM_PTR),load(ASM_PTR, offset)};}
		inline Code save(offset_t offset=0)                    {return {pop(ASM_PTR),save(ASM_PTR, offset)};}
		inline Code load(const Label& addr, offset_t offset=0) {return load(ASM_PTR, addr, offset);}
		inline Code save(const Label& addr, offset_t offset=0) {return save(ASM_PTR, addr, offset);}
		inline Code load(const Label& addr, Reg value, offset_t offset=0) {return load(ASM_PTR, addr, value, offset);}
		inline Code save(const Label& addr, Reg value, offset_t offset=0) {return save(ASM_PTR, addr, value, offset);}
	}
	using namespace Ops;
}
#endif //BBCPU_OPS_H
