//
// Created by chasingfar on 2021/8/21.
//

#ifndef BREADBOARDCPU_TYPE_H
#define BREADBOARDCPU_TYPE_H

#include "ops.h"
namespace BreadBoardCPU::ASM {
	template<addr_t Size>
	struct Type {
		static constexpr addr_t size = Size;
	};
	using Void = Type<0>;


	template<typename T,typename ...Ts>
	struct Struct:Type<(T::size+...+Ts::size)>{
		template<addr_t Index,addr_t Offset>
		struct SubType:Struct<Ts...>::template SubType<Index-1,Offset+T::size>{};
		template<addr_t Offset>
		struct SubType<0,Offset>:T{
			using type = T;
			static constexpr addr_t offset=Offset;
		};
	};

	template<typename T>
	struct Value:T{
		virtual operator code_t() const=0;
	};
	template<typename T>
	struct Expr:Value<T>{
		code_t code{};
		Expr() {}
		explicit Expr(const Value<T>& value):code(value){}
		operator code_t() const override{
			return code;
		}
		Expr<T>& operator <<(code_t c){
			code<<std::move(c);
			return *this;
		}
	};
	struct Stmt:Expr<Void>{
		template<typename T>
		explicit Stmt(const Value<T>& value):Expr<Void>{{value, adj(T::size)}}{}
	};

	template<addr_t Size,bool Signed=false>
	struct Int:Type<Size>{};
	using UInt8 =Int<1,false>;
	using  Int8 =Int<1,true>;
	using UInt16=Int<2,false>;
	using  Int16=Int<2,true>;
	using Bool=UInt8;
	template<typename T>
	using asInt=Int<sizeof(T),std::is_signed_v<T>>;

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
#endif //BREADBOARDCPU_TYPE_H
