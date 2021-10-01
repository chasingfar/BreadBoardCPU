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
	inline static auto _local_vars(offset_t start=0) {
		Var var{LocalVar::make(Var::size,start)};
		if constexpr (sizeof...(Rest)==0){
			return std::tuple{var};
		}else{
			return std::tuple_cat(std::tuple{var}, _local_vars<Rest...>(start-Var::size));
		}
	}
	template<typename ...Var>
	inline static std::tuple<Var...> local_vars(offset_t start=0) {
		if constexpr (sizeof...(Var)==0){
			return std::tuple{};
		}else{
			return _local_vars<Var...>(start);
		}
	}
	template<typename Ret,typename ...Args>
	struct FnBase:Block{
		static constexpr offset_t ret_size=Ret::size;
		static constexpr offset_t arg_size=(0+...+Args::size);
		offset_t local_size{0};
		Ret ret;
		std::tuple<Args...> args;
		explicit FnBase(offset_t ret_start,offset_t arg_start):
			ret{LocalVar::make(Ret::size,ret_start)},
			args{local_vars<Args...>(arg_start)}
			{}
		explicit FnBase(const std::string& name,offset_t ret_start,offset_t arg_start):
			FnBase(ret_start,arg_start)
			{ start.name=name;}
		template<typename ...Ts>
		auto let(std::tuple<Ts...> vars){
			local_size+=(Ts::size+...+0);
			return vars;
		}
		template<typename ...Ts>
		auto local(){
			return let(local_vars<Ts...>(-local_size));
		}
	};
	template<typename Ret,typename ...Args>
	struct Fn:FnBase<Ret,Args...>{
		using This = Fn<Ret,Args...>;
		using Base = FnBase<Ret,Args...>;
		static constexpr offset_t ret_start=Base::ret_size+Base::arg_size+4;
		static constexpr offset_t arg_start=Base::arg_size+4;
		explicit Fn(const std::string& name=""):Base{name,ret_start,arg_start}{}

		Ret operator()(const Args&... _args) const{
			code_t expr{};
			expr<<adj(-Base::ret_size);
			if constexpr (sizeof...(Args)>0){
				(expr<<...<<_args);
			}
			expr<<Ops::call(this->start)
				<<adj(Base::arg_size);
			return Ret{expr};
		}
		Fn<Ret,Args...>& impl(const code_t& code){
			this->body=code_t{ent(this->local_size),code};
			return *this;
		}

		template<typename F>requires std::is_invocable_r_v<code_t , F, This&, Args...>
		Block impl(F&& fn){
			code_t body=std::apply(fn,std::tuple_cat(std::forward_as_tuple(*this),this->args));
			return impl(body);
		}
		inline code_t _return(Ret value){return {this->ret.set(value),lev()};}
		inline code_t _return(){return {lev()};}

		explicit Fn(const code_t& code){impl(code);}
		template<typename F>requires std::is_invocable_r_v<code_t , F, This&, Args...>
		explicit Fn(F&& fn):Base{ret_start,arg_start}{impl(fn);}
	};

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
	template<typename Ret,typename ...Args>
	struct InplaceFn:FnBase<Ret,Args...>{
		using This = InplaceFn<Ret,Args...>;
		using Base = FnBase<Ret,Args...>;
		static constexpr offset_t ret_start=Base::arg_size+2;
		static constexpr offset_t arg_start=Base::arg_size+2;
		Label _end;

		template<typename F>requires std::is_invocable_r_v<code_t , F, This&, Args...>
		InplaceFn<Ret,Args...>& impl(F&& fn){
			code_t code=std::apply(fn,std::tuple_cat(std::forward_as_tuple(*this),this->args));
			this->body=code_t{
					saveBP(),
					adj(-this->local_size),
					code,
					_end,
					loadBP(),
					adj(Base::arg_size-Base::ret_size),
			};
			return *this;
		}
		inline code_t _return(Ret value){return {this->ret.set(value),jmp(_end)};}
		inline code_t _return(){return {jmp(_end)};}

		template<typename F>requires std::is_invocable_r_v<code_t , F, This&, Args...>
		explicit InplaceFn(F&& fn):Base{ret_start,arg_start}{impl(fn);}
	};
}
#endif //BBCPU_FUNCTION_H
