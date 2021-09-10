//
// Created by chasingfar on 2021/8/7.
//

#ifndef BBCPU_VAR_H
#define BBCPU_VAR_H
#include <cstddef>
#include "ops.h"

namespace BBCPU::ASM {
	struct Value{
		virtual code_t load() const=0;
		virtual ~Value()=default;
	};
	struct Expr:Value{
		code_t code{};
		Expr() = default;
		explicit Expr(code_t  value):code(std::move(value)){}
		explicit Expr(const Value& value):code(value.load()){}
		code_t load() const override{
			return code;
		}
		Expr& operator <<(code_t c){
			code<<std::move(c);
			return *this;
		}
	};
	struct Var:Value{
		virtual code_t save() const=0;
	};
	struct RegVar:Var{
		Reg reg;
		explicit RegVar(Reg reg):reg(reg){}
		static auto make(Reg reg){ return std::make_shared<RegVar>(reg);}
		code_t load() const override{
			return push(reg);
		}
		code_t save() const override{
			return pop(reg);
		}
	};
	struct MemVar:Var{
		addr_t size;
		explicit MemVar(addr_t size):size(size){}
		virtual code_t load(offset_t index) const=0;
		virtual code_t save(offset_t index) const=0;
		virtual std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const=0;
		code_t load() const override{
			code_t tmp{};
			for (addr_t i=0; i<size; ++i) {
				tmp<<load(i);
			}
			return tmp;
		}
		code_t save() const override{
			code_t tmp{};
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
		code_t load(offset_t index) const override{
			return load_local(offset-index);
		}
		code_t save(offset_t index) const override{
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
		code_t load(offset_t index) const override{
			return Ops::load(label,offset+index);
		}
		code_t save(offset_t index) const override{
			return Ops::save(label,offset+index);
		}
		std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const override{
			return make(new_size,label,offset + shift_offset);
		}
	};
	struct PtrVar:MemVar{
		code_t ptr;
		offset_t offset;
		explicit PtrVar(addr_t size,code_t ptr,offset_t offset=0):MemVar(size),ptr(std::move(ptr)),offset(offset){}
		static auto make(addr_t size,const code_t& ptr,offset_t offset=0){ return std::make_shared<PtrVar>(size,ptr,offset);}
		code_t load(offset_t index) const override{
			return {ptr,Ops::load(offset+index)};
		}
		code_t save(offset_t index) const override{
			return {ptr,Ops::save(offset+index)};
		}
		std::shared_ptr<MemVar> shift(offset_t shift_offset,addr_t new_size) const override{
			return make(new_size,ptr,offset + shift_offset);
		}
	};
}
#endif //BBCPU_VAR_H
