//
// Created by chasingfar on 2021/8/12.
//

#ifndef BREADBOARDCPU_FUNCTION_H
#define BREADBOARDCPU_FUNCTION_H

#include <utility>
#include "statement.h"
namespace BreadBoardCPU::ASM::Function{
/*
int16 sub_function(int8 arg1, int16 arg2, int8 arg3);

|    ....       | high address
+---------------+
|     low       |    new BP + 10
+ return value  +
|     high      |    new BP + 9
+---------------+
| arg1          |    new BP + 8
+---------------+
|     low       |    new BP + 7
+ arg2          +
|     high      |    new BP + 6
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

	template<typename Ret,typename ...Args>
	struct Fn:Block{
		offset_t local_size{0};
		std::tuple<LocalVar<Args>...> args{localvars<Args...>((4+...+Args::size))};
		LocalVar<Ret> ret{Ret::size+(4+...+Args::size)};
		explicit Fn(const std::string& name=""){ start.name=name;}

		template<typename ...Ts>
		auto local(){
			auto vars=localvars<Ts...>(-local_size);
			local_size+=(Ts::size+...+0);
			return vars;
		}

		Expr<Ret> operator()(const Value<Args>&... _args) const{
			Expr<Ret> expr{};
			expr<<adj(-Ret::size);
			(expr<<...<<_args);
			expr<<Ops::call(start)
				<<adj((Args::size+...+0));
			return expr;
		}
		Fn<Ret,Args...>& impl(const code_t& code){
			body=code_t{start,ent(local_size),code};
			return *this;
		}

		template<typename ...Ts,typename F>requires std::is_invocable_r_v<code_t , F, LocalVar<Args>...,LocalVar<Ts>...>
		Block impl(F&& fn){
			code_t body=std::apply(fn,std::tuple_cat(args,local<Ts...>()));
			return impl(body);
		}
		operator code_t(){
			return body;
		}
		inline code_t _return(const Value<Ret>& value){return {ret.set(value),lev()};}

	private:
		template<typename Var,typename ...Rest>
		std::tuple<LocalVar<Var>,LocalVar<Rest>...> _localvars(offset_t start) const{
			LocalVar<Var> var{start};
			if constexpr (sizeof...(Rest)==0){
				return std::make_tuple(var);
			}else{
				return std::tuple_cat(std::make_tuple(var), localvars<Rest...>(start-Var::size));
			}
		}
		template<typename ...Var>
		std::tuple<LocalVar<Var>...> localvars(offset_t start) const{
			if constexpr (sizeof...(Var)==0){
				return std::make_tuple();
			}else{
				return _localvars<Var...>(start);
			}
		}
	};
}
#endif //BREADBOARDCPU_FUNCTION_H
