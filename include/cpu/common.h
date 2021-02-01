//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_CPU_COMMON_H
#define BREADBOARDCPU_CPU_COMMON_H
#include "../util.h"
#include "../Bitwise.h"
#include <functional>
inline std::vector<std::string> logs{};
enum struct LogState{
	Start,Step,Stop
};
inline LogState state = LogState::Stop;
template<typename T0,typename... T>
std::string log_arg(T0 v0, T... value){
	std::stringstream ss;
	ss<<"("<<v0<<((ss << ',' << value), ...,")");
	return ss.str();
}
struct Logger{
	explicit Logger(const std::string& str){
		logs.emplace_back(str);
		if(state==LogState::Step){print();stop();}
	}
	~Logger(){
		logs.pop_back();
	}
	static void print(){
		for(const auto& log:logs){
			std::cout<<":"<<log;
		}
	}
	static void start(){state=LogState::Start;}
	static void stop(){state=LogState::Stop;}
	static void step(){state=LogState::Step;}
};
#define LOG(...) Logger __{__func__+log_arg(__VA_ARGS__)};
#define LOG_START() Logger::start();
#define LOG_STOP() Logger::stop();
#define LOG_STEP() Logger::step();
namespace BreadBoardCPU {
	using namespace Util::Bitwise::BitField;
	using Util::TruthTable;
	using Util::ROM;
	using Util::generateROM;
}
#endif //BREADBOARDCPU_CPU_COMMON_H
