//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_CIRCUIT_H
#define BBCPU_CIRCUIT_H
#include <vector>
#include <string>
#include <iostream>

namespace Circuit{
	enum struct PortMode{
		INPUT,
		OUTPUT,
	};
	struct Component{
		void update(){}
	};
	struct Pin;
	struct Wire{
		std::vector<Pin*> pins;
	};
	struct Pin{
		bool val;
		PortMode mode{PortMode::INPUT};
		Wire* wire;
		Component* parent;
		Pin& operator =(bool v){
			if(val!=v){
				if(mode==PortMode::INPUT){
					std::cout<<"alert wire conflict";
				}
				val=v;
				for(auto p:wire->pins){
					if(p!=this){

					}
				}
			}
			return *this;
		}
	};
	struct Port{
		std::string name;
		std::vector<Pin> pins;
		std::vector<Wire*> wire{};
		Port(std::string name,size_t size):name(std::move(name)),pins{size}{}

	};
	struct Circuit:Component{

	};

}
#endif //BBCPU_CIRCUIT_H
