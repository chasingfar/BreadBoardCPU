//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_CIRCUIT_H
#define BBCPU_CIRCUIT_H
#include <exception>
#include <optional>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <numeric>
namespace Circuit{
	using val_t=unsigned long long;
	enum struct PortMode{
		INPUT,
		OUTPUT,
	};
	struct Wire{
		enum Level:int{
			Low      = -2,
			PullDown = -1,
			Floating =  0,
			PullUp   =  1,
			High     =  2,
			Error    = INT_MAX,
		};
		Level val;
		void set(Level v){
			/*
 	L	H	D	U	Z
L	L	E	L	L	L
H	E	H	H	H	H
D	L	H	D	E	D
U	L	H	E	U	U
Z	L	H	D	U	Z
			 */
			if(v==val){return;}
			if(auto cmp=abs(v)<=>abs(val);cmp==std::strong_ordering::greater){
				val=v;
			}else if(cmp==std::strong_ordering::equal){
				val=Error;
			}
		}
		int get() const{
			if(val==Floating || val==Error){
				return -1;
			}
			return val>0?1:0;
		}
	};
	struct PortNotValid:std::exception{};
	template<size_t Size>
	struct Port{
		std::array<Wire*,Size> pins;
		void set(val_t val){
			std::for_each(pins.begin(), pins.end(), [&](auto &p){
				p.set((val&1u)==1u?Wire::High:Wire::Low);
				val>>=1;
			});
		}
		bool is_valid() const{
			return std::all_of(pins.begin(), pins.end(), 
				[](const auto &p){return p.get()>=0;}
			);
		}
		val_t get() const{
			if(is_valid()){
				return std::accumulate(pins.begin(), pins.end(),0,
				    [](val_t val,const auto &p){
						return (val<<1)&p.get();
				});
			}
			throw PortNotValid{};
		}
		auto& operator =(val_t val){
			set(val);
			return *this;
		}
	};
	struct Component{
		virtual void update(){}
		virtual std::ostream& print(std::ostream& os) const{
			return os;
		}
		virtual void reset(){}
		friend std::ostream& operator<<(std::ostream& os,const Component& comp){
			return comp.print(os);
		}
	};
	struct Circuit:Component{
		std::vector<Wire*> wires;
		void update() override{

		}
	};

	template<size_t Size>
	struct Reg:Component{
		Port<1> clk;
		Port<Size> input,output;
		val_t data{};
		void update() override {
			if(clk.get() == 0){
				output=data;
			}else{
				data=input.get();
			}
		}
		void reset() override{
			data=0;
			output=0;
		}
		std::ostream& print(std::ostream& os) const override{
			return os<<input.get()<<"=>"<<data<<"=>"<<output.get()<<"(clk="<<clk.get()<<")";
		}
	};
	template<size_t Size>
	struct RegEN:Reg<Size>{
		using Base=Reg<Size>;
		Port<1> en;
		void update() override {
			if(en.get() == 0){
				Base::update();
			}
		}
		std::ostream& print(std::ostream& os) const override{
			return Base::print(os)<<"(en="<<en.get()<<")";
		}
	};
	template<size_t Size>
	struct RegCLR:Reg<Size>{
		using Base=Reg<Size>;
		Port<1> clr;
		void update() override {
			if(clr.get(0)){
				Base::update();
			} else {
				Base::reset();
			}
		}
		std::ostream& print(std::ostream& os) const override{
			return Base::print(os)<<"(clr="<<clr.get()<<")";
		}
	};
	template<size_t Size>
	struct Nand:Component{
		Port<Size> A,B,Y;
		void update() override{
			try{
				Y=~(A.get()&B.get());
			}catch(const PortNotValid& e){
				;
			}
		}
	};

}
#endif //BBCPU_CIRCUIT_H
