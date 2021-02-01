//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_BASIC_H
#define BREADBOARDCPU_BASIC_H

#include "../cpu/cpu.h"
#include <memory>
#include <functional>
#include <variant>
#include <map>


namespace BreadBoardCPU::ASM {
	using namespace OpCode::Ops;
	using op_t = uint8_t;
	using addr_t = uint16_t;
	using lazy_t = std::function<op_t(addr_t)>;
	using code_t = Util::flat_vector<std::variant<op_t, lazy_t>>;
	using ops_t = std::vector<op_t>;

	std::ostream &operator<<(std::ostream &os, ops_t ops) {
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
		addr_t addr = 0;
		std::string name;
		Label() = default;

		Label(addr_t addr) : addr(addr) {}

		Label(std::string name) : name(name) {}

		void set(addr_t v) {
			addr = v;
			std::cout << name << "=" << v << std::endl;
		}

		op_t getByte(size_t i) {
			return (addr >> (i * 8)) & 0xff;
		}
	};

#define OP0(op) op::id::setAs<MARG::opcode,op_t>()
#define OP1(op, n1, v1) op::n1::setAs<MARG::opcode>(OP0(op),v1)
#define OP2(op, n1, v1, n2, v2) op::n2::setAs<MARG::opcode>(OP1(op,n1,v1),v2)
#define LAZY(fn) [&](addr_t pc){return fn;}
#define ADDR_HL(addr) LAZY(addr.getByte(1)),LAZY(addr.getByte(0))
#define ADDR_LH(addr) LAZY(addr.getByte(0)),LAZY(addr.getByte(1))
	enum struct Reg : op_t {
		A, B,
		C, D,
		E, F,
		L, H,
	};
	enum struct Reg16 : op_t {
		IMM, TMP,
		SP, PC,
		BA, DC,
		FE, HL,
	};

	struct ASM {
		code_t codes;
		static struct end_t {
		} END;

		size_t size() {
			return codes.size();
		}

		ops_t resolve() {
			ops_t ops;
			ops.reserve(codes.size());
			for (auto &code:codes) {
				ops.emplace_back(std::visit(Util::lambda_compose{
						[&](const lazy_t &fn) { return fn(ops.size()); },
						[&](op_t op) { return op; },
				}, code));
			}
			return ops;
		}

		ASM &operator<<(code_t code) {
			codes.insert(codes.end(), code.begin(), code.end());
			return *this;
		}

		ASM &operator>>(Label &label) {
			label.set(size());
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

		code_t push(Reg fromReg) {
			return {OP1(Push, from, fromReg)};
		}

		code_t pop(Reg toReg) {
			return {OP1(Pop, to, toReg)};
		}

		code_t load(Label &addr) {
			return {OP1(Load, from, Reg16::IMM), ADDR_HL(addr)};
		}

		code_t load(Reg16 addr) {
			return {OP1(Load, from, addr)};
		}

		code_t load() {//address from stack
			return load(Reg16::TMP);
		}

		code_t load(Label &addr, Reg value) {
			return {load(addr), pop(value)};
		}

		code_t load(Reg16 addr, Reg value) {
			return {load(addr), pop(value)};
		}

		code_t save(Label &addr) {
			return {OP1(Save, to, Reg16::IMM), ADDR_HL(addr)};
		}

		code_t save(Reg16 addr) {
			return {OP1(Save, to, addr)};
		}

		code_t save() {//address from stack
			return save(Reg16::TMP);
		}

		code_t save(Label &addr, Reg value) {
			return {push(value), save(addr)};
		}

		code_t save(Reg16 addr, Reg value) {
			return {push(value), save(addr)};
		}

		code_t imm(op_t value) {
			return {OP0(ImmVal), value};
		}

		code_t imm(Reg reg, op_t value) {
			return {imm(value), pop(reg)};
		}

		code_t push(op_t v) {
			return imm(v);
		}

		code_t brz(Label &addr) {
			return {OP0(BranchZero), ADDR_HL(addr)};
		}

		code_t brz(Label &addr, Reg reg) {
			return {push(reg), brz(addr)};
		}

		code_t brc(Label &addr) {
			return {OP0(BranchCF), ADDR_HL(addr)};
		}

		code_t jmp(Label &addr) {
			return {OP0(Jump), ADDR_HL(addr)};
		}

		code_t call(Label &addr) {
			return {OP0(Call), ADDR_HL(addr)};
		}

		code_t ret() {
			return {OP0(Return)};
		}

		code_t halt() {
			return {OP0(Halt)};
		}

		code_t ent(op_t size) {
			return {OP0(Enter), size};
		}

		code_t adj(op_t size) {
			return {OP0(Adjust), size};
		}

		code_t lev() {
			return {OP0(Leave)};
		}

		code_t lea(op_t offset) {
			return {OP0(Local), offset};
		}

		code_t load_local(op_t offset) {
			return {lea(offset), load()};
		}

		code_t load_local(op_t offset, Reg to) {
			return {load_local(offset), pop(to)};
		}

		code_t save_local(op_t offset) {
			return {lea(offset), save()};
		}

		code_t save_local(op_t offset, Reg value) {
			return {push(value), save_local(offset)};
		}

#define DEFINE_0(type, name, FN)          \
        code_t name(){                          \
            return {OP1(type,fn,type::fn::FN)}; \
        }
#define DEFINE_1(type, name, FN)          \
        DEFINE_0(type,name,FN)                  \
        code_t name(Reg res, auto lhs) {        \
            return {push(lhs),name(),pop(res)}; \
        }
#define DEFINE_2(type, name, FN)                    \
        DEFINE_0(type,name,FN)                            \
        code_t name(Reg res, auto lhs, auto rhs) {        \
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

		DEFINE_2(Logic, OR, OR)

		DEFINE_2(Logic, XOR, XOR)

#undef DEFINE_0
#undef DEFINE_1
#undef DEFINE_2
#undef OP0
#undef OP1
#undef OP2
#undef LAZY
#undef ADDR_HL
#undef ADDR_LH
	}
	using namespace Ops;
}
#endif //BREADBOARDCPU_BASIC_H
