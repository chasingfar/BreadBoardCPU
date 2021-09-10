//
// Created by chasingfar on 2021/8/21.
//

#ifndef BBCPU_TYPE_H
#define BBCPU_TYPE_H

#include "var.h"
#include <cstddef>
#include <memory>
namespace BBCPU::ASM {
	template<addr_t Size>
	struct Type {
		static constexpr addr_t size = Size;
		std::shared_ptr<Value> value{};
		Type():value(nullptr){}
		template<typename T> requires std::is_base_of_v<Value,T>
		Type(std::shared_ptr<T> value):value(value){}
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
	};
	using Void = Type<0>;

	namespace Val{
		inline static const Void _void{code_t{}};
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
		code_t operator =(const Struct<T,Ts...>& rhs) const {return this->set(rhs);}

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
		code_t operator =(const Union<Ts...>& rhs) const {return this->set(rhs);}

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
		code_t operator =(const Array<T,sizeof...(Ts)>& rhs) const {return this->set(rhs);}

		inline static auto make(Ts ...vals){
			return Array<T,sizeof...(Ts)>{code_t{vals...}};
		}
		auto operator[](size_t i){
			return T{
				std::dynamic_pointer_cast<MemVar>(this->value)->shift(T::size*i,T::size)
			};
		}
	};

	template<addr_t Size,bool Signed=false>
	struct Int:Type<Size>{
		code_t operator =(const Int<Size,Signed>& rhs) const {return this->set(rhs);}

		template<addr_t ...S> requires(Size==(S+...+0))
		inline static auto make(Int<S,Signed> ...vals){
			return Int<Size,Signed>{code_t{vals...}};
		}
	};
	using UInt8 =Int<1,false>;
	using  Int8 =Int<1,true>;
	using UInt16=Int<2,false>;
	using  Int16=Int<2,true>;
	using Bool=UInt8;
	template<typename T>
	using AsInt=Int<sizeof(T),std::is_signed_v<T>>;

	namespace Val{
		inline static const Bool _true{imm(1)};
		inline static const Bool _false{imm(0)};
	}

	template<typename T>
	struct Ptr:AsInt<addr_t>{
		code_t operator =(const Ptr<T>& rhs) const {return this->set(rhs);}

		using type=T;
		auto operator*(){
			return T{PtrVar::make(T::size,value->load(),0)};
		}
	};
	template<typename T>struct UnPtr        {};
	template<typename T>struct UnPtr<Ptr<T>>{using type = T;};
	template<typename T>concept IsPtr = requires {typename UnPtr<T>::type;};

	template<typename T>
	struct TypeCaster<Ptr<T>,UInt16>:SimpleCaster<Ptr<T>,UInt16>{};
	template<typename T>
	struct TypeCaster<UInt16,Ptr<T>>:SimpleCaster<UInt16,Ptr<T>>{};
	template<typename To,typename From>
	struct TypeCaster<Ptr<To>,Ptr<From>>:SimpleCaster<Ptr<To>,Ptr<From>>{};

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
	};
	inline auto operator""_i8 (unsigned long long val){return IntLiteral<  int8_t>{val};}
	inline auto operator""_u8 (unsigned long long val){return IntLiteral< uint8_t>{val};}
	inline auto operator""_i16(unsigned long long val){return IntLiteral< int16_t>{val};}
	inline auto operator""_u16(unsigned long long val){return IntLiteral<uint16_t>{val};}

	namespace RegVars{
		inline static const UInt8 A{RegVar::make(Reg::A)};
		inline static const UInt8 B{RegVar::make(Reg::B)};
		inline static const UInt8 C{RegVar::make(Reg::C)};
		inline static const UInt8 D{RegVar::make(Reg::D)};
		inline static const UInt8 E{RegVar::make(Reg::E)};
		inline static const UInt8 F{RegVar::make(Reg::F)};
		inline static const UInt8 L{RegVar::make(Reg::L)};
		inline static const UInt8 H{RegVar::make(Reg::H)};
	}
}
#endif //BBCPU_TYPE_H
