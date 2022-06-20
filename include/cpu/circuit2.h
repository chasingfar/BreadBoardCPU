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
	struct PortNotValid{};
	template<size_t Size>
	struct Port{
		std::array<Wire*,Size> pins;
		void set(val_t val){
			std::for_each(pins.begin(), pins.end(), [&](auto &p){
				p.set((val&1u)==1u?Wire::High:Wire::Low);
				val>>=1;
			});
		}
		bool is_valid(){
			return std::all_of(pins.begin(), pins.end(), 
				[](const auto &p){return p.get()>=0;}
			);
		}
		std::optional<val_t> get(){
			return std::accumulate(pins.begin(), pins.end(),0,
				[](std::optional<val_t> val,const auto &p)->std::optional<val_t>{
					int v=p.get();
					if(!val || v<0){
						return {};
					}
					return (*val<<1)&v;
			});
		}
		operator val_t(){
			if(is_valid()){
				return get();
			}
			throw PortNotValid{};
		}
	};
	struct Component{
		virtual void update(){}
	};
	struct Circuit:Component{

	};
	template<size_t Size>
	struct Nand:Component{
		Port<Size> A,B,Y;
		void update() override{
			try{
				Y=~(A&B);
			}catch(PortNotValid e){
				;
			}
		}
	};

}
#endif //BBCPU_CIRCUIT_H
