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

	template<auto fn,auto fnc=fn>
	inline RValue _calc(const RValue &lhs) {
		RValue tmp{lhs};
		if (tmp.size > 1) {
			tmp.push << saveBP();
			for (addr_t i = 0; i < tmp.size; ++i) {
				tmp.push << load_local(tmp.size + 2 - i)
						 << (i == 0 ? fn() : fnc())
						 << save_local(tmp.size + 2 - i);
			}
			tmp.push << loadBP();
		} else if (tmp.size == 1) {
			tmp.push << shl();
		}
		return tmp;
	}

	template<auto fn,auto fnc=fn>
	inline RValue _calc(const RValue& lhs,const RValue& rhs) {
		RValue tmp{lhs.extend(rhs.size)};
		if (rhs.size > lhs.size) { tmp.is_signed = rhs.is_signed; }
		if (lhs.size == rhs.size) { tmp.is_signed = lhs.is_signed && rhs.is_signed; }
		tmp.push << rhs.extend(tmp.size).push;
		if (tmp.size > 1) {
			tmp.push << saveBP();
			for (addr_t i = 0; i < tmp.size; ++i) {
				tmp.push << load_local(2 * tmp.size + 2 - i)
						 << load_local(tmp.size + 2 - i)
				         << (i == 0 ? fn() : fnc())
				         << save_local(2 * tmp.size + 2 - i);
			}
			tmp.push << loadBP() << adj(tmp.size);
		} else if (tmp.size == 1) {
			tmp.push << fn();
		}
		return tmp;
	}

	inline RValue shl(const RValue& lhs)                   {return _calc<Ops::shl,Ops::rcl>(lhs);}
	inline RValue shr(const RValue& lhs)                   {return _calc<Ops::shr,Ops::rcr>(lhs);}
	inline RValue add(const RValue& lhs,const RValue& rhs) {return _calc<Ops::add,Ops::adc>(lhs,rhs);}
	inline RValue sub(const RValue& lhs,const RValue& rhs) {return _calc<Ops::sub,Ops::suc>(lhs,rhs);}

	inline RValue NOT(const RValue& lhs)                   {return _calc<Ops::NOT,Ops::NOT>(lhs);}
	inline RValue AND(const RValue& lhs,const RValue& rhs) {return _calc<Ops::AND,Ops::AND>(lhs,rhs);}
	inline RValue  OR(const RValue& lhs,const RValue& rhs) {return _calc<Ops:: OR,Ops:: OR>(lhs,rhs);}
	inline RValue XOR(const RValue& lhs,const RValue& rhs) {return _calc<Ops::XOR,Ops::XOR>(lhs,rhs);}
}
#endif //BREADBOARDCPU_VAR_H
