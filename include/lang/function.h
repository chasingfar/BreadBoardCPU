//
// Created by chasingfar on 2021/8/12.
//

#ifndef BBCPU_FUNCTION_H
#define BBCPU_FUNCTION_H

#include <utility>
#include "statement.h"
namespace BBCPU::Lang{
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
			local_size+=size;
			return LocalVar::make(size,1-local_size);
		}
	};

	template<typename FnType> struct Fn;
	template<typename Ret,typename ...Args>
	struct Fn<Ret(Args...)>:FnBase<Ret(Args...)>{
		using This = Fn<Ret(Args...)>;
		using Base = FnBase<Ret(Args...)>;
		static constexpr offset_t ret_start=Base::arg_size+4+1;
		static constexpr offset_t arg_start=Base::arg_size+4;
		explicit Fn(const std::string& name=""):Base{name,ret_start,arg_start}{}

		Ret operator()(const Args&... args_) const{
			Code code{};
			code<<adj(-Base::ret_size);
			if constexpr (sizeof...(Args)>0){
				(code<<...<<args_);
			}
			code<<Ops::call(this->start)
				<<adj(Base::arg_size);
			return Ret{expr(code)};
		}
		This& impl(const Stmt& stmt){
			this->body<<asm_({ent(this->local_size), stmt});
			return *this;
		}

		template<typename F>requires std::is_invocable_r_v<Stmt , F, This&, Args...>
		Block impl(F&& fn){
			auto body=std::apply(fn,std::tuple_cat(std::forward_as_tuple(*this),this->args));
			return impl(body);
		}
		inline void_ return_(Ret value){return asm_({this->ret.set(value), lev()});}
		inline void_ return_(){return asm_(lev());}

		explicit Fn(const Stmt& stmt){impl(stmt);}
		template<typename F>requires std::is_invocable_r_v<Stmt , F, This&, Args...>
		explicit Fn(F&& fn):Base{ret_start,arg_start}{impl(fn);}
		auto operator&(){
			return ptr<This>(expr(push(this->start)));
		}
	};

	template<typename Ret,typename ...Args>
	struct ptr<Fn<Ret(Args...)>>: AsInt<addr_t>{
		DEF_TYPE(ptr,(ptr<Fn<Ret(Args...)>>),(AsInt<addr_t>)) // NOLINT(google-explicit-constructor)

		explicit ptr(const usize& v): Base(v.value){}
		template<typename U>
		explicit ptr(const ptr<U>& v):Base(v.value){}
		explicit ptr(const Label& v):Base(Raw::make({v.get_lazy(0),v.get_lazy(1)})){}

		using type=Fn<Ret(Args...)>;
		Ret operator()(const Args&... args_) const{
			Code code{};
			code<<adj(-type::ret_size);
			if constexpr (sizeof...(Args)>0){
				(code<<...<<args_);
			}
			code<<value->load()
				<<call_ptr()
				<<adj(type::arg_size);
			return Ret{expr(code)};
		}
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
		static constexpr offset_t ret_start=Base::arg_size+2+1-Base::ret_size;
		static constexpr offset_t arg_start=Base::arg_size+2;
		Label end_;

		template<typename F>requires std::is_invocable_r_v<Stmt , F, This&, Args...>
		This& impl(F&& fn){
			auto stmt=std::apply(fn,std::tuple_cat(std::forward_as_tuple(*this),this->args));
			this->body<<asm_({
				saveBP(),
				adj(-this->local_size),
				stmt,
				end_,
				loadBP(),
				adj(Base::arg_size-Base::ret_size),
			});
			return *this;
		}
		inline void_ return_(Ret value){return asm_({this->ret.set(value), jmp(end_)});}
		inline void_ return_(){return asm_(jmp(end_));}

		template<typename F>requires std::is_invocable_r_v<Stmt , F, This&, Args...>
		explicit InplaceFn(F&& fn):Base{ret_start,arg_start}{impl(fn);}
	};
}
#endif //BBCPU_FUNCTION_H
