#include <iostream>
#include <fstream>

#include "cpu/BreadBoardCPU.h"
int main() {
	using namespace BreadBoardCPU;


	//MCode mctx(0,0,0);
	//mctx.inc16(Reg16::PC);
	//std::cout<<std::bitset<16>(-1);
	//RegPair::generateRPROM();
	//OpCode::generateOPROM();
	ASM::generateASMROM();
	//ASM::testASM();
	/*{
		using namespace ALU74181;
		auto [c,f]=run<4>(Carry::no,Method::arithmetic,Arith::fn::AornotBplusAandB,0b0100u,0b1010u);
		std::cout<<std::bitset<4>(f)<<std::endl;
		std::cout<<std::boolalpha<<(c==Carry::yes)<<std::endl;
	}*/

	/*
	std::ofstream fout("a.txt");
	if(!fout) {return 1;}
	LogicGenerator::generateROM(fout,std::function{[](OpCode::StackBased::IN::type in){
		std::cout<<std::bitset<19>(in);
		auto out=OpCode::StackBased::opcode::run(in);
		std::cout<<"=>"<<std::bitset<32>(out)<<std::endl;
		return out;
	}});
	*/
	/*
	std::ofstream fout("b.txt");
	if(!fout) {return 1;}
	LogicGenerator::generateROM(fout,std::function{[](RegPair::IN::type in){
		RegPair::OUT::type out{0};
		std::cout<<std::bitset<9>(in);
		out=RegPair::OUT::run(out,in);
		std::cout<<"=>"<<std::bitset<8>(out)<<std::endl;
		return out;
	}});*/
	/*Util::bitset_wrap<5> a{3};
	Util::bitset_wrap<5> b{3};
	std::cout << (a==b) << std::endl;*/

	return 0;
}
