//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_CPU_REGFILE8X16_CPU_H
#define BBCPU_CPU_REGFILE8X16_CPU_H
#include <memory>
#include "opcode.h"
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

			bus.B.wire(D);
			bus.B.wire(Q);
			bus.dir.set(Bus<Size>::AtoB);
		}
	};
	template<size_t Size>
	struct RegCore:Circuit{
		Port<Size> A,D,F,XS,X;
		Port<1> clr,clk;
		Enable ce;

		Nand<3> nand{name+"[NAND]"};
		RegCLR<Size> reg{name+"[REG]"};
		explicit RegCore(std::string name=""):Circuit(std::move(name)){
			add_comps(nand,reg);

			clk.wire(nand.A.sub<1>(0),nand.B.sub<1>(0));
			nand.A.sub<1>(1).wire(nand.Y.sub<1>(0));
			nand.B.sub<1>(1).wire(ce);
			nand.Y.sub<1>(1).wire(nand.A.sub<1>(2),nand.B.sub<1>(2));

			reg.clk.wire(nand.Y.sub<1>(2));
			reg.clr.wire(clr);
			reg.D.wire(F);
			reg.Q.wire(D);
		}
	};
	template<size_t Size,size_t PortNum>
	struct RegUnit:Circuit{
		Port<1> clr,clk;
		Port<Size> F,S[PortNum],Q[PortNum];

		RegCore<Size> core;
		EqBus<Size> bus[PortNum];
		explicit RegUnit(val_t idx,std::string name="",std::array<std::string,PortNum> port_names):Circuit(std::move(name)){
			[&]<size_t ...I>(std::index_sequence<I...>){
				((bus[I].name=port_names[I]),...);
				add_comps(core,bus[I]...);

				core.A.wire(bus[I].A...);
				core.D.wire(bus[I].D...);
				core.F.wire(F);
				core.clr.wire(clr);
				core.clk.wire(clk);

				(S[I].wire(bus[I].S),...);
				(Q[I].wire(bus[I].Q),...);

				core.ce.wire(bus[0].ce);
				core.A.set(idx);
			}(std::make_index_sequence<PortNum>{});

		}
		explicit RegUnit(val_t idx,const std::string& name=""):RegUnit(idx,name,
				[name]<size_t ...I>(std::index_sequence<I...>){
					return std::array{(name+"[Port"+std::to_string(I)+"]")...};
				}(std::make_index_sequence<PortNum>{})
			){}
	};
	template<size_t Size,size_t RegNum,size_t PortNum>
	struct RegFile:Circuit{
		RegUnit<Size,PortNum> regs[RegNum];
		explicit RegFile(std::string name="",std::array<std::string,RegNum> reg_names):Circuit(std::move(name)){
			[&]<size_t ...I>(std::index_sequence<I...>){
				((regs[I].name=reg_names[I]),...);
				add_comps(regs[I]...);

			}(std::make_index_sequence<RegNum>{});

		}
		explicit RegFile(const std::string& name=""):RegFile(name,
		[name]<size_t ...I>(std::index_sequence<I...>){
			return std::array{(name+"[Port"+std::to_string(I)+"]")...};
		}(std::make_index_sequence<RegNum>{})
		){}
	};

	struct CU:CUBase<MARG::size,MCTRL::size,MARG::state::size,MARG::state::low,MCTRL::state::low>{

		Port<MARG::carry ::size> Ci ;
		Port<MARG::INT   ::size> INT{Level::PullDown};
		Port<MARG::opcode::size> op ;

		Port<MCTRL::INTA_    ::size> INTA_;
		Port<MCTRL::alu      ::size> CMS  ;
		Port<MCTRL::io::to   ::size> Os   ;
		Port<MCTRL::io::fromA::size> As   ;
		Port<MCTRL::io::fromB::size> Bs   ;
		Port<MCTRL::io::dir  ::size> dir  ;

		explicit CU(std::string name=""):CUBase(std::move(name)){
#define CUWIRE(PORTA,PORTB,NAME) (PORTA).wire((PORTB).sub<NAME::size>(NAME::low ))
			CUWIRE(Ci , sreg.D, MARG::carry );
			CUWIRE(INT, sreg.D, MARG::INT   );
			CUWIRE(op , sreg.D, MARG::opcode);

			CUWIRE(INTA_,tbl.D,MCTRL::INTA_    );
			CUWIRE(CMS  ,tbl.D,MCTRL::alu      );
			CUWIRE(Os   ,tbl.D,MCTRL::io::to   );
			CUWIRE(As   ,tbl.D,MCTRL::io::fromA);
			CUWIRE(Bs   ,tbl.D,MCTRL::io::fromB);
			CUWIRE(dir  ,tbl.D,MCTRL::io::dir  );
#undef CUWIRE
		}
	};
	/*
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

		Nand<1> nand{"[ClkNot]"};
		Memory<addr_size, word_size, word_size, word_size,5,addr_t,word_t> mem{"[Memory]"};
		RegCLR<MARG::carry::size> creg{"[cReg]"};
		CU cu{"[CU]"};
		Sim::ALU<word_size> alu{"[ALU]"};
		IOControl ioctl{"[IOctl]"};
		RegCESet<MCTRL::io::Rs::size, word_size> regset{"[RegSet]"};
		RAM<MCTRL::io::Bs::size, word_size> reg{"[RegFile]"};
		explicit CPU(std::string name=""):Circuit(std::move(name)){
			add_comps(nand,mem,creg,cu,alu,ioctl,regset,reg);

			clk.wire(nand.A,nand.B,creg.clk,regset.clk,reg.ce);
			clk_.wire(nand.Y,cu.clk);
			clr.wire(cu.clr,creg.clr);
			alu.Co.wire(creg.D);
			creg.Q.wire(cu.Ci);
			regset.Q[RegSet::I.v()].wire(cu.op);
			regset.Q[RegSet::A.v()].wire(alu.A);
			regset.Q[RegSet::L.v()].wire(mem.addr.sub<8>(0));
			regset.Q[RegSet::H.v()].wire(mem.addr.sub<8>(8));
			ioctl.B.wire(alu.B);
			ioctl.F.wire(alu.O,regset.D);
			ioctl.R.wire(reg.D);
			ioctl.M.wire(mem.data);
			cu.CMS.wire(alu.CMS);
			cu.bs.wire(reg.A);
			cu.rs.wire(regset.sel);
			cu.rs_en.wire(regset.en);
			cu.dir.wire(ioctl.dir);
			ioctl.mem_oe.wire(mem.oe);
			ioctl.mem_we.wire(mem.we);
			ioctl.reg_oe.wire(reg.oe);
			ioctl.reg_we.wire(reg.we);

			auto tbl=OpCode::genOpTable();
			std::copy(tbl.begin(), tbl.end(), cu.tbl.data);
			init();
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
			regset.regs[RegSet::I.v()].Q= regset.regs[RegSet::I.v()].data=op[0];
			cu.sreg.Q= cu.sreg.data=MARG::opcode::set(cu.sreg.D.value(), op[0]);
		}
		bool is_halt(){
			return regset.regs[RegSet::I.v()].Q.value() == OpCode::Ops::Halt::id::id;
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
			return reg.data[r.v()];
		}
		void set_reg(Reg r,word_t v){
			reg.data[r.v()]=v;
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

		Util::Printer print() const override{
			return [&](std::ostream& os){
				os<<"OP:"<<OpCode::Ops::all::parse(cu.op.value()).first<<std::endl;
				os<<"MCTRL:"<<MCTRL::decode(cu.tbl.D.value(),mem.addr.value())<<std::endl;
				os<<"INDEX:"<<MCTRL::state::index::get(cu.tbl.D.value())<<std::endl;
				os<<"TICK:"<<tick_count<<std::endl;

				for(size_t i=0;i<4;++i){
					os << "RS[" << RegSet(i).str() << "]=" << regset.Q[i].value() << " ";
				}
				os<<std::endl;

				for(size_t i=0;i<16;++i){
					os<<"Reg["<<Reg(i).str()<<"]="<<reg.data[i];
					if(i%8==7){
						os<<std::endl;
					}else{
						os<<" ";
					}
				}

				mem.print_ptrs(os,{
						{get_reg16(Reg16::PC),"PC"},
						{get_reg16(Reg16::SP),"SP"},
						{get_reg16(Reg16::HL),"HL"},
				},3);
				os<<std::endl;
			};
		}
	};
	*/
}
#endif //BBCPU_CPU_REGFILE8X16_CPU_H
