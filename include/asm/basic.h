//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_BASIC_H
#define BBCPU_BASIC_H

#include "cpu/cpu.h"
#include <memory>
#include <functional>
#include <utility>
#include <variant>
#include <map>


namespace BBCPU::ASM {
	using namespace OpCode::Ops;
	using Reg=OpCode::UReg;
	using Reg16=OpCode::UReg16;
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
		using val_type=std::variant<op_t, lazy_t, Label>;
		using arg_type=std::variant<op_t, lazy_t, Label, Code, data_t, ops_t>;
		std::vector<val_type> codes;

		static auto flatten(const arg_type& code){
			return std::visit(Util::lambda_compose{
				[](const auto&  other){ return std::vector<val_type>{other}; },
				[](const Code&    sub){ return sub.codes;},
				[](const data_t& data){
					std::vector<val_type> tmp;
					tmp.reserve(data.size());
					for(auto d:data){
						std::visit([&](const auto& v){tmp.template emplace_back(v);}, d);
					}
					return tmp;
				},
				[](const ops_t&   ops){
					std::vector<val_type> tmp;
					tmp.insert(tmp.end(),ops.begin(),ops.end());
					return tmp;
				},
			}, code);
		}
		static auto flatten(std::initializer_list<arg_type> codes){
			std::vector<val_type> res;
			for (const auto &code:codes) {
				auto tmp=flatten(code);
				res.insert(res.end(),tmp.begin(),tmp.end());
			}
			return res;
		}

		Code(std::initializer_list<arg_type> codes):codes{flatten(codes)}{}
		Code(const CanToCode auto& code):codes{code.to_code().codes}{}
		Code(int v):codes{static_cast<op_t>(v)}{}

		Code& operator <<(const arg_type& code){
			auto tmp=flatten(code);
			codes.insert(codes.end(),tmp.begin(),tmp.end());
			return *this;
		}
		static addr_t count(const val_type& code){
			return std::visit(Util::lambda_compose{
				[](    const lazy_t &fn)->addr_t { return 1; },
				[](             op_t op)->addr_t { return 1; },
				[](  const Label& label)->addr_t { return 0; },
			}, code);
		}
		addr_t size() const{
			addr_t sum=0;
			for (const auto &code:codes) {
				sum+=count(code);
			}
			return sum;
		}
		Code sub(addr_t offset,addr_t sub_size) const{
			Code tmp{};
			tmp.codes.reserve(sub_size);
			std::copy_n(codes.begin()+offset,sub_size,std::back_inserter(tmp.codes));
			return tmp;
		}
		data_t resolve(addr_t start=0) const{
			data_t data{};
			data.reserve(size());
			for (const auto &code:codes) {
				std::visit(Util::lambda_compose{
					[&](const auto&  fn)    { data.emplace_back(fn); },
					[&](const Label& label) { label.set(start+data.size()); },
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
	struct DataBlock{
		Label start;
		data_t body{};
		Label end;

		Code to_code() const{
			return {start,body,end};
		}
	};
}
#endif //BBCPU_BASIC_H
