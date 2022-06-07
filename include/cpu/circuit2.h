//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_CIRCUIT_H
#define BBCPU_CIRCUIT_H
#include <vector>
#include <string>
namespace Circuit{
	enum struct Status{};
	enum struct PortMode{
		INPUT,
		OUTPUT,
	};
	struct Wire{
		size_t size{};

	};
	struct Pin{
		bool val;
		PortMode mode{PortMode::INPUT};
		Wire* wire;
	};
	struct Port{
		std::string name;
		std::vector<Pin> pins;
		std::vector<Wire*> wire{};
		Port(std::string name,size_t size):name(std::move(name)),pins{size}{}

	};
	struct Component{
		void update(){}
	};
	struct Circuit:Component{

	};

}
#endif //BBCPU_CIRCUIT_H
