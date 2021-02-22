//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_BASIC_H
#define BREADBOARDCPU_BASIC_H

#include "../cpu/cpu.h"
#include <memory>
#include <functional>
#include <utility>
#include <variant>
#include <map>


namespace BreadBoardCPU::ASM {
	using namespace OpCode::Ops;
	using op_t = uint8_t;
	using addr_t = uint16_t;
	using lazy_t = std::function<op_t(addr_t)>;
	using ops_t = std::vector<op_t>;
	using Reg=Regs::UReg;
	using Reg16=Regs::UReg16;

	std::ostream &operator<<(std::ostream &os, const ops_t& ops) {
		std::string name;
		size_t size = 0, i = 0;
		for (auto op:ops) {
			os << i << " : " << std::bitset<8>(op) << " ; ";
			if (size == 0) {
				std::tie(name, size) = all::parse(op);
				os << name;
			} else {
				os << (int) *reinterpret_cast<int8_t *>(&op);
			}
			os << std::endl;
			--size;
			++i;
		}
		return os;
	}

	struct Label {
		std::shared_ptr<addr_t> addr;
		std::string name;
		explicit Label(addr_t addr=0) : addr(new addr_t(addr)) {}

		explicit Label(std::string name,addr_t addr=0) : name(std::move(name)),addr(new addr_t(addr)) {}

		void set(addr_t v) const {
			*addr = v;
			std::cout << "set  "  << addr << "(" << name << ")=" << *addr << std::endl;
		}

		op_t getByte(size_t i) const {
			//std::cout << "read " << addr << "(" << name << ")=" << *addr << std::endl;
			return (*addr >> (i * 8)) & 0xff;
		}
		addr_t& operator*() const{
			return *addr;
		}
		addr_t* operator->() const{
			return addr.get();
		}
	};
	using code_t = Util::flat_vector<std::variant<op_t, lazy_t, Label>>;

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

	struct ASM {
		using data_t = std::vector<std::variant<op_t, lazy_t>>;
		data_t data;
		static struct end_t {} END;

		size_t pc() const {
			return data.size();
		}

		ops_t resolve() {
			ops_t ops;
			ops.reserve(pc());
			for (auto &code:data) {
				ops.emplace_back(std::visit(Util::lambda_compose{
						[&](const lazy_t &fn) { return fn(ops.size()); },
						[&](op_t op) { return op; },
				}, code));
			}
			return ops;
		}

		ASM &operator<<(const code_t& codes) {
			for (auto &code:codes) {
				std::visit(Util::lambda_compose{
						[&](const lazy_t &fn) { data.emplace_back(fn); },
						[&](op_t op) {  data.emplace_back(op); },
						[&](const Label& label) { label.set(pc()); },
				}, code);
			}
			return *this;
		}

		ASM &operator>>(const Label& label) {
			label.set(pc());
			return *this;
		}

		ops_t operator<<(end_t) {
			return resolve();
		}

		friend std::ostream &operator<<(std::ostream &os, ASM asm_) {
			for (auto code:asm_.resolve()) {
				os << std::bitset<8>(code) << std::endl;
			}
			return os;
		}
	};
	namespace Ops {
		code_t load(Reg16 addr,int16_t offset=0)  {return {OP1(Load, from, addr),GET_HL(offset)};}
		code_t save(Reg16 addr,int16_t offset=0)  {return {OP1(Save, to, addr),GET_HL(offset)};}
		code_t push(Reg fromReg)       {return {OP1(Push, from, fromReg)};}
		code_t pop (Reg toReg)         {return {OP1(Pop, to, toReg)};}
		code_t imm (op_t value)        {return {OP0(ImmVal), value};}
		code_t imm (const Label& addr) {return {OP0(ImmVal), LAZY_L(addr),OP0(ImmVal), LAZY_H(addr)};}
		code_t brz (const Label& addr) {return {OP0(BranchZero), ADDR_HL(addr)};}
		code_t brc (const Label& addr) {return {OP0(BranchCF), ADDR_HL(addr)};}
		code_t jmp (const Label& addr) {return {OP0(Jump), ADDR_HL(addr)};}
		code_t call(const Label& addr) {return {OP0(Call), ADDR_HL(addr)};}
		code_t ret ()                  {return {OP0(Return)};}
		code_t halt()                  {return {OP0(Halt)};}
		code_t adj (int16_t offset)    {return {OP0(Adjust), GET_HL(offset)};}
		code_t pushSP()                {return {OP0(PushSP)};}
		code_t popSP ()                {return {OP0(PopSP)};}


