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
		T* next=self();
		auto self() const{return static_cast<const T*>(this);}
		auto self() {return static_cast<T*>(this);}

		auto each(auto&& fn) const{
			return loop(self(),std::forward<decltype(fn)>(fn));
		}
		auto each(auto&& fn){
			return loop(self(),std::forward<decltype(fn)>(fn));
		}
		bool has(T* node) const{
			return each([node](auto&& cur){
				return cur==node;
			});
		}
		T& link(T* list){
			if(!has(list)){
				std::swap(next,list->next);
			}
			return *self();
		}
		T& operator <<(T& list){
			return link(&list);
		}
	private:
		//CRTP guard
		CircularList()=default;
		friend T;

		static auto loop(auto head,auto&& fn){
			constexpr bool return_void=std::is_same_v<decltype(fn(head)),void>;
			auto cur=head;
			do {
				if constexpr(return_void){
					fn(cur);
				}else{
					if(fn(cur)){
						return true;
					}
				}
				cur=cur->next;
			}while(cur!=head);
			if constexpr(!return_void){
				return false;
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
			return each([](Wire* cur){
				return cur->updated;
			});
		}
	};
	struct PortNotValid:std::exception{};
	template<size_t Size>
	struct SubPort{
		std::span<Wire,Size> pins;
		template<typename ...Ts>
		requires (sizeof...(Ts)>0&&((std::is_convertible_v<Ts,SubPort<Size>>)&&...))
		auto wire(Ts&&... ports) {
			return [&]<size_t ...I>(std::index_sequence<I...>){
				return std::vector{[&](size_t i){
					return &(pins[i]<<...<<ports.pins[i]);
				}(I)...};
			}(std::make_index_sequence<Size>{});
		}
	};
	template<size_t Size>
	struct Port{
		std::array<Wire,Size> pins;
		Port()=default;
		Port(val_t val){set(val);}
		Port(Level level){set(0,level);}
		void set(val_t val,Level zero=Level::Low,Level one=Level::High){
			for(auto& p:pins){
				p.set((val&1u)==1u?one:zero);
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
			for (auto it = pins.rbegin(); it != pins.rend(); ++it) {
				os<<*it<<" ";
			}
			return os;
		}
		std::ostream& print_bit(std::ostream& os) const{
			for (auto it = pins.rbegin(); it != pins.rend(); ++it) {
				if(auto v=it->get();v>0){
					os<<v;
				}else{
					os<<"E";
				}
			}
			return os;
		}
		friend std::ostream& operator<<(std::ostream& os,const Port<Size>& port){
			//return port.print_ptr(os);
			try {
				os<<port.get();
			} catch (const PortNotValid& e) {
				port.print_bit(os);
			}
			return os;
		}
		operator SubPort<Size>(){
			return pins;
		}
		template<size_t NewSize=Size,size_t Offset=0>
		auto sub(size_t offset=Offset){
			return SubPort<NewSize>{std::span<Wire,NewSize>{&pins[offset],NewSize}};
		}
		auto wire(auto&& ...ports){
			return sub().wire(ports...);
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
		
		template<typename ...Ts>
		requires ((std::is_same_v<decltype(wires),Ts>)&&...)
		void add_wires(const Ts&... w){
			(wires.insert(wires.end(),w.begin(),w.end()),...);
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
		Enable en{Level::PullUp};
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
		Enable clr{Level::PullUp};
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
			add_wires(
				adder.O.wire(reg.input),
				adder.A.wire(reg.output)
			);
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
