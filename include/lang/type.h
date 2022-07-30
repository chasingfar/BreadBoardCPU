#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-macro-parentheses"
//
// Created by chasingfar on 2021/8/21.
//

#ifndef BBCPU_TYPE_H
#define BBCPU_TYPE_H

#include "var.h"
#include <cstddef>
#include <memory>

#define DEF_TYPE0 \
	using Base::Base; \
	auto operator =(const This& rhs) const {return this->set(rhs);}// NOLINT(bugprone-unhandled-self-assignment,misc-unconventional-assign-operator)
#define DEF_TYPE(NAME,THIS,BASE) \
	using This = Util::macro_param_t<void THIS>; \
	using Base = Util::macro_param_t<void BASE>; \
	DEF_TYPE0 \
	template<typename ...ARGs> requires std::is_constructible_v<Base,ARGs...> \
	explicit NAME(ARGs&&... args):Base{args...}{} \
	NAME(const Base& base):Base{base}{}
#define DEF_TYPE2(NAME,...) DEF_TYPE(NAME,(NAME),(__VA_ARGS__))

namespace BBCPU::Lang {
	template<addr_t Size>
	struct Type {
		static constexpr addr_t size = Size;
		std::shared_ptr<Value> value{};
		Type():value(nullptr){}
		template<typename T> requires std::is_base_of_v<Value,T>
		explicit Type(std::shared_ptr<T> value):value(value){}
		explicit Type(Allocator& allocator):value(allocator.alloc(size)){}
		explicit Type(const Code& expr):value(std::make_shared<Expr>(expr)){}

		template<typename T>
		auto as() const{return std::dynamic_pointer_cast<T>(value);}
		auto as_raw() const{return as<Raw>();}
		auto as_mem_var() const{return as<MemVar>();}
		auto as_local_var() const{return as<LocalVar>();}
		auto as_static_var() const{return as<StaticVar>();}

		Type<0> set(const Type<Size>& rhs) const{
			return Type<0>{Code{rhs.value->load(), as<Var>()->save()}};
		}
		auto operator =(const Type<Size>& rhs) const{ // NOLINT(bugprone-unhandled-self-assignment,misc-unconventional-assign-operator)
			return set(rhs);
		}
		Code to_code() const{
			return value->load();
		}
		Code to_stmt() const{
			if constexpr(Size==0){
				return to_code();
			}
			return {to_code(),adj(size)};

		}
		explicit operator Type<0>(){
			return Type<0>{to_stmt()};
		}
	};
	using void_ = Type<0>;

	namespace Val{
		inline static const void_ none{Code{}};
	}

	template<typename To,typename From>
	struct TypeCaster{};
	template<typename To,typename From>
	struct SimpleCaster{
		static auto to(From from){
			return To{from.value};
		}
	};

	template<typename T,typename ...Ts>
	struct Struct:Type<(T::size+...+Ts::size)>{
		DEF_TYPE(Struct,(Struct<T,Ts...>),(Type<(T::size+...+Ts::size)>)) // NOLINT(google-explicit-constructor)

		static constexpr size_t count=1+sizeof...(Ts);
		template<addr_t Index,addr_t Offset=0>
		struct SubType:Struct<Ts...>::template SubType<Index-1,Offset+T::size>{};
		template<addr_t Offset>
		struct SubType<0,Offset>:T{
			using type = T;
			static constexpr addr_t offset=Offset;
		};
		
		inline static auto make(T val,Ts ...vals){
			if((val.as_raw()&&...&&vals.as_raw())){
				data_t tmp;
				tmp.reserve(Base::size);
				for(const auto& data:{val.as_raw()->data,vals.as_raw()->data...}){
					tmp.insert(tmp.end(),data.begin(),data.end());
				}
				return This{std::make_shared<Raw>(tmp)};
			}
			return This{Code{val,vals...}};
		}

		auto extract(){
			return [&]<size_t... I>(std::index_sequence<I...>){
				return std::tuple{get<I>()...};
			}(std::make_index_sequence<count>{});
		}

		template<size_t I>
		auto get() const{
			using type = typename SubType<I>::type;
			if(auto raw=this->as_raw();raw){
				return type{raw->shift(SubType<I>::offset,type::size)};
			}else if(auto var=this->as_mem_var();var){
				return type{var->shift(SubType<I>::offset,type::size)};
			}else{
				return type{};
			}
		}
	};
	template<typename ...Ts>
	struct Union:Type<std::max({Ts::size...})>{
		DEF_TYPE(Union,(Union<Ts...>),(Type<std::max({Ts::size...})>)) // NOLINT(google-explicit-constructor)

		template<typename T> requires std::disjunction_v<std::is_same<T, Ts>...>
		explicit Union(const T& v):Base(v.value){}

		static constexpr size_t count=sizeof...(Ts);
		template<addr_t Index>
		struct SubType:std::tuple_element_t<Index, std::tuple<Ts...>>{
			using type = std::tuple_element_t<Index, std::tuple<Ts...>>;
			static constexpr addr_t offset=0;
		};
		template<typename V>
		inline static auto make(V val){
			if(auto v=val.as_raw();v){
				data_t tmp(Base::size,data_t::value_type(static_cast<op_t>(0)));
				std::copy(v->data.begin(),v->data.end(),tmp.begin());
				return This{std::make_shared<Raw>(tmp)};
			}
			Code tmp{val};
			for (size_t i=0; i<std::max({Ts::size...})-V::size; ++i) {
				tmp<<imm(0);
			}
			return This{tmp};
		}
		auto extract(){
			return [&]<size_t... I>(std::index_sequence<I...>){
				return std::tuple{get<I>()...};
			}(std::make_index_sequence<count>{});
		}

