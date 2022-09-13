//
// Created by chasingfar on 2020/11/23.
//

#ifndef BBCPU_CPU_REGSET_SRAM_MCODE_H
#define BBCPU_CPU_REGSET_SRAM_MCODE_H

#include "mctrl.h"
namespace BBCPU::RegSet_SRAM {
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
			mctrl=MCTRL::state::TCF::set(mctrl, MARG::carry::get(marg));
		}

		void reg_to_rs(Reg from,RegSet to,bool save_tcf=false){
			if(step()){mctrl=MCTRL::BtoReg(mctrl, from, to);if(save_tcf){save_TCF();}}
		}
		void rsA_to_reg(Reg to,bool save_tcf=false){
			if(step()){mctrl=MCTRL::AtoB(mctrl, to);if(save_tcf){save_TCF();}}
		}
		void mem_to_rs(RegSet to,bool save_tcf=false){
			if(step()){mctrl=MCTRL::MtoReg(mctrl, to);if(save_tcf){save_TCF();}}
		}
		void rsA_to_mem(bool save_tcf=false){
			if(step()){mctrl=MCTRL::AtoM(mctrl);if(save_tcf){save_TCF();}}
		}
		void no_op(bool save_tcf=false){
			if(step()){mctrl=MCTRL::noOp(mctrl);if(save_tcf){save_TCF();}}
		}


		void load_reg(Reg from){
			reg_to_rs(from,RegSet::A);
		}
		void save_reg(Reg to,bool save_tcf=false){
			rsA_to_reg(to,save_tcf);
		}

		void set_zero(RegSet rs){
			LOG(rs);
			if(step()){mctrl=MCTRL::zero(mctrl, rs);}
		}
		void set_zero(Reg reg){
			LOG(reg);
			if(step()){mctrl=MCTRL::zero(mctrl, reg);}
		}

		void set_minus_one(RegSet rs){
			LOG(rs);
			if(step()){mctrl=MCTRL::minusOne(mctrl, rs);}
		}
		void set_minus_one(Reg reg){
			LOG(reg);
			if(step()){mctrl=MCTRL::minusOne(mctrl, reg);}
		}

		void inc(){
			if(step()){mctrl=MCTRL::inc(mctrl);}
		}
		void inc(Carry carry){
			if(step()){mctrl=MCTRL::inc(mctrl, carry);}
		}

		void dec(){
			if(step()){mctrl=MCTRL::dec(mctrl);}
		}
		void dec(Carry carry){
			if(step()){mctrl=MCTRL::dec(mctrl, carry);}
		}

		void add(Reg rhs,Carry carry){
			LOG(rhs,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::add<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs, carry);}
		}
		void add(Reg rhs){
			LOG(rhs);
			constexpr MCTRL::type(*fn)(MCTRL::type)=MCTRL::alu::add<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs);}
		}
		void sub(Reg rhs,Carry carry){
			LOG(rhs,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::sub<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs, carry);}
		}
		void sub(Reg rhs){
			LOG(rhs);
			constexpr MCTRL::type(*fn)(MCTRL::type)=MCTRL::alu::sub<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs);}
		}
		void shift_left(Reg rhs,Carry carry){
			LOG(rhs,carry);
			constexpr MCTRL::type(*fn)(MCTRL::type,Carry)=MCTRL::alu::shiftLeft<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs, carry);}
		}
		void shift_left(Reg rhs,unsigned pad){
			LOG(rhs);
			constexpr MCTRL::type(*fn)(MCTRL::type,unsigned)=MCTRL::alu::shiftLeft<MCTRL::type>;
			if(step()){mctrl=MCTRL::calc<fn>(mctrl, rhs, pad);}
		}

		void logic_and(Reg rhs){
			LOG(rhs);
			constexpr auto fn=MCTRL::alu::AND<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, rhs);}
		}
		void logic_or(Reg rhs){
			LOG(rhs);
			constexpr auto fn=MCTRL::alu::OR<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, rhs);}
		}
		void logic_not(){
			constexpr auto fn=MCTRL::alu::NOT<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, Reg::OPR);}
		}
		void logic_xor(Reg rhs){
			LOG(rhs);
			constexpr auto fn=MCTRL::alu::XOR<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, rhs);}
		}

		void test_zero(){
			LOG(i);
			constexpr auto fn=MCTRL::alu::testAeqZero<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, Reg::OPR);}
		}
		void test_less(Reg rhs){
			LOG(rhs);
			constexpr auto fn=MCTRL::alu::testALessB<MCTRL::type>;
			if(step()){mctrl=MCTRL::logic<fn>(mctrl, rhs);}
		}
		void save_carry(){
			LOG(i);
			if(step()){mctrl=MCTRL::state::CF::set(MCTRL::noOp(mctrl), MARG::state::TCF::get(marg));}
		}


		void load_addr(Reg16 addr){
			reg_to_rs(addr.L(), RegSet::L);
			reg_to_rs(addr.H(), RegSet::H);
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
		void set_zero16(Reg16 reg16){
			LOG(reg16);
			set_zero(reg16.L());
			set_zero(reg16.H());
		}
		void set_minus_one16(Reg16 reg16){
			LOG(reg16);
			set_minus_one(reg16.L());
			set_minus_one(reg16.H());
		}
		void inc(Reg from,Reg to){
			LOG(from, to);
			load_reg(from);
			inc();
			save_reg(to,true);
		}
		void inc(Reg from, Reg to, Carry carry){
			LOG(from, to);
			load_reg(from);
			inc(carry);
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
			dec();
			save_reg(to,true);
		}
		void dec(Reg from, Reg to, Carry carry){
			LOG(from, to);
			load_reg(from);
			dec(carry);
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
			mem_to_rs(RegSet::A);
			save_reg(to);
		}
		void load_op(){
			LOG(i);
			load_addr(Reg16::PC);
			mem_to_rs(RegSet::I);
			end();
		}
		void halt(){
			load_op();
		}
		void init_op(){
			LOG(i);
			set_zero(RegSet::I);
		}
		void save(Reg16 to){
			LOG(to);
			load_addr(to);
			rsA_to_mem();
		}
		void save(Reg from,Reg16 to){
			LOG(from,to);
			load_reg(from);
			save(to);
		}

		void add(Reg lhs,Reg rhs,Reg dest,Carry carry){
			LOG(lhs,rhs,dest,carry);
			load_reg(lhs);
			add(rhs, carry);
			save_reg(dest, true);
		}
		void add(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			load_reg(lhs);
			add(rhs);
			save_reg(dest, true);
		}
		void add16(Reg16 lhs,Reg16 rhs,Reg16 dest){
			LOG(lhs,rhs,dest);
			add(lhs.L(),rhs.L(),dest.L());
			add(lhs.H(),rhs.H(),dest.H(),MARG::getTCF(marg));
		}
		void sub(Reg lhs,Reg rhs,Reg dest,Carry carry){
			LOG(lhs,rhs,dest,carry);
			load_reg(lhs);
			sub(rhs, carry);
			save_reg(dest, true);
		}
		void sub(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			load_reg(lhs);
			sub(rhs);
			save_reg(dest, true);
		}
		void sub16(Reg16 lhs,Reg16 rhs,Reg16 dest){
			LOG(lhs,rhs,dest);
			sub(lhs.L(),rhs.L(),dest.L());
			sub(lhs.H(),rhs.H(),dest.H(),MARG::getTCF(marg));
		}
		void shift_left(Reg lhs,Reg dest,unsigned pad){
			LOG(lhs,dest,pad);
			load_reg(lhs);
			shift_left(lhs, pad);
			save_reg(dest, true);
		}
		void shift_left(Reg lhs,Reg dest,Carry carry){
			LOG(lhs,dest,carry);
			load_reg(lhs);
			shift_left(lhs, carry);
			save_reg(dest, true);
		}
		void shift_right(Reg lhs,Reg dest,unsigned pad){
			LOG(lhs,dest,pad);
			load_reg(lhs);
			shift_left(lhs, pad);
			auto carry=MARG::getCarry(marg);
			for(int j=0;j<7;j++){
				shift_left(lhs, carry);
			}
			save_reg(dest, true);
		}
		void shift_right(Reg lhs,Reg dest,Carry CF){
			LOG(lhs,dest,CF);
			load_reg(lhs);
			shift_left(lhs, CF);
			auto carry=MARG::getCarry(marg);
			for(int j=0;j<7;j++){
				shift_left(lhs, carry);
			}
			save_reg(dest, true);
		}
		void logic_and(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			load_reg(lhs);
			logic_and(rhs);
			save_reg(dest);
		}
		void logic_or(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			load_reg(lhs);
			logic_or(rhs);
			save_reg(dest);
		}
		void logic_not(Reg lhs,Reg dest){
			LOG(lhs,dest);
			load_reg(lhs);
			logic_not();
			save_reg(dest);
		}
		void logic_xor(Reg lhs,Reg rhs,Reg dest){
			LOG(lhs,rhs,dest);
			load_reg(lhs);
			logic_xor(rhs);
			save_reg(dest);
		}
		void test_zero(Reg lhs){
			LOG(lhs);
			load_reg(lhs);
			test_zero();
			no_op(true);
		}
		void test_less(Reg lhs,Reg rhs){
			LOG(lhs,rhs);
			load_reg(lhs);
			test_less(rhs);
			no_op(true);
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
#endif //BBCPU_CPU_REGSET_SRAM_MCODE_H
