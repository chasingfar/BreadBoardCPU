//
// Created by chasingfar on 2021/8/7.
//

#ifndef BREADBOARDCPU_VAR_H
#define BREADBOARDCPU_VAR_H
#include <utility>
#include "type.h"

namespace BreadBoardCPU::ASM {
	template<typename T>
	struct Var:Value<T>{
		virtual code_t load(offset_t index) const=0;
		virtual code_t save(offset_t index) const=0;
		virtual std::shared_ptr<Var<T>> shift(offset_t size) const=0;
		code_t set(const Value<T>& value) const{
			Expr<T> tmp{value};
			for (offset_t i = 0; i < T::size; ++i) {
				tmp.code<<save(T::size-1-i);
			}
			return tmp;
		}
		operator code_t() const override{
			code_t tmp{};
			for (offset_t i = 0; i < T::size; ++i) {
				tmp<<load(i);
			}
			return tmp;
		}
	};
	template<typename T>
	struct LocalVar:Var<T>{
		offset_t offset;
		explicit LocalVar(offset_t offset):offset(offset){}
		code_t load(offset_t index) const override{
			return load_local(offset-index);
		}
		code_t save(offset_t index) const override{
			return save_local(offset-index);
		}
		std::shared_ptr<Var<T>> shift(offset_t size) const override {
			return std::make_shared<LocalVar<T>>(offset - size);
		}
	};
	template<typename T>
	struct StaticVar:Var<T>{
		Label label;
		offset_t offset;
		StaticVar(Label label,offset_t offset):label(std::move(label)),offset(offset){}
		code_t load(offset_t index) const override{
			return Ops::load(label,offset+index);
		}
		code_t save(offset_t index) const override{
			return Ops::save(label,offset+index);
		}
		std::shared_ptr<Var<T>> shift(offset_t size) const override {
			return std::make_shared<StaticVar>(label,offset + size);
		}
	};

	struct RegVar:Var<UInt8>{
		Reg reg;
		explicit RegVar(Reg reg):reg(reg){}
		code_t load(offset_t index) const override{
			return push(reg);
		}
		code_t save(offset_t index) const override{
			return pop(reg);
		}
		std::shared_ptr<Var<UInt8>> shift(offset_t size) const override {
			return std::make_shared<RegVar>(Reg(reg.v()+size));
		}

	};
	namespace RegVars{
		inline static const RegVar A{Reg::A};
		inline static const RegVar B{Reg::B};
		inline static const RegVar C{Reg::C};
		inline static const RegVar D{Reg::D};
		inline static const RegVar E{Reg::E};
		inline static const RegVar F{Reg::F};
		inline static const RegVar L{Reg::L};
		inline static const RegVar H{Reg::H};
	}
}
#endif //BREADBOARDCPU_VAR_H
