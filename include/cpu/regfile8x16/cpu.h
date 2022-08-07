//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_CPU_REGFILE8X16_CPU_H
#define BBCPU_CPU_REGFILE8X16_CPU_H
#include <memory>
#include "mcode.h"
#include "sim/sim.h"
namespace BBCPU::RegFile8x16::Impl{
	using namespace Sim;
	template<size_t Size>
	struct EqBus:Circuit{
		Port<Size> A,D,S,Q;
		Port<1> oe;
		Eq<Size> eq{name+"[EQ]"};
		Bus<Size> bus{name+"[BUS]"};
		explicit EqBus(std::string name=""):Circuit(std::move(name)){
			add_comps(eq,bus);

			eq.oe.enable();
			eq.P.wire(S);
			eq.Q.wire(A);
			eq.PeqQ.wire(bus.oe,oe);

			bus.A.wire(D);
			bus.B.wire(Q);
			bus.dir.set(Bus<Size>::AtoB);
		}
	};
	template<size_t Size>
	struct RegCore:Circuit{
		Port<Size> A,D,F,XS,X;
		Port<1> clr,clk;
		Enable ce;

		And<1> and_{name+"[AND]"};
		Not<1> not_{name+"[NOT]"};
		RegCLR<Size> reg{name+"[REG]"};
		explicit RegCore(std::string name=""):Circuit(std::move(name)){
			add_comps(and_,not_,reg);

			clk.wire(and_.A);
			not_.Y.wire(and_.B);
			ce.wire(not_.A);

			reg.clk.wire(and_.Y);
			reg.clr.wire(clr);
			reg.D.wire(F);
			reg.Q.wire(D);
		}
	};
	template<size_t Size,size_t PortNum>
	struct RegUnit:Circuit{
		Port<1> clr,clk;
		Port<Size> F,S[PortNum],Q[PortNum];

		RegCore<Size> core{name+"[Core]"};
		EqBus<Size> bus[PortNum];
		template <size_t ...I>
		explicit RegUnit(val_t idx,std::string name,std::index_sequence<I...>)
		:Circuit(name),
		bus{EqBus<Size>(name+"[Port"+std::to_string(I)+"]")...}
		{
			add_comps(core,bus[I]...);

			core.A.wire(bus[I].A...);
			core.D.wire(bus[I].D...);
			core.F.wire(F);
			core.clr.wire(clr);
			core.clk.wire(clk);

			(S[I].wire(bus[I].S),...);
			(Q[I].wire(bus[I].Q),...);

			core.ce.wire(bus[0].oe);
			core.A.set(idx);
		}
		explicit RegUnit(val_t idx,const std::string& name=""):RegUnit(idx,name,std::make_index_sequence<PortNum>{}){}
	};
	template<size_t Size,size_t RegNum,size_t PortNum>
	struct RegFile:Circuit{
		Port<1> clr,clk;
		Port<Size> F,S[PortNum],Q[PortNum];

		RegUnit<Size,PortNum> regs[RegNum];
		template <size_t ...I>
		explicit RegFile(std::string name,std::index_sequence<I...>)
		:Circuit(name),
		regs{RegUnit<Size,PortNum>(I,name+"[Reg "+Reg(I).str()+"]")...}
		{
			add_comps(regs[I]...);

			clr.wire(regs[I].clr...);
			clk.wire(regs[I].clk...);
			F.wire(regs[I].F...);
			for(size_t j=0;j<PortNum;++j){
				S[j].wire(regs[I].S[j]...);
				Q[j].wire(regs[I].Q[j]...);
			}
		}
		explicit RegFile(const std::string& name=""):RegFile(name,std::make_index_sequence<RegNum>{}){}
	};

	template<size_t Size,size_t SelSize>
	struct RegCTL:Circuit{
		constexpr static size_t reg_num=(1<<SelSize);
		Port<Size> F,L,R;
		Port<Size*2> A{Level::PullDown};
		Port<1> clr,clk;
		Port<SelSize> FS,LS,RS;
		Enable Mr,Mw;

