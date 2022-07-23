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
	template<typename FnType> struct FnBase;
	template<typename Ret,typename ...Args>
	struct FnBase<Ret(Args...)>:Block,Allocator{
		static constexpr offset_t ret_size=Ret::size;
		static constexpr offset_t arg_size=(0+...+Args::size);
		offset_t local_size;
		Ret ret;
		std::tuple<Args...> args;
		explicit FnBase(offset_t ret_start,offset_t arg_start):
			local_size(-arg_start),
			ret{LocalVar::make(Ret::size,ret_start)},
			args{vars<Args...>()}
			{local_size=0;}
		explicit FnBase(const std::string& name,offset_t ret_start,offset_t arg_start):
			FnBase(ret_start,arg_start)
			{ start.name=name;}
		std::shared_ptr<MemVar> alloc(addr_t size) override {
			offset_t pos=-local_size;
			local_size+=size;
			return LocalVar::make(size,pos);
		}
	};

	template<typename FnType> struct Fn;
	template<typename Ret,typename ...Args>
	struct Fn<Ret(Args...)>:FnBase<Ret(Args...)>{
		using This = Fn<Ret(Args...)>;
		using Base = FnBase<Ret(Args...)>;
		static constexpr offset_t ret_start=Base::ret_size+Base::arg_size+4;
		static constexpr offset_t arg_start=Base::arg_size+4;
		explicit Fn(const std::string& name=""):Base{name,ret_start,arg_start}{}

		Ret operator()(const Args&... _args) const{
			code_t expr{};
			expr<<adj(-Base::ret_size);
			if constexpr (sizeof...(Args)>0){
				(expr<<...<<_args.to_code());
			}
			expr<<Ops::call(this->start)
				<<adj(Base::arg_size);
			return Ret{expr};
		}
		This& impl(const Block& stmt){
			this->body=code_t{ent(this->local_size),stmt.to_code()};
			return *this;
		}

		template<typename F>requires std::is_invocable_r_v<Block , F, This&, Args...>
		Block impl(F&& fn){
			auto body=std::apply(fn,std::tuple_cat(std::forward_as_tuple(*this),this->args));
			return impl(body);
		}
		inline void_ return_(Ret value){return void_{code_t{this->ret.set(value).to_code(), lev()}};}
		inline void_ return_(){return void_{lev()};}

		explicit Fn(const code_t& code){impl(code);}
		template<typename F>requires std::is_invocable_r_v<Block , F, This&, Args...>
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
	template<typename FnType> struct InplaceFn;
	template<typename Ret,typename ...Args>
	struct InplaceFn<Ret(Args...)>:FnBase<Ret(Args...)>{
		using This = InplaceFn<Ret(Args...)>;
		using Base = FnBase<Ret(Args...)>;
		static constexpr offset_t ret_start=Base::arg_size+2;
		static constexpr offset_t arg_start=Base::arg_size+2;
		Label end_;

		template<typename F>requires std::is_invocable_r_v<Block , F, This&, Args...>
		This& impl(F&& fn){
			auto stmt=std::apply(fn,std::tuple_cat(std::forward_as_tuple(*this),this->args));
			this->body=code_t{
					saveBP(),
					adj(-this->local_size),
					stmt.to_code(),
					end_,
					loadBP(),
					adj(Base::arg_size-Base::ret_size),
			};
			return *this;
		}
		inline void_ return_(Ret value){return void_{code_t{this->ret.set(value).to_code(), jmp(end_)}};}
		inline void_ return_(){return void_{jmp(end_)};}

		template<typename F>requires std::is_invocable_r_v<Block , F, This&, Args...>
		explicit InplaceFn(F&& fn):Base{ret_start,arg_start}{impl(fn);}
	};
}
#endif //BBCPU_FUNCTION_H
