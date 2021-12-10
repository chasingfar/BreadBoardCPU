//
// Created by chasingfar on 2020/11/23.
//

#ifndef BBCPU_CPU_MCODE_H
#define BBCPU_CPU_MCODE_H

#include "mctrl.h"
namespace BBCPU{
	struct MCode{
		MARG::type marg;
		MCTRL::type mctrl;
		size_t i;
		using Carry=MCTRL::alu::Carry;
		MCode(MARG::type marg,MCTRL::type mctrl,size_t i):marg(marg),mctrl(mctrl),i(i){}
		bool step(){
			if(MARG::state::index::get(marg)==i){
				i+=1;
				mctrl=MCTRL::state::set(mctrl,MARG::state::get(marg));
				mctrl=MCTRL::setIndex(mctrl,i);
				LOG_STEP()
				return true;
			}else{
				i+=1;
				LOG_STOP()
				return false;
			}
		}
		void end(){
			if(MARG::state::index::get(marg)==i-1){
				mctrl=MCTRL::setIndex(mctrl,0);
			}
		}

		void save_TCF(){
			MCTRL::state::TCF::set(mctrl, MARG::carry::get(marg));
		}
		void load_reg(Reg from){
			if(step()){mctrl=MCTRL::BtoReg(mctrl, from, RegSet::A);}
		}
		void save_reg(Reg to,bool save_tcf=false){
			if(step()){mctrl=MCTRL::AtoB(mctrl, to);if(save_tcf){save_TCF();}}
		}
		void load_addr(Reg16 addr){
			if(step()){mctrl=MCTRL::BtoReg(mctrl, addr.L(), RegSet::L);}
			if(step()){mctrl=MCTRL::BtoReg(mctrl, addr.H(), RegSet::H);}
		}
		void halt(){
			//if(step()){mctrl=MCTRL::sig::halt(MCTRL::noOp(mctrl));}
			end();
		}
		void copy(Reg from,Reg to){
			LOG(from,to);
			load_reg(from);
			save_reg(to);
		}
		void copy16(Reg16 from,Reg16 to){
			LOG(from,to);
			copy(from.L(),to.L());
			copy(from.H(),to.H());
		}

		void inc(Reg from,Reg to){
			LOG(from, to);
			load_reg(from);
			if(step()){mctrl=MCTRL::inc(mctrl);}
			save_reg(to,true);
		}
		void inc(Reg from, Reg to, Carry carry){
			LOG(from, to);
			load_reg(from);
			if(step()){mctrl=MCTRL::inc(mctrl, carry);}
			save_reg(to,true);
		}
		void inc(Reg reg){
			inc(reg,reg);
		}
		void inc(Reg reg, Carry carry){
			inc(reg, reg, carry);
		}
		void inc16(Reg16 reg16){
			LOG(reg16);
			inc(reg16.L());
			inc(reg16.H(),MARG::getTCF(marg));
		}

		void dec(Reg from,Reg to){
			LOG(from, to);
			load_reg(from);
			if(step()){mctrl= MCTRL::dec(mctrl);}
			save_reg(to,true);
		}
		void dec(Reg from, Reg to, Carry carry){
			LOG(from, to);
			load_reg(from);
			if(step()){mctrl= MCTRL::dec(mctrl, carry);}
			save_reg(to,true);
		}
		void dec(Reg reg){
			dec(reg,reg);
		}
		void dec(Reg reg, Carry carry){
			dec(reg, reg, carry);
		}
		void dec16(Reg16 reg16){
			LOG(reg16);
			dec(reg16.L());
			dec(reg16.H(), MARG::getTCF(marg));
		}
		void load(Reg16 from,Reg to){
			LOG(from,to);
			load_addr(from);
			if(step()){mctrl=MCTRL::MtoReg(mctrl, RegSet::A);}
			save_reg(to);
		}
		void load_op(){
			LOG(i);
			load_addr(Reg16::PC);
			if(step()){mctrl=MCTRL::MtoReg(mctrl, RegSet::I);}
			end();
		}
		void save(Reg from,Reg16 to){
			LOG(from,to);
			load_reg(from);
			load_addr(to);
			if(step()){mctrl=MCTRL::AtoM(mctrl);}
		}

