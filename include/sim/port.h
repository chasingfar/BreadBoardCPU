#ifndef BBCPU_SIM_PORT_H
#define BBCPU_SIM_PORT_H

#include <iostream>
#include <type_traits>
#include <vector>
#include <array>
#include <span>
#include <string>
#include "wire.h"

namespace BBCPU::Sim{

	using val_t=unsigned long long;
	enum struct Mode{IN,IO,OUT};

	template<size_t Size,typename Pins=std::array<Wire,Size> >
	struct Port{
		std::string name;
		Mode mode=Mode::IO;
		Pins pins;
		size_t offset=0;

		Port(std::string name=""):name(std::move(name)){}
		explicit Port(Pins&& pins,size_t offset=0,std::string name=""):name(std::move(name)),pins{pins},offset{offset}{}
		explicit Port(val_t val,std::string name=""):name(std::move(name)),mode(Mode::OUT){set(val);}
		explicit Port(Level level,std::string name=""):name(std::move(name)),mode(Mode::OUT){set(0,level);}
		explicit Port(Mode mode,std::string name=""):name(std::move(name)),mode(mode){}

		void set(val_t val,Level zero=Level::Low,Level one=Level::High){
			for(auto& p:pins){
				p=(val&1u)==1u?one:zero;
				val>>=1;
			}
		}
		Util::Result<val_t,Level> get() const{
			val_t val=0;
			for(auto& p:pins){
				if(auto v=read(p);v){
					val|=*v;
					val=(val >> 1u) | ((val&1u) << (pins.size() - 1u));
				}else{
					return v.error();
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
		Util::Printer print_bit() const{
			return [&](std::ostream& os){
				for (auto it = pins.rbegin(); it != pins.rend(); ++it) {
					if(auto v=read(*it);v) {
						os << (*v?"1":"0");
					}else{
						os << (v.error()==Level::Floating?"F":"E");
					}
				}
			};
		}
		friend std::ostream& operator<<(std::ostream& os,const Port<Size,Pins>& port){
			//return port.print_ptr(os);
			if(auto v=port.get();v) {
				os << *v;
			}else{
				os << port.print_bit();
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
		Enable(std::string name=""):Port(Mode::IN,name){}
		enum En{Yes=0,No=1};
		bool is_enable(){
			return value()==Yes;
		}
		void  enable(){set(Yes);}
		void disable(){set(No);}
	};
	struct Clock:Port<1>{
		using Port<1>::Port;
		Clock(std::string name=""):Port(Mode::IN,name){}
		void clock(){
			set(~value());
		}
	};
}
#endif //BBCPU_SIM_PORT_H
