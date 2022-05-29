//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_BASIC_H
#define BBCPU_BASIC_H

//#include "../cpu/regfile8x16/cpu.h"
#include "../cpu/regset_sram/opcode.h"
#include <memory>
#include <functional>
#include <utility>
#include <variant>
#include <map>


namespace BBCPU::ASM {
	using namespace OpCode::Ops;
	using op_t = uint8_t;
	using addr_t = uint16_t;
	using offset_t = int16_t;
	using lazy_t = std::function<op_t(addr_t)>;
	using ops_t = std::vector<op_t>;
	using data_t = std::vector<std::variant<op_t, lazy_t>>;
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

		explicit Label(addr_t addr=0) : addr(std::make_shared<addr_t>(addr)) {}
		explicit Label(std::string name,addr_t addr=0) : name(std::move(name)),addr(new addr_t(addr)) {}

		void set(addr_t v) const {
			*addr = v;
#if ASM_DEBUG
			std::cout << "set  "  << addr << "(" << name << ")=" << *addr << std::endl;
#endif
		}

		op_t get_byte(size_t i) const {
			return (*addr >> (i * 8)) & 0xff;
		}
		lazy_t get_lazy(size_t i) const {
			return [=](addr_t pc){return get_byte(i);};
		}
		addr_t& operator*() const{
			return *addr;
		}
		addr_t* operator->() const{
			return addr.get();
		}
	};
	using code_t = Util::flat_vector<std::variant<op_t, lazy_t, Label>>;
	
	inline static size_t data_size(code_t code){
		return std::count_if(code.begin(), code.end(), [](auto c){return std::get_if<Label>(&c)==nullptr;});
	}

	struct ASM {
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

		static ops_t parse(const code_t& codes){
			return ASM{}<<codes<<END;
		}
	};

}
#endif //BBCPU_BASIC_H