		Not<1> notMr{name+"[notMr]"};
		And<1> rClk{name+"[rClk]"},isM{name+"[isM]"};
		Bus<Size> YtoL{name+"[YtoL]"},XtoAL{name+"[XtoAL]"},YtoAL{name+"[YtoAL]"},ZtoAH{name+"[ZtoAH]"};
		RegFile<Size,reg_num,3> regfile{name+"[REGFILE]"};
		explicit RegCTL(std::string name=""):Circuit(std::move(name)){
			add_comps(notMr,rClk,isM,YtoL,XtoAL,YtoAL,ZtoAH,regfile);
			F.wire(regfile.F);
			clr.wire(regfile.clr);
			clk.wire(rClk.A);
			Mw.wire(rClk.B);
			rClk.Y.wire(regfile.clk);

			FS.wire(regfile.S[0].template sub<SelSize>(0));
			LS.wire(regfile.S[1].template sub<SelSize>(0));
			RS.wire(regfile.S[2].template sub<SelSize>(0));

			regfile.S[0].template sub<Size-SelSize>(SelSize).set(0);
			regfile.S[1].template sub<Size-SelSize>(SelSize).set(0);
			regfile.S[2].template sub<Size-SelSize>(SelSize).set(0);

			 YtoL.dir.set(Bus<Size>::AtoB);
			XtoAL.dir.set(Bus<Size>::AtoB);
			YtoAL.dir.set(Bus<Size>::AtoB);
			ZtoAH.dir.set(Bus<Size>::AtoB);
			 YtoL.A.wire(regfile.Q[1]);
			XtoAL.A.wire(regfile.Q[0]);
			YtoAL.A.wire(regfile.Q[1]);
			ZtoAH.A.wire(regfile.Q[2],R);
			 YtoL.B.wire(L);
			XtoAL.B.wire(A.template sub<Size>(0));
			YtoAL.B.wire(A.template sub<Size>(0));
			ZtoAH.B.wire(A.template sub<Size>(Size));
			 YtoL.oe.wire(notMr.Y);
			XtoAL.oe.wire(Mw);
			YtoAL.oe.wire(Mr);
			ZtoAH.oe.wire(isM.Y);
			Mw.wire(isM.A);
			Mr.wire(isM.B,notMr.A);
		}
		auto& operator[](size_t i){
			return regfile.regs[i].core.reg;
		}
		auto& operator[](size_t i) const{
			return regfile.regs[i].core.reg;
		}
		auto& operator[](Reg r){
			return regfile.regs[r.v()].core.reg;
		}
		auto& operator[](Reg r) const{
			return regfile.regs[r.v()].core.reg;
		}
	};

	struct CU:CUBase<MARG::size,MCTRL::size,MARG::state::size,MARG::state::low,MCTRL::state::low>{

		Port<MARG::carry ::size> Ci ;
		Port<MARG::INT   ::size> INT{Level::PullDown};
		Port<MARG::opcode::size> op ;

		Port<MCTRL::INTA_              ::size> INTA_;
		Port<MCTRL::alu                ::size> CMS  ;
		Port<MCTRL::io::to             ::size> Os   ;
		Port<MCTRL::io::fromA          ::size> As   ;
		Port<MCTRL::io::fromB          ::size> Bs   ;
		Port<MCTRL::io::dir::mem_write_::size> Mw   ;
		Port<MCTRL::io::dir::mem_read_ ::size> Mr   ;

		explicit CU(std::string name=""):CUBase(std::move(name)){
#define CUWIRE(PORTA,PORTB,NAME) (PORTA).wire((PORTB).template sub<NAME::size>(NAME::low ))
			CUWIRE(Ci , sreg.D, MARG::carry );
			CUWIRE(INT, sreg.D, MARG::INT   );
			CUWIRE(op , sreg.D, MARG::opcode);

			CUWIRE(INTA_,tbl.D, MCTRL::INTA_              );
			CUWIRE(CMS  ,tbl.D, MCTRL::alu                );
			CUWIRE(Os   ,tbl.D, MCTRL::io::to             );
			CUWIRE(As   ,tbl.D, MCTRL::io::fromA          );
			CUWIRE(Bs   ,tbl.D, MCTRL::io::fromB          );
			CUWIRE(Mw   ,tbl.D, MCTRL::io::dir::mem_write_);
			CUWIRE(Mr   ,tbl.D, MCTRL::io::dir::mem_read_ );
#undef CUWIRE
		}
	};
	
	struct CPU:Circuit{
		using Reg = Regs::Reg;
		using Reg16 = Regs::Reg16;
		using addr_t = uint16_t;
		using word_t = uint8_t;
		static constexpr size_t addr_size=sizeof(addr_t)*8;
		static constexpr size_t word_size=sizeof(word_t)*8;
		size_t tick_count=0;

		Clock clk,clk_;
		Port<1> clr;

