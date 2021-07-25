//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_FUNCTION_DYNAMIC_H
#define BREADBOARDCPU_FUNCTION_DYNAMIC_H
#include "advance.h"
namespace BreadBoardCPU::ASM::DynamicFn{
	struct FnLocal{
		offset_t offset=0;
		FnLocal()= default;
		explicit FnLocal(offset_t offset):offset(offset){}
		code_t load(Reg to){
			return load_local(offset,to);
		}
		code_t save(Reg value){
			return save_local(offset,value);
		}
	};
	struct FnDecl{
		using locals_t = std::unordered_map<std::string,FnLocal>;
		Label start;
		const locals_t args;
		locals_t vars;
		FnDecl(std::string name,const std::vector<std::string>& arg_names):start(std::move(name)),args(make_args(arg_names)){}
		code_t call(){
			return {Ops::call(start),adj(args.size())};
		}
		code_t call(std::vector<std::variant<Reg,code_t>> arg){
			code_t codes{};
			for (size_t i = 0; i < args.size(); ++i) {
				if(auto reg=std::get_if<Reg>(&arg[i])){
					codes<<push(*reg);
				}
				if(auto code=std::get_if<code_t>(&arg[i])){
					codes<<*code;
				}
			}
			return codes<<call();
		}
		code_t impl(code_t body,bool protect= false){
			code_t fn{start,ent(vars.size()),body};
			Label end;
			return protect?code_t{jmp(end),fn,lev(),end}:fn;
		}
		FnLocal operator[](const std::string& name){
			if (args.contains(name)){
				return args.at(name);
			}
			return vars.emplace(name,-static_cast<int16_t>(vars.size())).first->second;
		}
	private:
		static locals_t make_args(const std::vector<std::string>& arg_names){
			locals_t tmp;
			int16_t i = 0;
			for (const auto& name:arg_names) {
				tmp.emplace(name,arg_names.size()-i+4);
				++i;
			}
			return tmp;
		}
	};
}
#endif //BREADBOARDCPU_FUNCTION_DYNAMIC_H
