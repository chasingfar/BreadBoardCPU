//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_STATEMENT_H
#define BBCPU_STATEMENT_H

#include <utility>
#include "type.h"

namespace BBCPU::ASM {
	using stmt_t=std::variant<Label,void_>;
	using stmts_t=std::initializer_list<stmt_t>;

	inline code_t from_stmt(const stmt_t& stmt){
		return std::visit(Util::lambda_compose{
				[&](Label v){return code_t{v};},
				[&](const void_& v){return v.to_code();},
		},stmt);
	}
	inline code_t from_stmt(stmts_t stmts){
		code_t code{};
		for(const auto& stmt:stmts){
			code<<from_stmt(stmt);
		}
		return code;
	}
	template<typename T>
	concept CanToCode=requires (T x){x.to_code();};
	template<typename T>
	concept CanMakeCode=requires (T x){code_t{x};};
	template<typename T>
	concept ProgramCode=CanMakeCode<T>||CanToCode<T>;
	struct Program{
		code_t code{};
		template<ProgramCode... Ts>
		Program(Ts... codes):code{to_code(codes)...}{}
		code_t to_code(const CanToCode auto& v){
			return v.to_code();
		}
		code_t to_code(const CanMakeCode auto& v){
			return code_t{v};
		}
		operator code_t() const{return code;}
	};

	struct Block{
		Label start;
		code_t body{};
		Label end;
		Block()=default;
		Block(std::initializer_list<std::variant<stmts_t,stmt_t,Block>> stmts){
			for(const auto& stmt:stmts){
				body<<std::visit(Util::lambda_compose{
					[](      stmts_t v){return from_stmt(v);},
					[](const stmt_t& v){return from_stmt(v);},
					[](const  Block& v){return v.to_code();},
				},stmt);
			}
		}

		code_t to_code() const{
			return {start,body,end};
		}
		code_t to_protect() const{
			return {jmp(end),to_code()};
		}
	};
	template<typename Ret=void_>
	struct if_{
		using ret_stmts_t = std::conditional_t<std::is_same_v<Ret,void_>,stmts_t,const Ret&>;
		bool_ cond;
		Block if_true{};
		Block if_false{};
		explicit if_(const bool_& cond):cond(cond){}
		if_& then(ret_stmts_t t_code){
			if constexpr(std::is_same_v<Ret,void_>){
				if_true.body=from_stmt(t_code);
			}else{
				if_true.body=t_code.to_code();
			}
			return *this;
		}
		if_& else_(ret_stmts_t f_code){
			if constexpr(std::is_same_v<Ret,void_>){
				if_false.body=from_stmt(f_code);
			}else{
				if_false.body=f_code.to_code();
			}
			return *this;
		}
		Ret end() const{
			return Ret{to_code()};
		}
		code_t to_code() const{
			return {
				cond.to_code(),
				brz(if_false.start),
				if_true.to_code(),
				jmp(if_false.end),
				if_false.to_code(),
			};
		}
	};
	template<typename Ret=void_>
	struct ifc{
		using ret_stmts_t = std::conditional_t<std::is_same_v<Ret,void_>,stmts_t,const Ret&>;
		void_ cond;
		Block if_no_carry{};
		Block if_carry{};
		explicit ifc(const void_& cond):cond(cond){}
		ifc& then(ret_stmts_t no_carry){
			if constexpr(std::is_same_v<Ret,void_>){
				if_no_carry.body=from_stmt(no_carry);
			}else{
				if_no_carry.body=no_carry.to_code();
			}
			if_no_carry.body=no_carry.to_code();
			return *this;
		}
		ifc& else_(ret_stmts_t carry){
			if constexpr(std::is_same_v<Ret,void_>){
				if_carry.body=from_stmt(carry);
			}else{
				if_carry.body=carry.to_code();
			}
			return *this;
		}
		Ret end() const{
			return Ret{to_code()};
		}
		code_t to_code() const {
			return {
				cond.to_code(),
				brc(if_carry.start),
				if_no_carry.to_code(),
				jmp(if_carry.end),
				if_carry.to_code(),
			};
		}
	};
	struct while_{
		bool_ cond;
		Block body{};
		explicit while_(const bool_& cond):cond(cond){}
		while_& do_(stmts_t code){
			body.body=from_stmt(code);
			return *this;
		}
		void_ end() const{
			return void_{to_code()};
		}
		code_t to_code() const {
			Label start,end;
			return {
				start,
				cond.to_code(),
				brz(end),
				body.to_code(),
				jmp(start),
				end,
			};
		}
	};
	struct StaticVars:Block,PresetAllocator<code_t>{
		std::shared_ptr<MemVar> alloc_preset(addr_t size,code_t value) override {
			auto var=StaticVar::make(size, start, static_cast<offset_t>(data_size(body)));
			body.insert(body.end(),value.begin(),value.end());
			for(addr_t i=data_size(value);i<size;++i){
				body.emplace_back(static_cast<op_t>(0));
			}
			return var;
		}
	};
}
#endif //BBCPU_STATEMENT_H