		template<size_t I>
		auto get() const{
			using type = typename SubType<I>::type;
			if(auto raw=this->as_raw();raw){
				return type{raw->shift(0,type::size)};
			}else if(auto var=this->as_mem_var();var){
				return type{var->shift(0,type::size)};
			}else{
				return type{};
			}
		}
	};

	template<typename T,size_t N,typename ...Ts>
	struct Array:Array<T,N-1,T,Ts...>{
		using Array<T,N-1,T,Ts...>::Array;
	};
	template<typename T,typename ...Ts>
	struct Array<T,0,Ts...>:Struct<Ts...>{
		DEF_TYPE(Array,(Array<T,sizeof...(Ts)>),(Struct<Ts...>)) // NOLINT(google-explicit-constructor)

		inline static auto make(Ts ...vals){
			if((vals.as_raw()&&...)){
				data_t tmp;
				tmp.reserve(Base::size);
				for(const auto& data:{vals.as_raw()->data...}){
					tmp.insert(tmp.end(),data.begin(),data.end());
				}
				return This{std::make_shared<Raw>(tmp)};
			}
			return This{Code{vals...}};
		}
		auto operator[](size_t i){
			return T{this->as_mem_var()->shift(T::size*i,T::size)};
		}
	};

	template<addr_t Size,bool Signed=false>
	struct Int:Type<Size>{
		DEF_TYPE(Int,(Int<Size,Signed>),(Type<Size>)) // NOLINT(google-explicit-constructor)

		explicit Int(long long val):Base(std::make_shared<Raw>(parse_int(val))){}
		explicit Int(unsigned long long val):Base(std::make_shared<Raw>(parse_int(val))){}
		static data_t parse_int(long long val){
			data_t tmp{};
			for (addr_t i=0;i<Size;++i){
				tmp.emplace_back(static_cast<op_t>((val>>i*8)&0xFF));
			}
			return tmp;
		}
		static data_t parse_int(unsigned long long val){
			data_t tmp{};
			for (addr_t i=0;i<Size;++i){
				tmp.emplace_back(static_cast<op_t>((val>>i*8)&0xFF));
			}
			return tmp;
		}
		std::optional<std::conditional_t<Signed,long long,unsigned long long>> to_int() const{
			std::conditional_t<Signed,long long,unsigned long long> v=0;
			if(auto raw=this->as_raw();raw){
				for(auto byte:raw->data){
					v<<=8;
					v|=byte;
				}
				return v;
			}
			return {};
		}
		template<addr_t ...S> requires(Size==(S+...+0))
		inline static auto make(Int<S,Signed> ...vals){
			return This{Code{vals...}};
		}
		explicit operator Int<1,false>() const{
			Code tmp{*this};
			for (addr_t i = 1; i < Size; ++i) {
				tmp << OR();
			}
			return Int<1,false>{tmp};
		}
	};
	template<typename T>
	using AsInt=Int<sizeof(T),std::is_signed_v<T>>;

	struct bool_:AsInt< uint8_t>{ DEF_TYPE2(bool_,AsInt< uint8_t>) }; // NOLINT(google-explicit-constructor)
	struct   u8 :AsInt< uint8_t>{ DEF_TYPE2(  u8 ,AsInt< uint8_t>) }; // NOLINT(google-explicit-constructor)
	struct   i8 :AsInt<  int8_t>{ DEF_TYPE2(  i8 ,AsInt<  int8_t>) }; // NOLINT(google-explicit-constructor)
	struct   u16:AsInt<uint16_t>{ DEF_TYPE2(  u16,AsInt<uint16_t>) }; // NOLINT(google-explicit-constructor)
	struct   i16:AsInt< int16_t>{ DEF_TYPE2(  i16,AsInt< int16_t>) }; // NOLINT(google-explicit-constructor)
	using usize=u16;
	using isize=i16;

	namespace Val{
		inline static const bool_ true_{imm(1)};
		inline static const bool_ false_{imm(0)};
	}

	template<typename T>
	struct ptr: AsInt<addr_t>{
		DEF_TYPE(ptr,(ptr<T>),(AsInt<addr_t>)) // NOLINT(google-explicit-constructor)

		explicit ptr(const usize& v): Base(v.value){}
		template<typename U>
		explicit ptr(const ptr<U>& v):Base(v.value){}
		explicit ptr(const Label& v):Base(Raw::make({v.get_lazy(0),v.get_lazy(1)})){}

		using type=T;
		auto operator*(){
			return T{PtrVar::make(T::size,value->load(),0)};
		}
	};
	template<typename T>struct UnPtr        {};
	template<typename T>struct UnPtr<ptr<T>>{using type = T;};
	template<typename T>concept IsPtr = requires {typename UnPtr<T>::type;};

	inline auto operator""_i8 (unsigned long long val){return AsInt<  int8_t>{val};}
	inline auto operator""_u8 (unsigned long long val){return AsInt< uint8_t>{val};}
	inline auto operator""_i16(unsigned long long val){return AsInt< int16_t>{val};}
	inline auto operator""_u16(unsigned long long val){return AsInt<uint16_t>{val};}

	inline static const u8 Reg_A{RegVar::make(Reg::A)};
	inline static const u8 Reg_B{RegVar::make(Reg::B)};
	inline static const u8 Reg_C{RegVar::make(Reg::C)};
	inline static const u8 Reg_D{RegVar::make(Reg::D)};
	inline static const u8 Reg_E{RegVar::make(Reg::E)};
	inline static const u8 Reg_F{RegVar::make(Reg::F)};
	inline static const u8 Reg_L{RegVar::make(Reg::L)};
	inline static const u8 Reg_H{RegVar::make(Reg::H)};
}
#endif //BBCPU_TYPE_H

#pragma clang diagnostic pop