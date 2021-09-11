//
// Created by chasingfar on 2021/8/12.
//

#ifndef BBCPU_FUNCTION_H
#define BBCPU_FUNCTION_H

#include <utility>
#include "statement.h"
namespace BBCPU::ASM::Function{
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
	template<typename Var,typename ...Rest>
	inline static auto get_local(offset_t start=0) {
		Var var{LocalVar::make(Var::size,start)};
		if constexpr (sizeof...(Rest)==0){
			return std::tuple{var};
		}else{
			return std::tuple_cat(std::tuple{var}, get_local<Rest...>(start-Var::size));
		}
	}
	struct FnBase:Block{
		template<typename ...Var>
		static std::tuple<Var...> local_vars(offset_t start=0) {
			if constexpr (sizeof...(Var)==0){
				return std::tuple{};
			}else{
				return get_local<Var...>(start);
			}
		}
	};
	template<typename Ret,typename ...Args>
	struct Fn:FnBase{
		offset_t local_size{0};
		std::tuple<Args...> args{local_vars<Args...>((4+...+Args::size))};
		Ret ret{LocalVar::make(Ret::size,Ret::size+(4+...+Args::size))};
		explicit Fn(const std::string& name=""){ start.name=name;}

		template<typename ...Ts>
		auto local(){
			auto vars=local_vars<Ts...>(-local_size);
			local_size+=(Ts::size+...+0);
			return vars;
		}

		Ret operator()(const Args&... _args) const{
			code_t expr{};
			expr<<adj(-Ret::size);
			if constexpr (sizeof...(Args)>0){
				(expr<<...<<_args);
			}
			expr<<Ops::call(start)
				<<adj((Args::size+...+0));
			return Ret{expr};
		}
		Fn<Ret,Args...>& impl(const code_t& code){
			body=code_t{ent(local_size),code};
			return *this;
		}

		template<typename ...Ts,typename F>requires std::is_invocable_r_v<code_t , F, Args...,Ts...>
		Block impl(F&& fn){
			code_t body=std::apply(fn,std::tuple_cat(args,local<Ts...>()));
			return impl(body);
		}
		template<typename ...Ts,typename F>requires std::is_invocable_r_v<code_t , F, std::function<code_t(Ret)>, Args...,Ts...>
		Block impl(F&& fn){
			code_t body=std::apply(fn,std::tuple_cat(
				std::tuple{[&](Ret value){return _return(value);}},
				args,
				local<Ts...>()
			));
			return impl(body);
		}
		template<typename ...Ts,typename F>requires std::is_invocable_r_v<code_t , F, code_t, Ret, Args..., Ts...>
		Block impl(F&& fn){
			code_t body=std::apply(fn,std::tuple_cat(
					std::tuple{_return(),ret},
					args,
					local<Ts...>()
			));
			return impl(body);
		}
		inline code_t _return(Ret value){return {ret.set(value),lev()};}
		inline code_t _return(){return {lev()};}

		explicit Fn(const code_t& code){impl(code);}
		template<typename ...Ts,typename F>requires std::is_invocable_r_v<code_t , F, Args...,Ts...>
		explicit Fn(std::tuple<Ts...> vars,F&& fn){impl<Ts...>(fn);}
		template<typename ...Ts,typename F>requires std::is_invocable_r_v<code_t , F, std::function<code_t(const Ret&)>, Args...,Ts...>
		explicit Fn(std::tuple<Ts...> vars,F&& fn){impl<Ts...>(fn);}
		template<typename ...Ts,typename F>requires std::is_invocable_r_v<code_t , F, code_t, Ret, Args...,Ts...>
		explicit Fn(std::tuple<Ts...> vars,F&& fn){impl<Ts...>(fn);}
/*

int16 sub_function(int8 arg1, int16 arg2, int8 arg3);

|    ....       | high address
+---------------+---------------+
| arg1          |     low       |    new BP + 6
+---------------+ return value  +
|     low       |     high      |    new BP + 5
+ arg2          +---------------+
|     high      |    new BP + 4
+---------------+
| arg3          |    new BP + 3
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
		template<typename ...Ts,typename F>
		requires std::is_invocable_r_v<code_t , F,
			const Label&,
			Ret,
			Args...,
			Ts...
		>
		inline static Ret inplace(F&& fn){
			Label end;
			return Ret{code_t{
				saveBP(),
				adj(-(Ts::size+...+0)),
				std::apply(fn,std::tuple_cat(
					std::tuple{end},
					local_vars<Ret>((Args::size+...+2)),
					local_vars<Args...>((Args::size+...+2)),
					local_vars<Ts...>(0)
				)),
				end,
				loadBP(),
				adj((Args::size+...+0)-Ret::size),
			}};
		}
		template<typename ...Ts,typename F>
		requires std::is_invocable_r_v<code_t , F,
			std::function<code_t(Ret)>,
			Args...,
			Ts...
		>
		inline static Ret inplace(F&& fn){
			return inplace<Ts...>([&](
				const Label& _end,
				Ret _ret,
				Args... _args,
				Ts... _vars){
					return fn(
						[=](Ret value)->code_t{
							return {_ret.set(value),jmp(_end)};
						},
						_args...,
						_vars...
					);
			});
		}
	};	
}
#endif //BBCPU_FUNCTION_H
