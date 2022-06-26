//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_CIRCUIT_H
#define BBCPU_CIRCUIT_H
#include <cstddef>
#include <exception>
#include <optional>
#include <type_traits>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <array>
#include <span>
#include <numeric>
namespace Circuit{
	using val_t=unsigned long long;
	struct State{
		enum Level:int{
			Low      = -2,
			PullDown = -1,
			Floating =  0,
			PullUp   =  1,
			High     =  2,
			Error    = INT_MAX,
		};
		Level level=Floating;
		bool updated=false;
		void set(Level new_level){
			/*
o\n	L	H	D	U	Z	E
L	L1	E3	L4	L4	L4	E2
H	E3	H1	H4	H4	H4	E2
D	L2	H2	D1	E3	D4	E2
U	L2	H2	E3	U1	U4	E2
Z	L2	H2	D2	U2	Z1	E2
E	E4	E4	E4	E4	E4	E1
			 */
			if(new_level==level){return;}//1
			if(int cmp=abs(new_level)-abs(level);cmp>0){//2
				level=new_level;
				updated=true;
			}else if(cmp==0){//3
				level=Error;
				updated=true;
			}
			//4
		}
		int get() const{
			if(level==Floating || level==Error){
				return -1;
			}
			return level>0?1:0;
		}
	};
	struct Wire{
		State state{};
		Wire* next=nullptr;
		State& access() {
			return (next == nullptr)?state:next->state;
		}
	};
	struct PortNotValid:std::exception{};
	template<size_t Size>
	struct Port{
		std::array<Wire*,Size> pins;
		Port(){
			for(auto& p:pins){
				p=new Wire{};
			}
		}
		explicit Port(const std::span<Wire*,Size>& pins_view):pins{pins_view}{}
		void set(val_t val){
			std::for_each(pins.begin(), pins.end(), [&](auto p){
				p->access().set((val&1u)==1u?State::High:State::Low);
				val>>=1;
			});
		}
		bool is_valid() const{
			return std::all_of(pins.begin(), pins.end(), [](auto p){
				return p->access().get()>=0;
			});
		}
		val_t get() const{
			if(is_valid()){
				return std::accumulate(pins.begin(), pins.end(),0,
				    [](val_t val, auto p){
						return (val<<1)&p->access().get();
					}
				);
			}
			throw PortNotValid{};
		}
		auto& operator =(val_t val){
			set(val);
			return *this;
		}
		template<size_t NewSize>
		auto sub(size_t start=0){
			return Port<NewSize>{*this,start};
		}
	};
	struct Wires{
		std::vector<Wire*> pool;
		void reset(){
			std::for_each(pool.begin(), pool.end(),[](auto w){
				w->access().updated=false;
			});
		}
		bool is_updated(){
			return std::any_of(pool.begin(), pool.end(),[](auto w){
				return w->access().updated;
			});
		}
		template<size_t Size,typename ...Ts>
		requires (sizeof...(Ts)>2&&((std::is_same_v<Ts,Port<Size>>)&&...))
		void link(Port<Size> port,Ts... ports) {
			for(size_t i=1;i<Size;++i){
				Wire* w=&port.pins[i]->access();
				for(auto p:{ports...}){
					p.pins[i]->access().next=w;
				}
			}
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
		Wires wires;
		std::vector<Component*> comps;
		void update() override{
			do{
				wires.reset();
				std::for_each(comps.begin(), comps.end(),[](auto c){
					c->update();
				});
			}while(wires.is_updated());
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
			if(clr.get()==0){
				Base::reset();
			} else {
				Base::update();
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
	template<size_t Size>
	struct Adder:Component{
		Port<Size> A,B,O;
		void update() override{
			O=A.get()+B.get();
		}
	};
	struct Sim:Circuit{
		Adder<8> adder{};
		RegCLR<8> reg{};
		Sim(){}
	};
}
#endif //BBCPU_CIRCUIT_H
