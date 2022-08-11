//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_STATEMENT_H
#define BBCPU_STATEMENT_H

#include <utility>
#include "type.h"

namespace BBCPU::Lang {
	struct Stmt{
		using type=std::variant<Label,void_,Stmt>;
		std::vector<type> stmts;
		Stmt(std::initializer_list<type> stmts):stmts{stmts}{}
		Code to_code() const{
			Code code{};
			for(const auto& stmt:stmts){
				code<<std::visit(Util::lambda_compose{
					[](const Label& v){return Code{v};},
					[](const void_& v){return v.to_code();},
					[](const  Stmt& v){return v.to_code();},
				},stmt);
			}
			return code;
		}
		Stmt& operator <<(const type& stmt){
			stmts.push_back(stmt);
			return *this;
		}
	};
	struct Block{
		Label start;
		Stmt body{};
		Label end;
		Block()=default;
		Block(std::initializer_list<Stmt> stmts){
			for(const auto& stmt:stmts){
				body<<stmt;
			}
		}

		Code to_code() const{
			return {start,body,end};
		}
		Code to_protect() const{
			return {jmp(end),to_code()};
		}
	};
	template<typename Ret=Stmt,typename Cond=bool_,Code (*Br)(const Label&)=Ops::brz>
	struct IF;
	template<typename Ret=Stmt,typename Cond=bool_,Code (*Br)(const Label&)=Ops::brz>
	struct if_{
		std::vector<std::pair<Cond,Block>> conds;
		explicit if_(const Cond& cond):conds{{cond,{}}}{}
		explicit if_(const std::vector<std::pair<Cond,Block>>& conds):conds{conds}{}
		auto then(Ret t_code){
			conds.back().second.body<<asm_(t_code);
			return IF<Ret,Cond,Br>(conds);
		}
	};
	template<typename Ret,typename Cond,Code (*Br)(const Label&)>
	struct IF{
		std::vector<std::pair<Cond,Block>> conds;
		Block else_block{};
		explicit IF(const std::vector<std::pair<Cond,Block>>& conds):conds{conds}{}
		auto elif(const Cond& cond){
			conds.emplace_back(cond,Block{});
			return if_<Ret,Cond,Br>(conds);
		}
		Ret else_(Ret f_code){
			else_block.body<<asm_(f_code);
			return end();
		}
		Ret end() const{
			if constexpr(std::is_same_v<Ret,Stmt>){
				return Stmt{asm_(to_code())};
			}else{
				return Ret{expr(to_code())};
			}
		}
		Code to_code() const{
			Label end;
			Code code{};
			if(!conds.empty()){
				for(auto it=conds.begin();it!=conds.end()-1;++it){
					auto [cond,block]=*it;
					Label next;
					code<<Code{
						cond,
						Br(next),
						block,
						jmp(end),
						next,
					};
				}
				auto [cond,block]=conds.back();
				if(else_block.body.stmts.empty()){
					return code<<Code{
						cond,
						Br(end),
						block,
						end,
					};
				}else{
					return code<<Code{
						cond,
						Br(else_block.start),
						block,
						jmp(end),
						else_block,
						end,
					};
				}
			}
			return {};
		}
	};
	
	template<typename Ret=Stmt>
	using ifc=if_<Ret,void_,Ops::brc>;

	struct LoopStmt{
		void_ break_,continue_;
	};
	struct while_{
		Label start_;
		bool_ cond;
		Block body{};
		Label end_;
		explicit while_(const bool_& cond):cond(cond){}
		while_& do_(Stmt code){
			body.body=std::move(code);
			return *this;
		}
		template<typename F>requires std::is_invocable_r_v<Stmt , F, LoopStmt>
		while_& do_(F&& fn){
			body.body=fn(LoopStmt{asm_(jmp(end_)),asm_(jmp(start_))});
			return *this;
		}
		void_ end() const{
			return asm_(to_code());
		}
		Code to_code() const {
			return {
				start_,
				cond,
				brz(end_),
				body,
				jmp(start_),
				end_,
			};
		}
	};
	struct do_{
		Label start_,cont,end_;
		Block body{};
		bool_ cond;
		explicit do_(Stmt code):body{std::move(code)}{}
		template<typename F>requires std::is_invocable_r_v<Stmt , F, LoopStmt>
		explicit do_(F&& fn):body{fn(LoopStmt{asm_(jmp(end_)),asm_(jmp(cont))})}{}
		void_ while_(const bool_& cond_){
			cond.value=cond_.value;
			return asm_(to_code());
		}
		Code to_code() const {
			return {
				start_,
				body,
				cont,
				cond,
				brz(end_),
				jmp(start_),
				end_,
			};
		}
	};
	struct for_{
		Label start_,cont,end_;
		void_ init,iter;
		bool_ cond;
		Block body{};
		explicit for_(void_ init,const bool_& cond,void_ iter):init(init),cond(cond),iter(iter){}
		for_& do_(Stmt code){
			body.body=std::move(code);
			return *this;
		}
		template<typename F>requires std::is_invocable_r_v<Stmt , F, LoopStmt>
		for_& do_(F&& fn){
			body.body=fn(LoopStmt{asm_(jmp(end_)),asm_(jmp(cont))});
			return *this;
		}
		void_ end() const{
			return asm_(to_code());
		}
		Code to_code() const {
			return {
				init,
				start_,
				cond,
				brz(end_),
				body,
				cont,
				iter,
				jmp(start_),
				end_,
			};
		}
	};
}
#endif //BBCPU_STATEMENT_H
