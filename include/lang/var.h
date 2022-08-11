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
			return std::accumulate(data.rbegin(),data.rend(),
			Code{},[](auto code,auto val){
				return code<<std::visit([](const auto &v) { return imm(v); }, val);
			});
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
		offset_t offset;
		explicit MemVar(addr_t size,offset_t offset):size(size),offset(offset){}
		virtual Code load(offset_t index,bool is_first=true) const=0;
		virtual Code save(offset_t index,bool is_first=true) const=0;
		virtual Code get_ref() const=0;
		virtual std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const=0;
		Code load() const override{
			Code tmp{};
			for (addr_t i=0; i<size; ++i) {
				tmp<<load(size-1-i,i==0);
			}
			return tmp;
		}
		Code save() const override{
			Code tmp{};
			for (addr_t i=0; i<size; ++i) {
				tmp<<save(i,i==0);
			}
			return tmp;
		}
	};
	struct LocalVar:MemVar{
		explicit LocalVar(addr_t size,offset_t offset):MemVar(size,offset){}
		static auto make(addr_t size,offset_t offset){ return std::make_shared<LocalVar>(size,offset);}
		Code load(offset_t index,bool is_first=true) const override{
			return load_local(offset+index);
		}
		Code save(offset_t index,bool is_first=true) const override{
			return save_local(offset+index);
		}
		Code get_ref() const override{
			return pushBP();
		}
		std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const override{
			return make(new_size,offset + shift_offset);
		}
	};
	struct StaticVar:MemVar{
		Label label;
		StaticVar(addr_t size,Label label,offset_t offset):MemVar(size,offset),label(std::move(label)){}
		static auto make(addr_t size,const Label& label,offset_t offset){ return std::make_shared<StaticVar>(size,label,offset);}
		Code load(offset_t index,bool is_first=true) const override{
			if(is_first){
				return Ops::load(label,offset+index);
			}else{
				return Ops::load(ASM_PTR,offset+index);
			}
		}
		Code save(offset_t index,bool is_first=true) const override{
			if(is_first){
				return Ops::save(label,offset+index);
			}else{
				return Ops::save(ASM_PTR,offset+index);
			}
		}
		Code get_ref() const override{
			return push(label);
		}
		std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const override{
			return make(new_size,label,offset + shift_offset);
		}
	};
	struct PtrVar:MemVar{
		Code ptr;
		explicit PtrVar(addr_t size,Code ptr,offset_t offset=0):MemVar(size,offset),ptr(std::move(ptr)){}
		static auto make(addr_t size,const Code& ptr,offset_t offset=0){ return std::make_shared<PtrVar>(size,ptr,offset);}
		Code load(offset_t index,bool is_first=true) const override{
			if(is_first){
				return {ptr,Ops::load(offset+index)};
			}else{
				return Ops::load(ASM_PTR,offset+index);
			}
		}
		Code save(offset_t index,bool is_first=true) const override{
			if(is_first){
				return {ptr,Ops::save(offset+index)};
			}else{
				return Ops::save(ASM_PTR,offset+index);
			}
		}
		Code get_ref() const override{
			return ptr;
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
	struct StaticVars:DataBlock,Allocator{
		CodeBlock init;
		std::shared_ptr<MemVar> alloc(addr_t size) override {
			auto var=StaticVar::make(size, start, static_cast<offset_t>(body.size()));
			body.resize(body.size()+size,static_cast<op_t>(0));
			return var;
		}
		template<typename T>
		auto operator=(const T& val){
			T var{alloc(T::size)};
			init.body<<var.set(val);
			return var;
		}
	};
	struct ReadOnlyVars:DataBlock,Allocator{
		std::forward_list<data_t> presets{};
		std::shared_ptr<MemVar> alloc(addr_t size) override {
			auto var=StaticVar::make(size, start, static_cast<offset_t>(body.size()));
			if(presets.empty()){
				body.resize(body.size()+size,static_cast<op_t>(0));
			}else{
				auto data=presets.front();
				body.insert(body.end(),data.begin(),data.end());
				presets.pop_front();
			}
			return var;
		}
		template<typename T>
		auto operator=(const T& val){
			presets.push_front(val.as_raw()->data);
			return T{alloc(T::size)};
		}
	};
}
#endif //BBCPU_VAR_H
