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
	struct ReadFloating:std::exception{};
	inline uint8_t operator*(Level level){
		if(level==Level::Floating || level==Level::Error){
			throw ReadFloating{};
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
		uint8_t operator*() const{
			return *get();
		}
		Wire& operator=(Level new_level){
			set(new_level);
			return *this;
		}
	};
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
		val_t get() const{
			val_t val=0;
			for(auto& p:pins){
				val|=*p;
				val=(val >> 1) | ((val&1) << (pins.size() - 1));
			}
			return val;
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
				try {
					os<<**it;
				} catch (const ReadFloating& e) {
					os<<"E";
				}
			}
			return os;
		}
		friend std::ostream& operator<<(std::ostream& os,const Port<Size,Pins>& port){
			//return port.print_ptr(os);
			try {
				os<<port.get();
			} catch (const ReadFloating& e) {
				port.print_bit(os);
			}
			return os;
		}

		auto operator()(const std::vector<Level>& state) const{
			return Port<Size,std::span<const Level>>{
				std::span<const Level,Size>{&state[offset],Size}
			};
		}

		template<size_t NewSize=Size,size_t Start=0>
		auto sub(size_t start=Start) {
			return Port<NewSize,std::span<Wire,NewSize>>{
				std::span<Wire,NewSize>{&pins[start],NewSize},offset+start
			};
		}
		template<size_t NewSize=Size,size_t Start=0>
		auto sub(size_t start=Start) const{
			return Port<NewSize,std::span<const Wire,NewSize>>{
				std::span<const Wire,NewSize>{&pins[start],NewSize},offset+start
			};
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
			return get()==0;
		}
	};
	struct Clock:Port<1>{
		using Port<1>::Port;
		void clock(){
			set(~get());
		}
	};
	struct Component{
		std::vector<Wire*> pins;
		std::string name="";
		Component(std::string name=""):name(std::move(name)){}
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
		auto save() const{
			std::vector<Level> state{pins.size()};
			std::transform(pins.begin(),pins.end(),state.begin(),[](Wire* w){
				return w->get();
			});
			return state;
		}

		virtual bool update(){
			auto before=save();
			std::cout<<name<<"{"<<print(before)<<"}";
			run();
			auto after=save();
			if(before!=after){
				std::cout<<"=>{"<<print(after)<<"}[updated!!!!!]"<<std::endl;
				return true;
			}
			std::cout<<std::endl;
			return false;
		}
		virtual void run(){}
		virtual Util::Printer print(const std::vector<Level>& state) const{
			return [](std::ostream& os){};
		}
		virtual void reset(){}
		friend std::ostream& operator<<(std::ostream& os,const Component& comp){
			return os<<comp.print(comp.save());
		}
	};
	struct Circuit:Component{
		inline static bool ignoreReadFloating=false;
		std::vector<Component*> comps;
		Circuit(std::string name=""):Component(std::move(name)){}
		bool comps_update(){
			bool hasReadFloating=false;
			bool hasUpdate=false;
			for(auto c:comps){
				try{
					if(c->update()){
						hasUpdate=true;
					}
				}catch(const ReadFloating& e){
					std::cout<<std::endl;
					hasReadFloating=true;
				}
			}
			if(hasReadFloating && !ignoreReadFloating){
				std::cout<<"ReadFloating"<<std::endl;
			}
			return hasUpdate;
		}
		bool update() override{
			bool hasUpdate=false;
			while(comps_update()){
				hasUpdate=true;
			};
			if(hasUpdate){
				std::cout<<name<<std::endl;
			}
			return hasUpdate;
		}

		template<typename ...Ts>
		requires ((std::is_base_of_v<Component,Ts>)&&...)
		void add_comps(Ts&... c){
			(comps.push_back(&c),...);
		}
	};
}
#endif //BBCPU_CIRCUIT_H
