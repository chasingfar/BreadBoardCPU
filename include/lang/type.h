//
// Created by chasingfar on 2021/8/21.
//

#ifndef BBCPU_TYPE_H
#define BBCPU_TYPE_H

#include "var.h"
#include <cstddef>
#include <memory>
#define DEF_TYPE_COMMON \
	using Base::Base;\
	code_t operator =(const This& rhs) const {return this->set(rhs);}
namespace BBCPU::ASM {
	template<addr_t Size>
	struct Type {
		static constexpr addr_t size = Size;
		std::shared_ptr<Value> value{};
		Type():value(nullptr){}
		template<typename T> requires std::is_base_of_v<Value,T>
		Type(std::shared_ptr<T> value):value(value){}
		Type(Allocator& allocator):value(allocator.alloc(size)){}
		Type(const code_t& expr):value(std::make_shared<Expr>(expr)){}
		
		operator code_t() const{
			return value->load();
		}
		code_t set(const Type<Size>& rhs) const{
			return {rhs.value->load(),std::dynamic_pointer_cast<Var>(value)->save()};
		}
		code_t operator =(const Type<Size>& rhs) const{
			return set(rhs);
		}
		explicit operator Type<0>(){
			return Type<0>{code_t{*this,adj(size)}};
		}
	};
	using void_ = Type<0>;

	namespace Val{
		inline static const void_ none{code_t{}};
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
		using This = Struct<T,Ts...>;
		using Base = Type<(T::size+...+Ts::size)>;
		DEF_TYPE_COMMON

		static constexpr size_t count=1+sizeof...(Ts);
		template<addr_t Index,addr_t Offset=0>
		struct SubType:Struct<Ts...>::template SubType<Index-1,Offset+T::size>{};
		template<addr_t Offset>
		struct SubType<0,Offset>:T{
			using type = T;
			static constexpr addr_t offset=Offset;
		};
		
		inline static auto make(T val,Ts ...vals){
			return Struct<T,Ts...>{code_t{val,vals...}};
		}

		auto extract(){
			return [&]<size_t... I>(std::index_sequence<I...>){
				return std::tuple{get<I>()...};
			}(std::make_index_sequence<count>{});
		}

		template<size_t I>
		auto get(){
			using type = typename SubType<I>::type;
			return type{
				std::dynamic_pointer_cast<MemVar>(this->value)->shift(SubType<I>::offset,type::size)
			};
		}
	};
	template<typename ...Ts>
	struct Union:Type<std::max({Ts::size...})>{
		using This = Union<Ts...>;
		using Base = Type<std::max({Ts::size...})>;
		DEF_TYPE_COMMON

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
			code_t tmp{val};
			for (size_t i=0; i<std::max({Ts::size...})-V::size; ++i) {
				tmp<<imm(0);
			}
			return Union<Ts...>{tmp};
		}
		auto extract(){
			return [&]<size_t... I>(std::index_sequence<I...>){
				return std::tuple{get<I>()...};
			}(std::make_index_sequence<count>{});
		}

		template<size_t I>
		auto get(){
			using type = typename SubType<I>::type;
			return type{
				std::dynamic_pointer_cast<MemVar>(this->value)->shift(0,type::size)
			};
		}
	};

	template<typename T,size_t N,typename ...Ts>
	struct Array:Array<T,N-1,T,Ts...>{};
	template<typename T,typename ...Ts>
	struct Array<T,0,Ts...>:Struct<Ts...>{
		using This = Array<T,sizeof...(Ts)>;
		using Base = Struct<Ts...>;
		DEF_TYPE_COMMON

		inline static auto make(Ts ...vals){
			return This{code_t{vals...}};
		}
		auto operator[](size_t i){
			return T{
				std::dynamic_pointer_cast<MemVar>(this->value)->shift(T::size*i,T::size)
			};
		}
	};

	template<addr_t Size,bool Signed=false>
	struct Int:Type<Size>{
		using This = Int<Size,Signed>;
		using Base = Type<Size>;
		DEF_TYPE_COMMON

		template<addr_t ...S> requires(Size==(S+...+0))
		inline static auto make(Int<S,Signed> ...vals){
			return This{code_t{vals...}};
		}
		operator Int<1,false>() const{
			code_t tmp{*this};
			for (addr_t i = 1; i < Size; ++i) {
				tmp << OR();
			}
			return Int<1,false>{tmp};
		}
	};
	using u8 =Int<1,false>;
	using i8 =Int<1,true>;
	using u16=Int<2,false>;
	using i16=Int<2,true>;
	using usize=u16;
	using isize=i16;
	using bool_=u8;
	template<typename T>
	using AsInt=Int<sizeof(T),std::is_signed_v<T>>;

	namespace Val{
		inline static const bool_ true_{imm(1)};
		inline static const bool_ false_{imm(0)};
	}

	template<typename T>
	struct ptr: AsInt<addr_t>{
		using This = ptr<T>;
		using Base = AsInt<addr_t>;
		DEF_TYPE_COMMON

		explicit ptr(const usize& v): Base(v.value){}
		template<typename U>
		explicit ptr(const ptr<U>& v):Base(v.value){}

		using type=T;
		auto operator*(){
			return T{PtrVar::make(T::size,value->load(),0)};
		}
	};
	template<typename T>struct UnPtr        {};
	template<typename T>struct UnPtr<ptr<T>>{using type = T;};
	template<typename T>concept IsPtr = requires {typename UnPtr<T>::type;};

	template<typename T>
	struct IntLiteral:AsInt<T>{
		T literal;
		explicit IntLiteral(long long val):literal(val){
			code_t tmp{};
			for (size_t i=0;i<sizeof(T);++i){
				tmp<<imm((val>>i*8)&0xFF);
			}
			this->value=std::make_shared<Expr>(tmp);
		}
		explicit IntLiteral(unsigned long long val):literal(val){
			code_t tmp{};
			for (size_t i=0;i<sizeof(T);++i){
				tmp<<imm((val>>i*8)&0xFF);
			}
			this->value=std::make_shared<Expr>(tmp);
		}
		auto operator-(){
			return IntLiteral<T>{-literal};
		}
		operator AsInt<T>(){
			return AsInt<T>{this->value};
		}
	};
	inline auto operator""_i8 (unsigned long long val){return IntLiteral<  int8_t>{val};}
	inline auto operator""_u8 (unsigned long long val){return IntLiteral< uint8_t>{val};}
	inline auto operator""_i16(unsigned long long val){return IntLiteral< int16_t>{val};}
	inline auto operator""_u16(unsigned long long val){return IntLiteral<uint16_t>{val};}

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
