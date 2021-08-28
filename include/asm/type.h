//
// Created by chasingfar on 2021/8/21.
//

#ifndef BBCPU_TYPE_H
#define BBCPU_TYPE_H

#include "ops.h"
#include <cstddef>
namespace BBCPU::ASM {
	template<addr_t Size>
	struct Type {
		static constexpr addr_t size = Size;
	};
	using Void = Type<0>;
	
	template<typename U>
	struct TypeCaster{};

	template<typename T>
	struct Value:T{
		virtual operator code_t() const=0;
	};
	template<typename T>
	struct Expr:Value<T>{
		code_t code{};
		Expr() {}
		explicit Expr(const code_t& value):code(value){}
		explicit Expr(const Value<T>& value):code(value){}
		operator code_t() const override{
			return code;
		}
		Expr<T>& operator <<(code_t c){
			code<<std::move(c);
			return *this;
		}
	};
	inline static const Expr<Void> _void{};
	struct Stmt:Expr<Void>{
		template<typename T>
		explicit Stmt(const Value<T>& value):Expr<Void>{{value, adj(T::size)}}{}
	};

	template<typename U,typename T,typename ...Ts>
	struct Struct:Type<(T::size+...+Ts::size)>{
		static constexpr size_t count=1+sizeof...(Ts);
		template<addr_t Index,addr_t Offset=0>
		struct SubType:Struct<U,Ts...>::template SubType<Index-1,Offset+T::size>{};
		template<addr_t Offset>
		struct SubType<0,Offset>:T{
			using type = T;
			static constexpr addr_t offset=Offset;
		};
		
		inline static auto make(const Value<T>& val,const Value<Ts>& ...vals){
			return Expr<U>{{val,vals...}};
		}
	};

	template<typename T,size_t N,typename ...Ts>
	struct Array:Array<T,N-1,T,Ts...>{};
	template<typename T,typename ...Ts>
	struct Array<T,0,Ts...>:Struct<Array<T,sizeof...(Ts)>,Ts...>{};

	template<addr_t Size,bool Signed=false>
	struct Int:Type<Size>{
		template<addr_t ...S> requires(Size==(S+...+0))
		inline static auto make(const Value<Int<S,Signed>>& ...vals){
			return Expr<Int<Size,Signed>>{{vals...}};
		}
	};
	using UInt8 =Int<1,false>;
	using  Int8 =Int<1,true>;
	using UInt16=Int<2,false>;
	using  Int16=Int<2,true>;
	using Bool=UInt8;
	template<typename T>
	using asInt=Int<sizeof(T),std::is_signed_v<T>>;

	template<typename T>
	struct Ptr:asInt<addr_t>{
		using type=T;
	};
	template<typename T>struct UnPtr        {};
	template<typename T>struct UnPtr<Ptr<T>>{using type = T;};
	template<typename T>concept IsPtr = requires {typename UnPtr<T>::type;};

	template<typename T>
	struct IntLiteral:Expr<asInt<T>>{
		T literal;
		explicit IntLiteral(long long val):literal(val){
			for (size_t i=0;i<sizeof(T);++i){
				this->code<<imm((val>>i*8)&0xFF);
			}
		}
		explicit IntLiteral(unsigned long long val):literal(val){
			for (size_t i=0;i<sizeof(T);++i){
				this->code<<imm((val>>i*8)&0xFF);
			}
		}
		auto operator-(){
			return IntLiteral<T>{-literal};
		}
	};
	inline auto operator""_i8 (unsigned long long val){return IntLiteral<  int8_t>{val};}
	inline auto operator""_u8 (unsigned long long val){return IntLiteral< uint8_t>{val};}
	inline auto operator""_i16(unsigned long long val){return IntLiteral< int16_t>{val};}
	inline auto operator""_u16(unsigned long long val){return IntLiteral<uint16_t>{val};}
}
#endif //BBCPU_TYPE_H