		void add(Reg lhs,Reg rhs,Reg dest,Carry carry){
			LOG(lhs,rhs,dest,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::add<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs, carry);}
			save_reg(dest, true);
		}
		void add(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr MCTRL::type(*fn)(MCTRL::type)=MCTRL::alu::add<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs);}
			save_reg(dest, true);
		}
		void add16(Reg16 lhs,Reg16 rhs,Reg16 dest){
			LOG(lhs,rhs,dest);
			add(lhs.L(),rhs.L(),dest.L());
			add(lhs.H(),rhs.H(),dest.H(),MARG::getTCF(marg));
		}
		void sub(Reg lhs,Reg rhs,Reg dest,Carry carry){
			LOG(lhs,rhs,dest,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::sub<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs, carry);}
			save_reg(dest, true);
		}
		void sub(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr MCTRL::type(*fn)(MCTRL::type)=MCTRL::alu::sub<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs);}
			save_reg(dest, true);
		}
		void sub16(Reg16 lhs,Reg16 rhs,Reg16 dest){
			LOG(lhs,rhs,dest);
			sub(lhs.L(),rhs.L(),dest.L());
			sub(lhs.H(),rhs.H(),dest.H(),MARG::getTCF(marg));
		}
		void shift_left(Reg lhs,Reg dest,unsigned pad){
			LOG(lhs,dest,pad);
			constexpr MCTRL::type(*fn)(MCTRL::type,unsigned)=MCTRL::alu::shiftLeft<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, pad);}
			save_reg(dest, true);
		}
		void shift_left(Reg lhs,Reg dest,Carry carry){
			LOG(lhs,dest,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::shiftLeft<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, carry);}
			save_reg(dest, true);
		}
		void shift_right(Reg lhs,Reg dest,unsigned pad){
			LOG(lhs,dest,pad);
			constexpr MCTRL::type(*fn)(MCTRL::type,unsigned)=MCTRL::alu::shiftLeft<MCTRL::type>;
			constexpr MCTRL::type(*fnc)(MCTRL::type,Carry)=MCTRL::alu::shiftLeft<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, pad);}
			auto carry=MARG::getCarry(marg);
			for(int j=0;j<7;j++){
				if(step()){mctrl=MCTRL::calc<fnc>(mctrl, lhs, carry);}
			}
			save_reg(dest, true);
		}
		void shift_right(Reg lhs,Reg dest,Carry CF){
			LOG(lhs,dest,CF);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::shiftLeft<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, CF);}
			auto carry=MARG::getCarry(marg);
			for(int j=0;j<7;j++){
				if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, carry);}
			}
			save_reg(dest, true);
		}
		void logic_and(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr auto fn=MCTRL::alu::AND<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, rhs);}
			save_reg(dest);
		}
		void logic_or(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr auto fn=MCTRL::alu::OR<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, rhs);}
			save_reg(dest);
		}
		void logic_not(Reg lhs,Reg dest){
			LOG(lhs,dest);
			constexpr auto fn=MCTRL::alu::NOT<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, lhs);}
			save_reg(dest);
		}
		void logic_xor(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr auto fn=MCTRL::alu::XOR<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, rhs);}
			save_reg(dest);
		}
		void set_zero(Reg dest){
			LOG(dest);
			constexpr auto fn=MCTRL::alu::zero<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, dest);}
			save_reg(dest);
		}
		void test_zero(Reg lhs){
			LOG(lhs,dest);
			constexpr auto fn=MCTRL::alu::testAeqZero<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, lhs);}
			if(step()){mctrl=MCTRL::noOp(mctrl);save_TCF();}

		}
		void test_less(Reg lhs,Reg rhs){
			LOG(lhs,rhs,dest);
			constexpr auto fn=MCTRL::alu::testALessB<MCTRL::type>;
			load_reg(lhs);
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, rhs);}
			if(step()){mctrl=MCTRL::noOp(mctrl);save_TCF();}
		}
		void save_carry(){
			LOG(i);
			if(step()){mctrl=MCTRL::state::CF::set(MCTRL::noOp(mctrl), MARG::state::TCF::get(marg));}
		}

		void next_op(){
			LOG(i);
			inc16(Reg16::PC);
			load_op();
		}
		void stack_pop(Reg reg){
			LOG(reg);
			inc16(Reg16::SP);
			load(Reg16::SP,reg);
		}
		void stack_push(Reg reg){
			LOG(reg);
			save(reg,Reg16::SP);
			dec16(Reg16::SP);
		}
		void load_imm(Reg reg){
			LOG(reg);
			inc16(Reg16::PC);
			load(Reg16::PC,reg);
		}
		void stack_pop16(Reg16 reg16){
			LOG(reg16);
			stack_pop(reg16.H());
			stack_pop(reg16.L());
		}
		void stack_push16(Reg16 reg16){
			LOG(reg16);
			stack_push(reg16.L());
			stack_push(reg16.H());
		}
		void load_imm16(Reg16 reg16){
			LOG(reg16);
			load_imm(reg16.H());
			load_imm(reg16.L());
		}
		void jump(Reg16 addr){
			LOG(addr);
			copy16(addr,Reg16::PC);
			load_op();
		}
		void branch(Reg16 addr,Carry cond=Carry::yes){
			LOG(addr);
			if(MARG::getCF(marg)==cond){
				jump(addr);
			}
		}
		void branch_zero(Reg16 addr,Reg lhs){
			LOG(addr);
			test_zero(lhs);
			save_carry();
			branch(addr,MCTRL::alu::ifAeqZero);
		}
		void branch_less(Reg16 addr,Reg lhs,Reg rhs){
			LOG(addr);
			test_less(lhs,rhs);
			save_carry();
			branch(addr,MCTRL::alu::ifALessB);
		}
		bool isINT(){
			return MARG::isINT(marg);
		}
	};
}
#endif //BBCPU_CPU_MCODE_H
