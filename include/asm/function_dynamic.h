//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_FUNCTION_DYNAMIC_H
#define BREADBOARDCPU_FUNCTION_DYNAMIC_H
#include "advance.h"
namespace BreadBoardCPU::ASM::DynamicFn{
	struct FnLocal{
		int8_t offset=0;
		FnLocal()= default;
		explicit FnLocal(int8_t offset):offset(offset){}
		code_t load(Reg to){
			return load_local(offset,to);
		}
		code_t save(Reg value){
			return save_local(offset,value);
		}
	};
	struct FnArg:FnLocal{
		const size_t N;
		FnArg(size_t N,int8_t i):N{N},FnLocal(N-i+4){}
	};
	struct FnVar:FnLocal{
		FnVar(int8_t i):FnLocal(-i){}
	};
	struct FnDecl{
		Label start;
		const size_t ArgNum;
		FnDecl(std::string name,size_t ArgNum):start(std::move(name)),ArgNum(ArgNum){}
		code_t call(){
			return {Ops::call(start),adj(ArgNum)};
		}
		code_t call(std::vector<std::variant<Reg,code_t>> arg){
			code_t codes{};
			for (size_t i = 0; i < ArgNum; ++i) {
				if(auto reg=std::get_if<Reg>(&arg[i])){
					codes<<push(*reg);
				}
				if(auto code=std::get_if<code_t>(&arg[i])){
					codes<<*code;
				}
			}
			return codes<<call();
		}
	};
}
#endif //BREADBOARDCPU_FUNCTION_DYNAMIC_H
