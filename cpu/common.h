//
// Created by chasingfar on 2020/11/21.
//

#ifndef BREADBOARDCPU_COMMON_H
#define BREADBOARDCPU_COMMON_H

#include "../include/Bitwise.h"
#include "alu.h"
#include <functional>
namespace BreadBoardCPU {
	using namespace Util::Bitwise::BitField;

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

		static auto pass(auto o) {
			return setLogic(o,LogicFn::A);
		}

		static auto zero(auto o) {
			return setLogic(o,LogicFn::fill0);
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

	template<typename IN,typename OUT,size_t S=0>
	struct TruthTable{
		std::function<OUT(IN)> fn;
		struct iterator{
			size_t in;
			TruthTable& table;
			iterator(TruthTable& table,size_t in):in(in),table(table){}
			OUT operator*() const { return table(in); }

			// Prefix increment
			iterator& operator++() { ++in; return *this; }

			friend bool operator== (const iterator& a, const iterator& b) { return (a.in == b.in)/*&&(a.table==b.table)*/; };
			friend bool operator!= (const iterator& a, const iterator& b) { return !(a==b); };
		};
		explicit TruthTable(std::function<OUT(IN)> fn):fn(fn){}
		OUT operator ()(IN in) const { return fn(in); }
		iterator begin() { return iterator(*this,0); }
		iterator end()   { return iterator(*this,IN(1ull<<S)); }
	};
	template<typename IN,typename OUT> TruthTable(std::function<OUT(IN)>)
	->TruthTable<typename IN::type,typename OUT::type,IN::size>;

	template<typename T>
	struct ROM{
		T data;
		explicit ROM(T data):data(data){}
		friend std::ostream& operator<<(std::ostream& os,ROM rom){
			os<<"v2.0 raw"<<std::endl;
			size_t i=0;
			for(auto out:rom.data){
				os<<std::hex<< static_cast<size_t>(out);
				if(i%8==7){
					os<<std::endl;
				}else{
					os<<" ";
				}
				++i;
			}
			return os;
		}
	};
	/*template<typename IN,typename OUT>
	struct ROM<TruthTable<IN,OUT>>{
		TruthTable<IN,OUT> data;
		explicit ROM(std::function<OUT(IN)> fn):data(TruthTable(fn)){}
	};*/

	template<typename IN,typename OUT>
	void generateROM(std::ostream& os,std::function<OUT(IN)> program){
		os<<"v2.0 raw"<<std::endl;
		IN in{0};//{0b1111100000000000000};
		do{
			//std::cout<<"LGin:"<<in<<std::endl;
			auto out=program(in);
			os<<std::hex<<out;
			//std::cout<<std::dec<<out<<std::endl;
			if((in&7u)==7){
				os<<std::endl;
			}else{
				os<<" ";
			}
			if(in>= (1ull<<19u)-1){
				break;
			}
			in++;
		}while(true);
	}

}
#endif //BREADBOARDCPU_COMMON_H
