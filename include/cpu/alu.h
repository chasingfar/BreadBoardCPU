//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_CPU_ALU_H
#define BBCPU_CPU_ALU_H
#include "common.h"
#include "../alu.h"
namespace BBCPU {
	using namespace ALU74181;
	BITFILEDBASE(6) struct ALU : Base {
		using fn     = BitField<4, Base, FollowMode::innerLow>;
		using method = BitField<1, fn>;
		using carry  = BitField<1, method>;

		using LogicFn=ALU74181::Logic::fn;
		using ArithFn=ALU74181::Arith::fn;
		using Method=ALU74181::Method;
		using Carry=ALU74181::Carry;

		template<size_t size,typename T,typename U>
		static std::pair<Carry,U> run(T o,U A,U B){
			auto Cn=static_cast<Carry>(carry::get(o));
			auto M=static_cast<Method>(method::get(o));
			auto S=fn::get(o);
			return ALU74181::run<size>(Cn,M,S,A,B);
		}
		template<typename T,typename U>
		static std::string get_fn_str(T o,U A,U B){
			auto Cn=static_cast<Carry>(carry::get(o));
			auto M=static_cast<Method>(method::get(o));
			auto S=fn::get(o);
			return ALU74181::get_fn_str(Cn,M,S,A,B);
		}


		static auto setLogic(auto o,LogicFn fn) {
			o=fn::set(o,fn);
			o=method::set(o,Method::logic);
			return o;
		}
		static auto setArith(auto o,ArithFn fn,Carry carry) {
			o=fn::set(o,fn);
			o=method::set(o,Method::arithmetic);
			o=carry::set(o,carry);
			return o;
		}
		static auto setArithMinus(auto o,ArithFn fn,Carry carry) {
			o=fn::set(o,fn);
			o=method::set(o,Method::arithmetic);
			o=carry::set(o,carry==Carry::yes?Carry::no:Carry::yes);
			return o;
		}

		static auto passA(auto o) {
			return setLogic(o,LogicFn::A);
		}
		static auto passB(auto o) {
			return setLogic(o,LogicFn::B);
		}

		static auto zero(auto o) {
			return setLogic(o,LogicFn::fill0);
		}
		static auto minusOne(auto o) {
			return setLogic(o,LogicFn::fill1);
		}

		static auto testAeqZero(auto o) {
			return setArith(o,ArithFn::Aminus1, Carry::no);//CF=Carry::no if A==0
		}
		inline static Carry ifAeqZero=Carry::no;

		static auto testALessB(auto o) {
			return setArith(o,ArithFn::AminusBminus1, Carry::yes);//CF=Carry::no if A<B
		}
		inline static Carry ifALessB=Carry::no;

		static auto add(auto o,Carry carry) {
			return setArith(o,ArithFn::AplusB, carry);
		}
		static auto add(auto o) {
			return add(o, Carry::no);
		}

		static auto sub(auto o,Carry carry) {
			return setArith(o,ArithFn::AminusBminus1, carry);
		}
		static auto sub(auto o) {
			return sub(o, Carry::yes);
		}

		static auto inc(auto o,Carry carry) {
			return setArith(o,ArithFn::A, carry);
		}
		static auto inc(auto o) {
			return inc(o, Carry::yes);
		}

		static auto dec(auto o,Carry carry) {
			return setArith(o,ArithFn::Aminus1, carry);
		}
		static auto dec(auto o) {
			return dec(o,Carry::no);
		}

		static auto shiftLeft(auto o, Carry carry) {
			return setArith(o,ArithFn::AplusA, carry);
		}
		static auto shiftLeft(auto o, unsigned pad) {
			return shiftLeft(o, pad==1?Carry::yes:Carry::no);
		}
		static auto shiftLeft(auto o) {
			return shiftLeft(o, 0);
		}

		static auto AND(auto o) {
			return setLogic(o,LogicFn::AandB);
		}
		static auto OR(auto o) {
			return setLogic(o,LogicFn::AorB);
		}
		static auto XOR(auto o) {
			return setLogic(o,LogicFn::AxorB);
		}
		static auto NOT(auto o) {
			return setLogic(o,LogicFn::notA);
		}
	};
}
#endif //BBCPU_CPU_ALU_H
