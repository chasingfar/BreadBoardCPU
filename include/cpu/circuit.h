//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_CIRCUIT_H
#define BBCPU_CIRCUIT_H
#include <type_traits>
#include <vector>
#include <iostream>
#include <array>
#include <span>
#include <optional>
#include "../util.h"

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
	inline Level& operator+=(Level& o,Level n){
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
			return o=n;
		}else if(cmp==0){//3
			return o=Level::Error;
		}
		return o;//4
	}
	inline Level operator+(Level o,Level n){
		Level t=o;
		return t+=n;
	}
	inline std::optional<uint8_t> read(const Level level){
		if(level==Level::Floating || level==Level::Error){
			return {};
		}
		return static_cast<int8_t>(level)>0?1:0;
	}
	struct Wire:Util::CircularList<Wire>{
		Level level=Level::Floating;
		Level get() const{
			Level tmp=level;
			each([&tmp](const Wire* cur){
				tmp+=cur->level;
			});
			return tmp;
		}
		void set(Level new_level){
			if(level!=new_level){
				level=new_level;
			}
		}
		Wire& operator=(Level new_level){
			set(new_level);
			return *this;
		}
	};
	inline std::optional<uint8_t> read(const Wire& wire){
		return read(wire.get());
	}
	template<size_t Size,typename Pins=std::array<Wire,Size> >
	struct Port{
		Pins pins;
		size_t offset=0;

		Port()=default;
		explicit Port(Pins&& pins,size_t offset=0):pins{pins},offset{offset}{}
		explicit Port(val_t val){set(val);}
		explicit Port(Level level){set(0,level);}

		void set(val_t val,Level zero=Level::Low,Level one=Level::High){
			for(auto& p:pins){
				p=(val&1u)==1u?one:zero;
				val>>=1;
			}
		}
		std::optional<val_t> get() const{
			val_t val=0;
			for(auto& p:pins){
				if(auto v=read(p);v){
					val|=*v;
					val=(val >> 1u) | ((val&1u) << (pins.size() - 1u));
				}else{
					return {};
				}
			}
			return val;
		}
		val_t value() const{
			return get().value();
		}
		auto& operator =(val_t val){
			set(val);
			return *this;
		}
		auto& operator =(Level level){
			set(0,level);
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
				if(auto v=read(*it);v) {
					os << *v;
				}else{
					os << "E";
				}
			}
			return os;
		}
		friend std::ostream& operator<<(std::ostream& os,const Port<Size,Pins>& port){
			//return port.print_ptr(os);
			if(auto v=port.get();v) {
				os << *v;
			}else{
				port.print_bit(os);
			}
			return os;
		}

		auto operator()(std::span<const Level> state) const{
			return Port<Size,std::span<const Level>>{state.subspan(offset,Size)};
		}

		template<size_t NewSize=Size,size_t Start=0>
		auto sub(size_t start=Start) {
			using SPAN=std::span<std::remove_reference_t<decltype(pins[0])>,NewSize>;
			return Port<NewSize,SPAN>{SPAN{&pins[start],NewSize},offset+start};
		}
		template<size_t NewSize=Size,size_t Start=0>
		auto sub(size_t start=Start) const{
			using SPAN=std::span<std::remove_reference_t<decltype(pins[0])>,NewSize>;
			return Port<NewSize,SPAN>{SPAN{&pins[start],NewSize},offset+start};
		}

		template<typename ...Ts>
		requires (sizeof...(Ts)>0&&(
			(std::is_convertible_v<std::remove_cvref_t<decltype(std::declval<Ts>().pins[0])>,Wire>)&&...
		))
		auto wire(Ts&&... ports) {
			return [&]<size_t ...I>(std::index_sequence<I...>){
				return std::vector{[&](size_t i){
					return (pins[i]>>...>>ports.pins[i]),&pins[i];
				}(I)...};
			}(std::make_index_sequence<Size>{});
		}
	};
	struct Enable:Port<1>{
		using Port<1>::Port;
		bool is_enable(){
			return value()==0;
		}
	};
	struct Clock:Port<1>{
		using Port<1>::Port;
		void clock(){
			set(~value());
		}
	};
	struct Component{
		std::string name;
		explicit Component(std::string name=""):name(std::move(name)){}

		virtual bool update()=0;
		virtual void run()=0;

		virtual Util::Printer print() const{
			return [](std::ostream& os){};
		}
		friend std::ostream& operator<<(std::ostream& os,const Component& comp){
			return os<<comp.print();
		}
	};
	struct Chip:Component{
		inline static bool log_read_floating=true;
		inline static bool log_state=true;
		inline static bool log_change=true;

		std::vector<Wire*> pins;
		std::vector<Level> last_state;

		explicit Chip(std::string name=""):Component(std::move(name)){}

		auto save() const{
			std::vector<Level> state{pins.size()};
			std::transform(pins.begin(),pins.end(),state.begin(),[](Wire* w){
				return w->get();
			});
			return state;
		}
		bool update() override{
			auto before=save();
			if(before==last_state){
				return false;
			}
			try{
				run();
			}catch(const std::bad_optional_access& e){
				if(log_read_floating){ std::cout<<"[Warning]"<<name<<"Read Floating"<<std::endl; }
				return false;
			}
			auto new_state=save();
			if(log_state||log_change){ std::cout<<name<<"{"<<print(before)<<"}"; }
			if(before!=new_state){
				last_state=new_state;
				if(log_change){ std::cout<<"{"<<print(new_state)<<"}"<<std::endl; }
				return true;
			}
			if(log_state){std::cout<<std::endl;}
			return false;
		}

		template<size_t ...Sizes>
		void add_ports(Port<Sizes>&... ports){
			pins.reserve((pins.size()+...+Sizes));
			([&](auto& port){
				port.offset=pins.size();
				for(auto& w:port.pins){
					pins.push_back(&w);
				}
			}(ports),...);
		}
		virtual Util::Printer print(std::span<const Level> state) const{
			return [](std::ostream& os){};
		}
		Util::Printer print() const override{
			return [&](std::ostream& os){
				os<<print(save());
			};
		}
	};
	struct Circuit:Component{
		std::vector<Component*> comps;
		explicit Circuit(std::string name=""):Component(std::move(name)){}

		bool update() override{
			return std::any_of(comps.begin(),comps.end(),[](auto c){
				return c->update();
			});
		}
		void run() override {
			while(update()){}
		}

		template<typename ...Ts>
		requires ((std::is_base_of_v<Component,Ts>)&&...)
		void add_comps(Ts&... c){
			(comps.push_back(&c),...);
		}
	};
}
#endif //BBCPU_CIRCUIT_H
