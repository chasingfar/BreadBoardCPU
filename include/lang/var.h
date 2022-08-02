//
// Created by chasingfar on 2021/8/7.
//

#ifndef BBCPU_VAR_H
#define BBCPU_VAR_H
#include <cstddef>
#include <forward_list>
#include "asm/asm.h"

namespace BBCPU::Lang {
	using namespace ASM;
	using ASM::Reg;
	struct Value{
		virtual Code load() const=0;
		virtual ~Value()=default;
	};
	struct Raw:Value{
		data_t data{};
		Raw() = default;
		explicit Raw(data_t data):data(std::move(data)){}
		static auto make(const data_t& data){ return std::make_shared<Raw>(data);}
		Code load() const override{
			Code code{};
			for(auto v:data){
				code<<std::visit(Util::lambda_compose{
					[](const lazy_t &fn) { return imm(fn); },
					[](         op_t op) { return imm(op); },
				}, v);
			}
			return code;
		}
		std::shared_ptr<Raw> shift(offset_t shift_offset,addr_t new_size) const{
			data_t tmp(new_size);
			std::copy_n(data.begin()+shift_offset,new_size,tmp.begin());
			return make(tmp);
		}
	};
	template<typename T>
	concept CanAsRaw = requires(T x) {
		{ x.as_raw() }->std::same_as<std::shared_ptr<Raw>>;
	};
	struct Expr:Value{
		Code code{};
		Expr() = default;
		explicit Expr(Code  value):code(std::move(value)){}
		explicit Expr(const Value& value):code(value.load()){}
		Code load() const override{
			return code;
		}
		Expr& operator <<(Code c){
			code<<std::move(c);
			return *this;
		}
	};
	inline auto expr(const Code& code){return std::make_shared<Expr>(code);}
	struct Var:Value{
		virtual Code save() const=0;
	};
	struct RegVar:Var{
		Reg reg;
		explicit RegVar(Reg reg):reg(reg){}
		static auto make(Reg reg){ return std::make_shared<RegVar>(reg);}
		Code load() const override{
			return push(reg);
		}
		Code save() const override{
			return pop(reg);
		}
	};
	struct MemVar:Var{
		addr_t size;
		explicit MemVar(addr_t size):size(size){}
		virtual Code load(offset_t index) const=0;
		virtual Code save(offset_t index) const=0;
		virtual std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const=0;
		Code load() const override{
			Code tmp{};
			for (addr_t i=0; i<size; ++i) {
				tmp<<load(i);
			}
			return tmp;
		}
		Code save() const override{
			Code tmp{};
			for (addr_t i=0; i<size; ++i) {
				tmp<<save(size-1-i);
			}
			return tmp;
		}
	};
	struct LocalVar:MemVar{
		offset_t offset;
		explicit LocalVar(addr_t size,offset_t offset):MemVar(size),offset(offset){}
		static auto make(addr_t size,offset_t offset){ return std::make_shared<LocalVar>(size,offset);}
		Code load(offset_t index) const override{
			return load_local(offset-index);
		}
		Code save(offset_t index) const override{
			return save_local(offset-index);
		}
		std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const override{
			return make(new_size,offset - shift_offset);
		}
	};
	struct StaticVar:MemVar{
		Label label;
		offset_t offset;
		StaticVar(addr_t size,Label label,offset_t offset):MemVar(size),label(std::move(label)),offset(offset){}
		static auto make(addr_t size,const Label& label,offset_t offset){ return std::make_shared<StaticVar>(size,label,offset);}
		Code load(offset_t index) const override{
			return Ops::load(label,offset+index);
		}
		Code save(offset_t index) const override{
			return Ops::save(label,offset+index);
		}
		std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const override{
			return make(new_size,label,offset + shift_offset);
		}
	};
	struct PtrVar:MemVar{
		Code ptr;
		offset_t offset;
		explicit PtrVar(addr_t size,Code ptr,offset_t offset=0):MemVar(size),ptr(std::move(ptr)),offset(offset){}
		static auto make(addr_t size,const Code& ptr,offset_t offset=0){ return std::make_shared<PtrVar>(size,ptr,offset);}
		Code load(offset_t index) const override{
			return {ptr,Ops::load(offset+index)};
		}
		Code save(offset_t index) const override{
			return {ptr,Ops::save(offset+index)};
		}
		std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const override{
			return make(new_size,ptr,offset + shift_offset);
		}
	};
	struct Allocator{
		template<typename Type,typename ...Rest>
		auto _vars() {
			Type var{alloc(Type::size)};
			if constexpr (sizeof...(Rest)==0){
				return std::tuple{var};
			}else{
				return std::tuple_cat(std::tuple{var}, _vars<Rest...>());
			}
		}
		template<typename ...Types>
		std::tuple<Types...> vars() {
			if constexpr (sizeof...(Types)==0){
				return std::tuple{};
			}else{
				return _vars<Types...>();
			}
		}
		virtual std::shared_ptr<MemVar> alloc(addr_t size)=0;
	};
}
#endif //BBCPU_VAR_H
