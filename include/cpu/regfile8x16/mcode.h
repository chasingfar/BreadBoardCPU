//
// Created by chasingfar on 2020/11/23.
//

#ifndef BBCPU_CPU_REGFILE8X16_MCODE_H
#define BBCPU_CPU_REGFILE8X16_MCODE_H

#include "mctrl.h"
namespace BBCPU::RegFile8x16{
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

		void halt(){
			mctrl=MCTRL::sig::halt(MCTRL::noOp(mctrl));
		}
		void copy(Reg from,Reg to){
			LOG(from,to);
			if(step()){mctrl=MCTRL::copy(mctrl, from, to);}
		}
		void copy16(Reg16 from,Reg16 to){
			LOG(from,to);
			copy(from.L(),to.L());
			copy(from.H(),to.H());
		}

		void inc(Reg from,Reg to){
			LOG(from, to);
			if(step()){mctrl= MCTRL::inc(mctrl, from, to);}
		}
		void inc(Reg from, Reg to, Carry carry){
			LOG(from, to);
			if(step()){mctrl= MCTRL::inc(mctrl, from, to, carry);}
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
			inc(reg16.H(),MARG::getCarry(marg));
		}

		void dec(Reg from,Reg to){
			LOG(from, to);
			if(step()){mctrl= MCTRL::dec(mctrl, from, to);}
		}
		void dec(Reg from, Reg to, Carry carry){
			LOG(from, to);
			if(step()){mctrl= MCTRL::dec(mctrl, from, to, carry);}
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
			dec(reg16.H(), MARG::getCarry(marg));
		}
		void load(Reg16 from,Reg to){
			LOG(from,to);
			if(step()){mctrl=MCTRL::load(mctrl, from, to);}
		}
		void load_reset(Reg16 from,Reg to){
			LOG(from,to);
			if(step()){mctrl=MCTRL::load(mctrl, from, to);}
		}
		void save(Reg from,Reg16 to){
			LOG(from,to);
			if(step()){mctrl=MCTRL::save(mctrl, from, to);}
		}

		void add(Reg lhs,Reg rhs,Reg dest,Carry carry){
			LOG(lhs,rhs,dest,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::add<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, rhs, dest, carry);}
		}
		void add(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr MCTRL::type(*fn)(MCTRL::type)=MCTRL::alu::add<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, rhs, dest);}
		}
		void add16(Reg16 lhs,Reg16 rhs,Reg16 dest){
			LOG(lhs,rhs,dest);
			add(lhs.L(),rhs.L(),dest.L());
			add(lhs.H(),rhs.H(),dest.H(),MARG::getCarry(marg));
		}
		void sub(Reg lhs,Reg rhs,Reg dest,Carry carry){
			LOG(lhs,rhs,dest,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::sub<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, rhs, dest, carry);}
		}
		void sub(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr MCTRL::type(*fn)(MCTRL::type)=MCTRL::alu::sub<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, rhs, dest);}
		}
		void sub16(Reg16 lhs,Reg16 rhs,Reg16 dest){
			LOG(lhs,rhs,dest);
			sub(lhs.L(),rhs.L(),dest.L());
			sub(lhs.H(),rhs.H(),dest.H(),MARG::getCarry(marg));
		}
		void shift_left(Reg lhs,Reg dest,unsigned pad){
			LOG(lhs,dest,pad);
			constexpr MCTRL::type(*fn)(MCTRL::type,unsigned)=MCTRL::alu::shiftLeft<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, lhs, dest, pad);}
		}
		void shift_left(Reg lhs,Reg dest,Carry carry){
			LOG(lhs,dest,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::shiftLeft<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, lhs, lhs, dest, carry);}
		}
		void shift_right(Reg lhs,Reg dest,unsigned pad){
			LOG(lhs,dest,pad);
			shift_left(lhs,dest,pad);
			auto carry=MARG::getCarry(marg);
			for(int j=0;j<7;j++){
				shift_left(dest,dest,carry);
			}
		}
		void shift_right(Reg lhs,Reg dest,Carry CF){
			LOG(lhs,dest,CF);
			shift_left(lhs,dest,CF);
			auto carry=MARG::getCarry(marg);
			for(int j=0;j<7;j++){
				shift_left(dest,dest,carry);
			}
		}
		void logic_and(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr auto fn=MCTRL::alu::AND<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, lhs, rhs, dest);}
		}
		void logic_or(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr auto fn=MCTRL::alu::OR<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, lhs, rhs, dest);}
		}
		void logic_not(Reg lhs,Reg dest){
			LOG(lhs,dest);
			constexpr auto fn=MCTRL::alu::NOT<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, lhs, lhs, dest);}
		}
		void logic_xor(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr auto fn=MCTRL::alu::XOR<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, lhs, rhs, dest);}
		}
		void set_zero(Reg dest){
			LOG(dest);
			constexpr auto fn=MCTRL::alu::zero<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, dest, dest, dest);}
		}
		void set_zero16(Reg16 reg16){
			LOG(reg16);
			set_zero(reg16.L());
			set_zero(reg16.H());
		}
		void set_minus_one(Reg reg){
			LOG(reg);
			if(step()){mctrl=MCTRL::minusOne(mctrl, reg);}
		}
		void set_minus_one16(Reg16 reg16){
			LOG(reg16);
			set_minus_one(reg16.L());
			set_minus_one(reg16.H());
		}
		void test_zero(Reg lhs,Reg dest){
			LOG(lhs,dest);
			constexpr auto fn=MCTRL::alu::testAeqZero<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, lhs, lhs, dest);}
		}
		void test_less(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			constexpr auto fn=MCTRL::alu::testALessB<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, lhs, rhs, dest);}
		}
		void save_carry(){
			LOG(i);
			if(step()){mctrl=MCTRL::state::CF::set(MCTRL::noOp(mctrl), MARG::carry::get(marg));}
		}

		void load_op(){
			LOG(i);
			load(Reg16::PC, Reg::OPR);
			end();
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
			stack_pop(reg16.L());
			stack_pop(reg16.H());
		}
		void stack_push16(Reg16 reg16){
			LOG(reg16);
			stack_push(reg16.H());
			stack_push(reg16.L());
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
		void branch_zero(Reg16 addr,Reg lhs,Reg dest){
			LOG(addr);
			test_zero(lhs,dest);
			save_carry();
			branch(addr,MCTRL::alu::ifAeqZero);
		}
		void branch_less(Reg16 addr,Reg lhs,Reg rhs,Reg dest){
			LOG(addr);
			test_less(lhs,rhs,dest);
			save_carry();
			branch(addr,MCTRL::alu::ifALessB);
		}
		bool isINT(){
			return MARG::isINT(marg);
		}
	};
}
#endif //BBCPU_CPU_REGFILE8X16_MCODE_H
