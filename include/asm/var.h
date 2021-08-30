//
// Created by chasingfar on 2021/8/7.
//

#ifndef BBCPU_VAR_H
#define BBCPU_VAR_H
#include <cstddef>
#include <utility>
#include "type.h"

namespace BBCPU::ASM {
	template<typename T>
	struct Var:Value<T>{
		virtual code_t load(offset_t index) const=0;
		virtual code_t save(offset_t index) const=0;
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
	template<template<typename> typename V,typename T>
	struct StructVar:Var<T>{
		auto extract(){
			return [&]<size_t... I>(std::index_sequence<I...>){
				return std::tuple{get<I>()...};
			}(std::make_index_sequence<T::count>{});
		}

		template<size_t I>
		auto get(){
			return static_cast<V<T>*>(this)->template shift<
				typename T::template SubType<I>::type
			>(T::template SubType<I>::offset);
		}
		auto operator[](size_t i){
			using t = typename T::template SubType<0>::type;
			return static_cast<V<T>*>(this)->template shift<t>(t::size*i);
		}
	};
	template<typename T>
	struct LocalVar:StructVar<LocalVar,T>{
		offset_t offset;
		explicit LocalVar(offset_t offset):offset(offset){}
		code_t load(offset_t index) const override{
			return load_local(offset-index);
		}
		code_t save(offset_t index) const override{
			return save_local(offset-index);
		}
		template<typename U>
		auto shift(addr_t size) const {
			return LocalVar<U>{static_cast<offset_t>(offset - size)};
		}
	};
	template<typename T>
	struct StaticVar:StructVar<StaticVar,T>{
		Label label;
		offset_t offset;
		StaticVar(Label label,offset_t offset):label(std::move(label)),offset(offset){}
		code_t load(offset_t index) const override{
			return Ops::load(label,offset+index);
		}
		code_t save(offset_t index) const override{
			return Ops::save(label,offset+index);
		}
		template<typename U>
		auto shift(addr_t size) const {
			return StaticVar<U>{label,static_cast<offset_t>(offset + size)};
		}
	};
	template<typename T>
	struct PtrVar:StructVar<PtrVar,T>{
		code_t ptr;
		offset_t offset;
		explicit PtrVar(const Value<Ptr<T>>& ptr,offset_t offset=0):ptr(ptr),offset(offset){}
		code_t load(offset_t index) const override{
			return {ptr,Ops::load(offset+index)};
		}
		code_t save(offset_t index) const override{
			return {ptr,Ops::save(offset+index)};
		}
		template<typename U>
		auto shift(addr_t size) const {
			return PtrVar<U>{ptr,static_cast<offset_t>(offset + size)};
		}
	};
	template<typename T>
	inline static auto operator*(const Value<Ptr<T>>& lhs){
		return PtrVar<T>{lhs};
	}

	struct RegVar:Var<UInt8>{
		Reg reg;
		explicit RegVar(Reg reg):reg(reg){}
		code_t load(offset_t index) const override{
			return push(reg);
		}
		code_t save(offset_t index) const override{
			return pop(reg);
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
#endif //BBCPU_VAR_H
