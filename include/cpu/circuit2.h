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
namespace Util{
	template<typename T>
	struct CircularList{
		using node_t = T*;
		template<typename S>
		static auto self(S* ptr){return static_cast<std::conditional_t<std::is_const_v<S>,const T*,T*>>(ptr);}
		template<typename S,typename Fn>
		static bool any_of(S* ptr,Fn&& fn){
			auto cur=self(ptr);
			do {
				if(fn(cur)){
					return true;
				}
				cur=cur->next;
			}while(cur!=self(ptr));
			return false;
		}
		T* next=static_cast<T*>(this);
		bool any(auto&& fn) const{
			return any_of(this,fn);
		}
		bool any(auto&& fn){
			return any_of(this,fn);
		}
		void each(auto&& fn) const{
			any([&](auto&& cur){
				fn(cur);
				return false;
			});
		}
		void each(auto&& fn) {
			any([&](auto&& cur){
				fn(cur);
				return false;
			});
		}
		bool has(T* node) const{
			return any([node](auto&& cur){
				return cur==node;
			});
		}
		void merge(T* list){
			if(!has(list)){
				std::swap(next,list->next);
			}
		}
	};
}
namespace Circuit{
	using val_t=unsigned long long;
	enum struct Level:int8_t{
		Low      = -2,
		PullDown = -1,
		Floating =  0,
		PullUp   =  1,
		High     =  2,
		Error    = INT8_MAX,
	};
	inline Level interact(Level o,Level n){
		/*
o\n	L	H	D	U	Z	E
L	L1	E3	L4	L4	L4	E2
H	E3	H1	H4	H4	H4	E2
D	L2	H2	D1	E3	D4	E2
U	L2	H2	E3	U1	U4	E2
Z	L2	H2	D2	U2	Z1	E2
E	E4	E4	E4	E4	E4	E1
		*/
		if(n==o){return o;}//1
		int cmp=abs(static_cast<int8_t>(n))-abs(static_cast<int8_t>(o));
		if(cmp>0){//2
			return n;
		}else if(cmp==0){//3
			return Level::Error;
		}
		return o;
		//4
	}
	inline int read_level(Level level){
		if(level==Level::Floating || level==Level::Error){
			return -1;
		}
		return static_cast<int8_t>(level)>0?1:0;
	}
	struct Wire:Util::CircularList<Wire>{
		Level level=Level::Floating;
		bool updated=false;
		int get() const{
			Level tmp=level;
			each([&tmp](const Wire* cur){
				tmp=interact(tmp, cur->level);
			});
			return read_level(tmp);
		}
		void set(Level new_level){
			if(level!=new_level){
				level=new_level;
				updated=true;
			}
		}
		void reset(){
			each([](Wire* cur){
				cur->updated=false;
			});
		}
		bool has_updated(){
			return any([](Wire* cur){
				return cur->updated;
			});
		}
	};
	struct PortNotValid:std::exception{};
	template<size_t Size>
	struct Port{
		std::array<Wire,Size> pins;
		void set(val_t val){
			for(auto& p:pins){
				p.set((val&1u)==1u?Level::High:Level::Low);
				val>>=1;
			}
		}
		bool is_valid() const{
			for(auto& p:pins){
				if(p.get()<0){
					return false;
				}
			}
			return true;
		}
		val_t get() const{
			if(is_valid()){
				val_t val=0;
				for(auto& p:pins){
					val|=p.get();
					val=(val >> 1) | ((val&1) << (Size - 1));
				}
				return val;
			}
			throw PortNotValid{};
		}
		auto& operator =(val_t val){
			set(val);
			return *this;
		}
		std::ostream& print_ptr(std::ostream& os) const{
			for (auto i = pins.rbegin(); i != pins.rend(); ++i) {
				os<<*i<<" ";
			}
			return os;
		}
		friend std::ostream& operator<<(std::ostream& os,const Port<Size>& port){
			//return port.print_ptr(os);
			try {
				os<<port.get();
			} catch (const PortNotValid& e) {
				os<<"E";
			}
			return os;
		}
		operator std::span<Wire,Size>(){
			return pins;
		}
		template<size_t NewSize>
		auto sub(size_t offset=0){
			return std::span<Wire,NewSize>{&pins[offset],NewSize};
		}
	};
	struct Enable:Port<1>{
		operator bool(){
			return get()==0;
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
		std::vector<Component*> comps;
		void wires_reset(){
			for(auto w:wires){
				w->reset();
			}
		}
		bool wires_is_updated(){
			for(auto w:wires){
				if(w->has_updated()){
					return true;
				}
			}
			return false;
		}
		void comps_update(){
			bool has_PortNotValid=false;
			for(auto c:comps){
				try{
					c->update();
				}catch(const PortNotValid& e){
					has_PortNotValid=true;
				}
			}
			if(has_PortNotValid){
				std::cout<<"PortNotValid"<<std::endl;
			}
		}
		void update() override{
			do{
				wires_reset();
				comps_update();
			}while(wires_is_updated());
		}
		template<size_t Size,typename ...Ts>
		requires (sizeof...(Ts)>0&&((std::is_convertible_v<Ts,std::span<Wire,Size>>)&&...))
		void wire(std::span<Wire,Size> pins,Ts&&... other) {
			for(size_t i=0;i<Size;++i){
				for(auto&& p:{static_cast<std::span<Wire,Size>>(other)...}){
					pins[i].merge(&p[i]);
				}
				wires.push_back(&pins[i]);
			}
		}
		template<size_t Size,typename ...Ts>
		requires (sizeof...(Ts)>0&&((std::is_convertible_v<Ts,std::span<Wire,Size>>)&&...))
		void wire(Port<Size>& port,Ts&&... other) {
			wire(static_cast<std::span<Wire,Size>>(port),other...);
		}
		template<typename ...Ts>
		requires ((std::is_base_of_v<Component,Ts>)&&...)
		void add_comps(Ts&... c){
			(comps.push_back(&c),...);
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
			return os<<input<<"=>"<<data<<"=>"<<output<<"(clk="<<clk<<")";
		}
	};
	template<size_t Size>
	struct RegEN:Reg<Size>{
		using Base=Reg<Size>;
		Enable en;
		void update() override {
			if(en){
				Base::update();
			}
		}
		std::ostream& print(std::ostream& os) const override{
			return Base::print(os)<<"(en="<<en<<")";
		}
	};
	template<size_t Size>
	struct RegCLR:Reg<Size>{
		using Base=Reg<Size>;
		Enable clr;
		void update() override {
			if(clr){
				Base::reset();
			} else {
				Base::update();
			}
		}
		std::ostream& print(std::ostream& os) const override{
			return Base::print(os)<<"(clr="<<clr<<")";
		}
	};
	template<size_t Size>
	struct Nand:Component{
		Port<Size> A,B,Y;
		void update() override{
			Y=~(A.get()&B.get());
		}
	};
	template<size_t Size>
	struct Adder:Component{
		Port<Size> A,B,O;
		void update() override{
			O=A.get()+B.get();
		}
		std::ostream& print(std::ostream& os) const override{
			return os<<A<<"+"<<B<<"="<<O;
		}
	};
	struct Sim:Circuit{
		Adder<8> adder{};
		RegCLR<8> reg{};
		Sim(){
			add_comps(adder,reg);
			wire(adder.O,reg.input);
			wire(adder.A,reg.output);
			adder.B.set(1);
			reg.clr.set(1);
			reg.clk.set(0);
		}
		void update() override{
			reg.clk.set(~reg.clk.get());
			Circuit::update();
		}
	};
}
#endif //BBCPU_CIRCUIT_H
