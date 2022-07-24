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
	using namespace RegSet_SRAM::OpCode::Ops;
	using RegSet_SRAM::MARG;
	using RegSet_SRAM::MCTRL;
	using Reg=RegSet_SRAM::Regs::UReg;
	using Reg16=RegSet_SRAM::Regs::UReg16;
	using op_t = uint8_t;
	using addr_t = uint16_t;
	using offset_t = int16_t;
	using lazy_t = std::function<op_t(addr_t)>;
	using ops_t = std::vector<op_t>;
	using data_t = std::vector<std::variant<op_t, lazy_t>>;

	inline std::ostream &operator<<(std::ostream &os, const ops_t& ops) {
		size_t i = 0;
		for(auto it=ops.begin();it!=ops.end();){
			auto before=it;
			auto name = all::parse_iter(it,ops.end());
			os << i << " : ";
			for(;before!=it;++before,++i){
				os<<std::bitset<8>(*before)<<" ";
			}
			os<<"; "<<name<<std::endl;
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
	template<typename T>
	concept CanToCode=requires (T x){x.to_code();};
	struct Code{
		using type=std::variant<op_t, lazy_t, Label, Code>;
		std::vector<type> codes;
		Code(std::initializer_list<type> codes):codes{codes}{}
		Code(const CanToCode auto& code):codes{code.to_code()}{}
		Code& operator <<(const type& code){
			codes.push_back(code);
			return *this;
		}
		addr_t size() const{
			addr_t sum=0;
			for (const auto &code:codes) {
				sum+=std::visit(Util::lambda_compose{
					[](    const lazy_t &fn)->addr_t { return 1; },
					[](             op_t op)->addr_t { return 1; },
					[](  const Label& label)->addr_t { return 0; },
					[](const Code& sub_code)->addr_t { return sub_code.size();},
				}, code);
			}
			return sum;
		}
		data_t resolve(addr_t start=0) const{
			data_t data{};
			data.reserve(size());
			for (const auto &code:codes) {
				std::visit(Util::lambda_compose{
					[&](const lazy_t &fn)   { data.emplace_back(fn); },
					[&](op_t op)            { data.emplace_back(op); },
					[&](const Label& label) { label.set(start+data.size()); },
					[&](const Code& sub_code) {
						data_t sub_data=sub_code.resolve(start+data.size());
						data.insert(data.end(),sub_data.begin(),sub_data.end());
					},
				}, code);
			}
			return data;
		}
		static ops_t assemble(data_t data){
			ops_t ops;
			ops.reserve(data.size());
			for (auto &code:data) {
				ops.emplace_back(std::visit(Util::lambda_compose{
					[&](const lazy_t &fn) { return fn(ops.size()); },
					[&](op_t op) { return op; },
				}, code));
			}
			return ops;
		}
		ops_t assemble(addr_t start=0) const{
			return assemble(resolve(start));
		}
	};
	struct CodeBlock{
		Label start;
		Code body{};
		Label end;

		Code to_code() const{
			return {start,body,end};
		}
	};
}
#endif //BBCPU_BASIC_H
