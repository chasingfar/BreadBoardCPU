//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_FUNCTION_STATIC_H
#define BREADBOARDCPU_FUNCTION_STATIC_H
#include "advance.h"

namespace BreadBoardCPU::ASM::StaticFn{
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
	template<size_t N>
	struct FnArg:FnLocal{
		explicit FnArg(int8_t i):FnLocal(N-i+4){}
	};
	struct FnVar:FnLocal{
		explicit FnVar(int8_t i):FnLocal(-i){}
	};
	struct FnVars{
		std::unordered_map<std::string,FnVar> vars;
		size_t size(){return vars.size();}
		FnVar operator[](const std::string& name){
			return vars.emplace(name,vars.size()).first->second;
		}
	};
	template<size_t ArgNum>
	struct FnDecl{
		Label start;
		explicit FnDecl(std::string name):start(std::move(name)){}
		code_t call(){
			return {Ops::call(start),adj(ArgNum)};
		}
		code_t call(std::array<std::variant<Reg,code_t>,ArgNum> args){
			code_t codes{};
			for (auto arg:args) {
				if(auto reg=std::get_if<Reg>(&arg)){
					codes<<push(*reg);
				}
				if(auto code=std::get_if<code_t>(&arg)){
					codes<<*code;
				}
			}
			return codes<<call();
		}

		template<typename F>
		Block impl(F&& fn){
			FnVars vars;
			code_t body=_impl(std::forward<F>(fn),vars,std::make_integer_sequence<int8_t,ArgNum>{});
			return {start,{ent(vars.size()),body}};
		}
	private:
		template<typename F,int8_t ...I>
		code_t _impl(F&& fn,FnVars& vars,std::integer_sequence<int8_t,I...>){
			return std::forward<F>(fn)(vars,FnArg<sizeof...(I)>{I}...);
		}
	};
}
#endif //BREADBOARDCPU_FUNCTION_STATIC_H
