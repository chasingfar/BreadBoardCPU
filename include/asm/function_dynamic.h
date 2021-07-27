//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_FUNCTION_DYNAMIC_H
#define BREADBOARDCPU_FUNCTION_DYNAMIC_H
#include "advance.h"
namespace BreadBoardCPU::ASM::DynamicFn{
/*
sub_function(arg1, arg2, arg3);

|    ....       | high address
+---------------+
| arg1          |    new BP + 7
+---------------+
| arg2          |    new BP + 6
+---------------+
| arg3          |    new BP + 5
+---------------+
|     low       |    new BP + 4
+return address +
|     high      |    new BP + 3
+---------------+
|     low       |    new BP + 2
+ old BP        +
|     high      |    new BP + 1
+---------------+
| local var 1   | <- new BP
+---------------+
| local var 2   |    new BP - 1
+---------------+
|    ....       |  low address

*/
	struct FnVar: Var{
		offset_t offset=0;
		explicit FnVar(offset_t& _offset)
		:offset(_offset),Var{load_local(_offset), save_local(_offset)}{
			_offset-=1;
		}
	};
	using FnArg=FnVar;
	struct FnDecl{
		Label start;
		const offset_t ArgNum;
		offset_t offset;

		explicit FnDecl(addr_t ArgNum=0):FnDecl("",ArgNum){}
		explicit FnDecl(std::string name,addr_t ArgNum=0)
		:start(std::move(name)),ArgNum(ArgNum),offset(ArgNum+4){}

		template<typename Var,typename ...Rest>
		std::tuple<Var,Rest...> getVars(){
			if(offset==4) {
				offset = 0;//jump over return addr(2byte)+old BP(2byte)
			}
			std::tuple<Var> var{offset};
			if constexpr (sizeof...(Rest)==0){
				return var;
			}else{
				return std::tuple_cat(var, getVars<Rest...>());
			}
		}
		code_t call() const{
			return {Ops::call(start),adj(ArgNum)};
		}
		code_t call(std::vector<std::variant<Reg,code_t>> arg) const{
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
		code_t impl(code_t body,bool protect= false){
			code_t fn{start,ent(1-offset),body};
			Label end;
			return protect?code_t{jmp(end),fn,lev(),end}:fn;
		}
	};
}
#endif //BREADBOARDCPU_FUNCTION_DYNAMIC_H
