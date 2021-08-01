//
// Created by chasingfar on 2021/2/2.
//

#ifndef BREADBOARDCPU_FUNCTION_DYNAMIC_H
#define BREADBOARDCPU_FUNCTION_DYNAMIC_H
#include <utility>
#include "advance.h"
namespace BreadBoardCPU::ASM::DynamicFn{
/*
sub_function(arg1, arg2, arg3);

|    ....       | high address
+---------------+
| return value  |    new BP + 8
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
	template<addr_t _Size=0,bool Signed=false>
	struct FnVar: Var{
		inline static addr_t Size=_Size;
		offset_t offset=0;
		explicit FnVar(offset_t offset,addr_t size=_Size,bool is_signed=Signed)
		:offset(offset),Var{size,{},{},is_signed}{
			for(addr_t i = 0; i < size; ++i) {
				push<<load_local(offset-i);
				pop<<save_local(offset-(size-1-i));
			}
		}
	};
	using FnU8=FnVar<1,false>;

	template<typename Ret=FnVar<0,false> >
	struct FnDecl{
		Label start;
		const addr_t RetSize;
		const addr_t ArgSize;
		offset_t offset;
		Ret ret;

		explicit FnDecl(addr_t RetSize=0,addr_t ArgSize=0):FnDecl("",RetSize,ArgSize){}
		explicit FnDecl(std::string name,addr_t RetSize=Ret::Size,addr_t ArgSize=0)
		:start(std::move(name)),RetSize(RetSize),ArgSize(ArgSize),offset(ArgSize+4),ret(RetSize+ArgSize+4,RetSize){}

		template<typename Var,typename ...Rest>
		std::tuple<Var,Rest...> getVars(){
			if(offset==4) {
				offset = 0;//jump over return addr(2byte)+old BP(2byte)
			}
			std::tuple<Var> var{offset};
			offset-=std::get<0>(var).size;
			if constexpr (sizeof...(Rest)==0){
				return var;
			}else{
				return std::tuple_cat(var, getVars<Rest...>());
			}
		}
		code_t call(const std::vector<RValue*>& args) const{
			code_t codes{adj(-RetSize)};
			for(const auto& arg:args){
				codes<<arg->push;
			}
			return codes<<Ops::call(start)<<adj(ArgSize);
		}

		template<typename ...Args>
		RValue operator()(Args...args) const{
			code_t codes{adj(-RetSize)};
			(codes<<...<<args.push)<<Ops::call(start)<<adj(ArgSize);
			return {RetSize,codes,ret.is_signed};
		}
		code_t impl(code_t body,bool protect= false){
			code_t fn{start,ent(1-offset),body};
			Label end;
			return protect?code_t{jmp(end),fn,lev(),end}:fn;
		}
		inline code_t _return(const RValue& value){return {ret.set(value),lev()};}
	};
}
#endif //BREADBOARDCPU_FUNCTION_DYNAMIC_H
