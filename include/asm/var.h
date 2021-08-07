//
// Created by chasingfar on 2021/8/7.
//

#ifndef BREADBOARDCPU_VAR_H
#define BREADBOARDCPU_VAR_H
#include "ops.h"

namespace BreadBoardCPU::ASM {

	struct RValue{
		addr_t size=0;
		code_t push{};
		bool is_signed=false;
		RValue extend(addr_t to_size) const{
			RValue tmp{*this};
			if(size<to_size){
				tmp.size=to_size;
				for(addr_t i=size;i<to_size;++i){
					tmp.push<<imm(0);
				}
			}
			return tmp;
		}
		operator code_t(){
			return push;
		}
	};
	struct LValue:RValue{
		code_t pop;
		LValue(addr_t size,code_t push,bool is_signed,code_t pop):RValue{size,std::move(push),is_signed},pop(std::move(pop)){}
		code_t set(const RValue& rValue) const{
			code_t tmp{rValue.extend(size).push};
			if(rValue.size>size){
				tmp<<adj(rValue.size-size);
			}
			return {tmp,pop};
		}
	};
	struct Var:LValue{
		Var(addr_t size,const code_t& a,const code_t& b,bool is_signed=false):LValue{size,a,is_signed,b}{}
		explicit Var(Reg reg):LValue{1,Ops::push(reg),false,Ops::pop(reg)}{}
		code_t load(Reg to) const{
			return {push,Ops::pop(to)};
		}
		code_t save(Reg value) const{
			return {Ops::push(value),pop};
		}
	};
	template<typename T>
	struct ImmNum:RValue{
		T value;
		explicit ImmNum(long long value):value(value),RValue{sizeof(T),{},std::is_signed_v<T>}{
			for (size_t i=0;i<size;++i){
				push<<imm((value>>i*8)&0xFF);
			}
		}
		explicit ImmNum(unsigned long long value):value(value),RValue{sizeof(T),{},std::is_signed_v<T>}{
			for (size_t i=0;i<size;++i){
				push<<imm((value>>i*8)&0xFF);
			}
		}
		auto operator-(){
			return ImmNum<T>{-value};
		}
	};
	inline auto operator""_i8 (unsigned long long val){return ImmNum<  int8_t>{val};}
	inline auto operator""_u8 (unsigned long long val){return ImmNum< uint8_t>{val};}
	inline auto operator""_i16(unsigned long long val){return ImmNum< int16_t>{val};}
	inline auto operator""_u16(unsigned long long val){return ImmNum<uint16_t>{val};}

#define DEFINE_1(name1, name2)                                        \
		inline RValue name1(const RValue& lhs) {                      \
			RValue tmp{lhs};                                          \
			if(tmp.size>1){                                           \
				tmp.push<<saveBP();                                   \
				for(addr_t i=0;i<tmp.size;++i){                       \
					tmp.push<<load_local(tmp.size+2-i)                \
					        <<((i==0)?name1():name2())                \
					        <<save_local(tmp.size+2-i);               \
				}                                                     \
				tmp.push<<loadBP();                                   \
			}else if(tmp.size==1){                                    \
				tmp.push<<name1();                                    \
			}                                                         \
			return tmp;                                               \
        }
#define DEFINE_2(name1, name2)                                        \
		inline RValue name1(const RValue& lhs,const RValue& rhs) {    \
			RValue tmp{lhs.extend(rhs.size)};                         \
			if(rhs.size>lhs.size){                                    \
				tmp.is_signed=rhs.is_signed;                          \
			}                                                         \
			if(lhs.size==rhs.size){                                   \
				tmp.is_signed=lhs.is_signed&&rhs.is_signed;           \
			}                                                         \
			tmp.push<<rhs.extend(tmp.size).push;                      \
			if(tmp.size>1){                                           \
				tmp.push<<saveBP();                                   \
				for(addr_t i=0;i<tmp.size;++i){                       \
					tmp.push<<load_local(2*tmp.size+2-i)              \
							<<load_local(tmp.size+2-i)                \
					        <<((i==0)?name1():name2())                \
					        <<save_local(2*tmp.size+2-i);             \
				}                                                     \
				tmp.push<<loadBP()                                    \
						<<adj(tmp.size);                              \
			}else if(tmp.size==1){                                    \
				tmp.push<<name1();                                    \
			}                                                         \
			return tmp;                                               \
        }

	DEFINE_1(shl, rcl)
	DEFINE_1(shr, rcr)
	DEFINE_2(add, adc)
	DEFINE_2(sub, suc)

	DEFINE_1(NOT, NOT)
	DEFINE_2(AND, AND)
	DEFINE_2( OR,  OR)
	DEFINE_2(XOR, XOR)

#undef DEFINE_1
#undef DEFINE_2
}
#endif //BREADBOARDCPU_VAR_H