		code_t push(Reg16 from)        {return {push(from.L()),push(from.H())};}
		code_t pop (Reg16 to)          {return {pop(to.H()),pop(to.L())};}
		code_t push(op_t v)            {return imm(v);}
		code_t push(const Label& v)    {return imm(v);}
		code_t load(Reg16 addr, Reg value,int16_t offset=0)  {return {load(addr,offset), pop(value)};}
		code_t save(Reg16 addr, Reg value,int16_t offset=0)  {return {push(value), save(addr,offset)};}
		code_t load(Reg16 tmp, const Label &addr,int16_t offset=0) {return {imm(addr), pop(tmp), load(tmp,offset)};}
		code_t save(Reg16 tmp, const Label &addr,int16_t offset=0) {return {imm(addr), pop(tmp), save(tmp,offset)};}
		code_t load(Reg16 tmp, const Label& addr, Reg value,int16_t offset=0) {return {load(tmp,addr,offset), pop(value)};}
		code_t save(Reg16 tmp, const Label& addr, Reg value,int16_t offset=0) {return {push(value), save(tmp,addr,offset)};}
		code_t imm (Reg reg, op_t value)    {return {imm(value), pop(reg)};}
		code_t imm (Reg16 reg, const Label& addr)    {return {imm(addr), pop(reg)};}
		code_t brz (const Label& addr, Reg reg)   {return {push(reg), brz(addr)};}

		code_t ent (Reg16 BP,op_t size)   {return {push(BP),pushSP(),pop(BP),adj(-size)};}
		code_t lev (Reg16 BP)             {return {push(BP),popSP(),pop(BP),ret()};}
		code_t load_local(Reg16 BP,op_t offset)            {return load(BP,offset);}
		code_t load_local(Reg16 BP,op_t offset, Reg to)    {return {load_local(BP,offset), pop(to)};}
		code_t save_local(Reg16 BP,op_t offset)            {return save(BP,offset);}
		code_t save_local(Reg16 BP,op_t offset, Reg value) {return {push(value), save_local(BP,offset)};}
		code_t ent (op_t size)                    {return ent(Reg16::HL,size);}
		code_t lev ()                             {return lev(Reg16::HL);}
		code_t load_local(op_t offset)            {return load_local(Reg16::HL,offset);}
		code_t load_local(op_t offset, Reg to)    {return load_local(Reg16::HL,offset,to);}
		code_t save_local(op_t offset)            {return save_local(Reg16::HL,offset);}
		code_t save_local(op_t offset, Reg value) {return save_local(Reg16::HL,offset,value);}
		code_t load(const Label& addr, Reg value,int16_t offset=0) {return load(Reg16::FE,addr,value,offset);}
		code_t save(const Label& addr, Reg value,int16_t offset=0) {return save(Reg16::FE,addr,value,offset);}

#define DEFINE_0(type, name, _FN)                \
        code_t name(){                          \
            return {OP1(type,fn,type::FN::_FN)}; \
        }
#define DEFINE_1(type, name, FN)                \
        DEFINE_0(type,name,FN)                  \
        code_t name(Reg res, Reg lhs) {         \
            return {push(lhs),name(),pop(res)}; \
        }                                       \
        code_t name(Reg res, int8_t lhs) {        \
            return {push(lhs),name(),pop(res)}; \
        }
#define DEFINE_2(type, name, FN)                          \
        DEFINE_0(type,name,FN)                            \
        code_t name(Reg res, Reg lhs, Reg rhs) {          \
            return {push(rhs),push(lhs),name(),pop(res)}; \
        }                                                 \
        code_t name(Reg res, Reg lhs, int8_t rhs) {       \
            return {push(rhs),push(lhs),name(),pop(res)}; \
        }                                                 \
        code_t name(Reg res, int8_t lhs, Reg rhs) {       \
            return {push(rhs),push(lhs),name(),pop(res)}; \
        }

		DEFINE_1(Calc, shl, SHL)
		DEFINE_1(Calc, shr, SHR)
		DEFINE_1(Calc, rcl, RCL)
		DEFINE_1(Calc, rcr, RCR)
		DEFINE_2(Calc, add, ADD)
		DEFINE_2(Calc, sub, SUB)
		DEFINE_2(Calc, adc, ADC)
		DEFINE_2(Calc, suc, SUC)

		DEFINE_1(Logic, NOT, NOT)
		DEFINE_2(Logic, AND, AND)
		DEFINE_2(Logic, OR , OR )
		DEFINE_2(Logic, XOR, XOR)

#undef DEFINE_0
#undef DEFINE_1
#undef DEFINE_2
#undef OP0
#undef OP1
#undef OP2
#undef LAZY
#undef LAZY_H
#undef LAZY_L
#undef ADDR_HL
#undef ADDR_LH
	}
	using namespace Ops;
}
#endif //BREADBOARDCPU_BASIC_H