		Not<1> clkNot{"[ClkNot]"};
		Bus<word_size> FiMo{name+"[FiMo]"},MiBo{name+"[MiBo]"};
		Memory<addr_size, word_size, word_size, word_size,5,addr_t,word_t> mem{"[Memory]"};
		RegCLR<MARG::carry::size> creg{"[cReg]"};
		CU cu{"[CU]"};
		Sim::ALU<word_size> alu{"[ALU]"};
		RegCTL<word_size,MCTRL::io::to::size> reg{"[RegCTL]"};
		explicit CPU(std::string name=""):Circuit(std::move(name)){
			add_comps(clkNot,FiMo,MiBo,mem,creg,cu,alu,reg);

			clk.wire(clkNot.A,creg.clk,reg.clk);
			clk_.wire(clkNot.Y,cu.clk);
			clr.wire(cu.clr,creg.clr,reg.clr);
			alu.Co.wire(creg.D);
			creg.Q.wire(cu.Ci);

			cu.op.wire(reg[Reg::OPR].Q);
			cu.CMS.wire(alu.CMS);
			cu.Os.wire(reg.FS);
			cu.As.wire(reg.LS);
			cu.Bs.wire(reg.RS);
			cu.Mw.wire(reg.Mw,mem.we);
			cu.Mr.wire(reg.Mr,mem.oe);

			reg.F.wire(alu.O);
			reg.L.wire(alu.A);
			reg.R.wire(alu.B);
			reg.A.wire(mem.addr);

			FiMo.dir.set(Bus<word_size>::AtoB);
			MiBo.dir.set(Bus<word_size>::AtoB);
			FiMo.A.wire(alu.O);
			MiBo.A.wire(mem.data);
			FiMo.B.wire(mem.data);
			MiBo.B.wire(alu.A);
			FiMo.oe.wire(cu.Mw);
			MiBo.oe.wire(cu.Mr);
		}
		void init(){
			clk.set(1);
			clr.set(0);
			run();
			clr.set(1);
			run();
		}
		void load(const std::vector<word_t>& data, addr_t start=0){
			mem.load(data,start);
		}
		void load_op(const std::vector<word_t>& op){
			load(op, get_reg16(Reg16::PC));
			set_reg(Reg::OPR,op[0]);
			cu.sreg.set(MARG::opcode::set(cu.sreg.D.value(), op[0]));
		}
		word_t get_op() const{
			return get_reg(Reg::OPR);
		}
		void tick(){
			++tick_count;
			clk.clock();
			run();
			clk.clock();
			run();
		}
		void tick_op(){
			do{
				tick();
			}while(MCTRL::state::index::get(cu.tbl.D.value())!=0);
		}

		word_t get_reg(Reg r) const{
			return reg[r].Q.value();
		}
		void set_reg(Reg r,word_t v){
			reg[r].set(v);
		}
		addr_t get_reg16(Reg16 reg16) const{
			return (static_cast<addr_t>(get_reg(reg16.H()))<<8u)|get_reg(reg16.L());
		}
		void set_reg16(Reg16 reg16,addr_t v){
			set_reg(reg16.H(),(v>>8u)&0xff);
			set_reg(reg16.L(),      v&0xff);
		}

		word_t read_ptr(Reg16 reg16, int16_t offset=0) const{
			return mem.get_data(get_reg16(reg16)+offset).value_or(0);
		}
		word_t get_stack_top() const{
			return read_ptr(Reg16::SP,1);
		}
		word_t get_stack_insert() const{
			return read_ptr(Reg16::SP);
		}
		word_t get_mem_data(addr_t addr) const{
			return mem.get_data(addr).value();
		}

		using Flags=MCTRL::state;
		template<typename FLAG>
		void set_flag(auto v){
			cu.tbl.D=FLAG::set(cu.tbl.D.value(),v);
		}

		virtual Util::Printer print(const std::vector<Reg16>& reg_ptrs) const {
			return [=](std::ostream& os){
				os<<"MCTRL:"<<MCTRL::decode(cu.tbl.D.value(),mem.addr.value())<<std::endl;
				os<<"INDEX:"<<MCTRL::state::index::get(cu.tbl.D.value())<<std::endl;
				os<<"TICK:"<<tick_count<<std::endl;

				for(size_t i=0;i<16;++i){
					os<<"Reg["<<Reg(i).str()<<"]="<<(int)get_reg(Reg(i));
					if(i%8==7){
						os<<std::endl;
					}else{
						os<<" ";
					}
				}
				std::multimap<addr_t,std::string> ptrs{};
				for(auto r:reg_ptrs){
					ptrs.emplace(get_reg16(r),r.str());
				}
				mem.print_ptrs(os,ptrs,3);
				os<<std::endl;
			};
		}
		Util::Printer print() const override{
			return print({Reg16::PC,Reg16::SP});
		}
	};
}
#endif //BBCPU_CPU_REGFILE8X16_CPU_H
