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
	using offset_t = int16_t;
	using lazy_t = std::function<op_t(addr_t)>;
	using ops_t = std::vector<op_t>;
	using Reg=Regs::UReg;
	using Reg16=Regs::UReg16;

	inline std::ostream &operator<<(std::ostream &os, const ops_t& ops) {
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
#if ASM_DEBUG
			std::cout << "set  "  << addr << "(" << name << ")=" << *addr << std::endl;
#endif
		}

		op_t getByte(size_t i) const {
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


		inline code_t push(Reg16 from)        {return {push(from.L()),push(from.H())};}
		inline code_t pop (Reg16 to)          {return {pop(to.H()),pop(to.L())};}
		inline code_t push(op_t v)            {return imm(v);}
		inline code_t push(const Label& v)    {return imm(v);}
		inline code_t load(Reg16 addr, Reg value, offset_t offset=0)  {return {load(addr,offset), pop(value)};}
		inline code_t save(Reg16 addr, Reg value, offset_t offset=0)  {return {push(value), save(addr,offset)};}
		inline code_t load(Reg16 tmp, const Label& addr, offset_t offset=0) {return {imm(addr), pop(tmp), load(tmp,offset)};}
		inline code_t save(Reg16 tmp, const Label& addr, offset_t offset=0) {return {imm(addr), pop(tmp), save(tmp,offset)};}
		inline code_t load(Reg16 tmp, const Label& addr, Reg value, offset_t offset=0) {return {load(tmp,addr,offset), pop(value)};}
		inline code_t save(Reg16 tmp, const Label& addr, Reg value, offset_t offset=0) {return {push(value), save(tmp,addr,offset)};}
		inline code_t imm (Reg reg, op_t value)          {return {imm(value), pop(reg)};}
		inline code_t imm (Reg16 reg, const Label& addr) {return {imm(addr), pop(reg)};}
		inline code_t brz (const Label& addr, Reg reg)   {return {push(reg), brz(addr)};}

		inline code_t ent (Reg16 BP, op_t size)   {return {push(BP),pushSP(),pop(BP),adj(-size)};}
		inline code_t lev (Reg16 BP)              {return {push(BP),popSP(),pop(BP),ret()};}
		inline code_t load_local(Reg16 BP, offset_t offset)            {return load(BP,offset);}
		inline code_t save_local(Reg16 BP, offset_t offset)            {return save(BP,offset);}
		inline code_t load_local(Reg16 BP, offset_t offset, Reg to)    {return {load_local(BP,offset), pop(to)};}
		inline code_t save_local(Reg16 BP, offset_t offset, Reg value) {return {push(value), save_local(BP,offset)};}
#define ASM_BP Reg16::HL
#define ASM_TMP Reg16::FE
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

	struct Var{
		code_t push;
		code_t pop;
		code_t load(Reg to) const{
			return {push,Ops::pop(to)};
		}
		code_t save(Reg value) const{
			return {Ops::push(value),pop};
		}
	};

	template <typename T,typename ...Ts>
	using Convertible=std::enable_if_t<std::disjunction_v<std::is_convertible<T,Ts>...>, bool>;
	template <typename T>
	using Pushable=Convertible<T,Var,Reg,int8_t>;
	template <typename T>
	using Popable=Convertible<T,Var,Reg>;

	namespace Ops {
		inline code_t push(const Var& from) {return from.push;}
		inline code_t pop (const Var& to)   {return to.pop;}

		template<typename V,typename Res,Pushable<V> = true,Popable<Res> = true>
		inline code_t set(Res res,V value) {return {push(value),pop(res)};}

#define DEFINE_0(type, name, _FN)                   \
        inline code_t name(){                       \
            return {OP1(type,fn,type::FN::_FN)};    \
        }
#define DEFINE_1(type, name, FN)                    \
        DEFINE_0(type,name,FN)                      \
        template<typename L,Pushable<L> = true>     \
        inline code_t name(L lhs) {                 \
            return {push(lhs),name()};              \
        }                                           \
        template<typename L,typename Res,           \
            Pushable<L> = true,Popable<Res> = true> \
        inline code_t name(Res res, L lhs) {        \
            return {name(lhs),pop(res)};            \
        }
#define DEFINE_2(type, name, FN)                    \
        DEFINE_0(type,name,FN)                      \
        template<typename L,typename R,             \
            Pushable<L> = true,Pushable<R> = true>  \
        inline code_t name(L lhs, R rhs) {          \
            return {push(rhs),push(lhs),name()};    \
        }                                           \
        template<typename L,typename R,typename Res,\
            Pushable<L> = true,Pushable<R> = true,  \
            Popable<Res> = true>                    \
        inline code_t name(Res res, L lhs, R rhs) { \
            return {name(lhs,rhs),pop(res)};        \
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
