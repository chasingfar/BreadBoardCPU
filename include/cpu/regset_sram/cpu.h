//
// Created by chasingfar on 2021/2/2.
//

#ifndef BBCPU_CPU_REGSET_SRAM_CPU_H
#define BBCPU_CPU_REGSET_SRAM_CPU_H
#include "mcode.h"
#include "sim/sim.h"
namespace BBCPU::RegSet_SRAM::Impl{
	using namespace Sim;
	template<size_t SelSize=2,size_t Size=8>
	struct RegCESet:Circuit{
		static constexpr size_t regs_num=1<<SelSize;
		Clock clk;
		Enable en;
		Port<SelSize> sel;
		Port<Size> input,output[regs_num];

		Demux<SelSize> demux{name+"[DeMux]"};
		RegCE<Size> regs[regs_num];
		RegCESet(std::string name,std::array<std::string,regs_num> reg_names):Circuit(std::move(name)){
			[&]<size_t ...I>(std::index_sequence<I...>){
				((regs[I].name=reg_names[I]),...);
				add_comps(demux,regs[I]...);

				en.wire(demux.G);
				sel.wire(demux.S);
				((demux.Y.template sub<1>(I)).wire(regs[I].ce),...);
				clk.wire(regs[I].clk...);
				input.wire(regs[I].D...);
				(output[I].wire(regs[I].Q),...);
			}(std::make_index_sequence<regs_num>{});
		}
		explicit RegCESet(const std::string& name=""):RegCESet(
			name,
			[name]<size_t ...I>(std::index_sequence<I...>){
				return std::array{
					(name+"[Reg"+std::to_string(I)+"]")...
				};
			}(std::make_index_sequence<regs_num>{})
		){}
	};
	struct IOControl:Circuit{
		Port<2> dir;
		Port<8> F,B,R,M;
		Enable mem_we,reg_we,reg_oe,mem_oe;

		Demux<2> demux{name+"[DeMux]"};
		Nand<2> nand{name+"[NAND]"};
		Bus<8> RiBo{name+"[RiBo]"},
				RoFi{name+"[RoFi]"},
				MiBo{name+"[MiBo]"},
				MoFi{name+"[MoFi]"};
		explicit IOControl(std::string name=""):Circuit(std::move(name)){
			add_comps(demux,nand,RiBo,RoFi,MiBo,MoFi);

			nand.A.sub<1>(1).wire(demux.Y.sub<1>(0));
			nand.A.sub<1>(0).wire(demux.Y.sub<1>(1),RoFi.oe,reg_we);
			nand.B.sub<1>(0).wire(demux.Y.sub<1>(2));
			nand.B.sub<1>(1).wire(demux.Y.sub<1>(3),MoFi.oe,mem_we);
			nand.Y.sub<1>(0).wire(RiBo.oe,reg_oe);
			nand.Y.sub<1>(1).wire(MiBo.oe,mem_oe);

			RiBo.dir.set(Bus<8>::AtoB);
			RoFi.dir.set(Bus<8>::BtoA);
			MiBo.dir.set(Bus<8>::AtoB);
			MoFi.dir.set(Bus<8>::BtoA);

			R.wire(RiBo.A,RoFi.A);
			M.wire(MiBo.A,MoFi.A);
			B.wire(RiBo.B,MiBo.B);
			F.wire(RoFi.B,MoFi.B);

			dir.wire(demux.S);
			demux.G.enable();
		}
	};
	struct CU:CUBase<MARG::size,MCTRL::size,MARG::state::size,MARG::state::low,MCTRL::state::low>{

		Port<MARG::carry ::size> Ci ;
		Port<MARG::INT   ::size> INT{Level::PullDown};
		Port<MARG::opcode::size> op ;

		Port<MCTRL::INTA_  ::size> INTA_;
		Port<MCTRL::alu    ::size> CMS  ;
		Port<MCTRL::io::Bs ::size> bs   ;
		Port<MCTRL::io::Rs ::size> rs   ;
		Port<MCTRL::io::dir::size> dir  ;
		Enable rs_en;

		explicit CU(std::string name=""):CUBase(std::move(name)){
#define CUWIRE(PORTA,PORTB,NAME) (PORTA).wire((PORTB).sub<NAME::size>(NAME::low ))
			CUWIRE(Ci , sreg.D, MARG::carry );
			CUWIRE(INT, sreg.D, MARG::INT   );
			CUWIRE(op , sreg.D, MARG::opcode);
			
			CUWIRE(INTA_,tbl.D,MCTRL::INTA_  );
			CUWIRE(CMS  ,tbl.D,MCTRL::alu    );
			CUWIRE(bs   ,tbl.D,MCTRL::io::Bs );
			CUWIRE(rs   ,tbl.D,MCTRL::io::Rs );
			CUWIRE(dir  ,tbl.D,MCTRL::io::dir);
#undef CUWIRE
			rs_en.wire(tbl.D.sub<1>(MCTRL::io::dir::low));
		}
	};
	struct Clocker:Circuit{
		Clock clk;
		Enable clr;
		Port<1> sclk,wclk;

		Counter<2> counter{name+"[Counter]"};
		Demux<2> demux{name+"[DeMux]"};
		explicit Clocker(std::string name=""):Circuit(std::move(name)){
			add_comps(counter,demux);

			clr.wire(counter.clr);
			clk.wire(counter.clk);
			demux.S.wire(counter.Q);
			demux.G.set(0);
			demux.Y.sub<1>(1).wire(sclk);
			demux.Y.sub<1>(3).wire(wclk);
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

		Clock clk;
		Port<1> clr;

		Clocker clocker{"[Clker]"};
		Or<1> reg_wctl{"[RegWCtl]"},mem_wctl{"[RegWCtl]"};
		Memory<addr_size, word_size, word_size, word_size,5,addr_t,word_t> mem{"[Memory]"};
		RegCLR<MARG::carry::size> creg{"[cReg]"};
		CU cu{"[CU]"};
		Sim::ALU<word_size> alu{"[ALU]"};
		IOControl ioctl{"[IOctl]"};
		RegCESet<MCTRL::io::Rs::size, word_size> regset{"[RegSet]"};
		RAM<MCTRL::io::Bs::size, word_size> reg{"[RegFile]"};
		explicit CPU(std::string name=""):Circuit(std::move(name)){
			add_comps(clocker,reg_wctl,mem_wctl,mem,creg,cu,alu,ioctl,regset,reg);

			clocker.clk.wire(clk);
			clocker.sclk.wire(cu.clk);
			clocker.wclk.wire(creg.clk,regset.clk,reg_wctl.A,mem_wctl.A);
			reg.ce.enable();
			clr.wire(cu.clr,creg.clr,clocker.clr);
			alu.Co.wire(creg.D);
			creg.Q.wire(cu.Ci);
			regset.output[RegSet::I.v()].wire(cu.op);
			regset.output[RegSet::A.v()].wire(alu.A);
			regset.output[RegSet::L.v()].wire(mem.addr.sub<8>(0));
			regset.output[RegSet::H.v()].wire(mem.addr.sub<8>(8));
			ioctl.B.wire(alu.B);
			ioctl.F.wire(alu.O,regset.input);
			ioctl.R.wire(reg.D);
			ioctl.M.wire(mem.data);
			cu.CMS.wire(alu.CMS);
			cu.bs.wire(reg.A);
			cu.rs.wire(regset.sel);
			cu.rs_en.wire(regset.en);
			cu.dir.wire(ioctl.dir);
			ioctl.mem_oe.wire(mem.oe);
			ioctl.mem_we.wire(mem_wctl.B);
			ioctl.reg_oe.wire(reg.oe);
			ioctl.reg_we.wire(reg_wctl.B);
			mem_wctl.Y.wire(mem.we);
			reg_wctl.Y.wire(reg.we);
		}
		void init(){
			clk.set(0);
			clr.set(0);
			run();
			clr.set(1);
			run();
			tick_half();
		}
		void load(const std::vector<word_t>& data, addr_t start=0){
			mem.load(data,start);
		}
		void load_op(const std::vector<word_t>& op){
			load(op, get_reg16(Reg16::PC));
			regset.regs[RegSet::I.v()].set(op[0]);
			cu.sreg.set(MARG::opcode::set(cu.sreg.D.value(), op[0]));
		}
		word_t get_op() const{
			return regset.regs[RegSet::I.v()].Q.value();
		}
		void tick_half(){
			clk.clock();
			run();
			clk.clock();
			run();
			clk.clock();
			run();
			clk.clock();
			run();
		}
		void tick(){
			++tick_count;
			tick_half();
			tick_half();
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

		virtual Util::Printer print(const std::vector<Reg16>& reg_ptrs) const {
			return [=](std::ostream& os){
				os<<"MCTRL:"<<MCTRL::decode(cu.tbl.D.value(),mem.addr.value())<<std::endl;
				os<<"INDEX:"<<MCTRL::state::index::get(cu.tbl.D.value())<<std::endl;
				os<<"TICK:"<<tick_count<<std::endl;

				for(size_t i=0;i<4;++i){
					os<<"RS["<<RegSet(i).str()<<"]="<<regset.output[i].value()<<" ";
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
/*namespace BBCPU{
	struct CPU{
		using Reg = Regs::Reg;
		using Reg16 = Regs::Reg16;
		using op_t = uint8_t;
		using addr_t = uint16_t;

		op_t REG[16]{};
		op_t REGSET[4]{};
		op_t RAM[1u<<16u]{};
		MARG::type marg=0;
		MCTRL::type mctrl=0;
		void clear(){
			for (auto& reg:REG){
				reg=0;
			}
			for (auto& ram:RAM){
				ram=0;
			}
			marg=0;
		}
		void load(auto data,addr_t addr=0){
			for (auto op:data){
				RAM[addr]=op;
				addr++;
			}
		}
		void print_reg(){
			for (int i = 0; i < 16; ++i) {
				std::cout<<std::to_string(REG[i]);
				if(i%8==8-1)std::cout<<std::endl;
				else if(i%4==4-1)std::cout<<"|";
				else if(i%2==2-1)std::cout<<",";
				else std::cout<<" ";
			}
		}
		void print_stack(){
			size_t addr_max=1u<<16u;
			size_t sp=get_addr((size_t)pair(Reg16::SP))+1;
			size_t i;
			for (i=sp; i < std::min(sp+10,addr_max); ++i) {
				std::cout<<std::to_string(RAM[i])<<" ";
			}
			if(i<addr_max){
				std::cout<<"...";
			}
			std::cout<<std::endl;
		}
		std::string get_reg_str(size_t i){
			return "REG["+std::to_string(i)+"]("+std::to_string(REG[i])+")";
		}
		std::string get_regset_str(size_t i){
			return "REGSET["+std::to_string(i)+"]("+std::to_string(REGSET[i])+")";
		}
		std::string get_ram_str(size_t i){
			return "RAM["+std::to_string(i)+"]("+std::to_string(RAM[i])+")";
		}
		addr_t get_addr(auto v){
			return (static_cast<addr_t>(REG[v+1])<<8u)|REG[v];
		}
		addr_t get_pair(Reg16 reg16){
			return get_addr(pair(reg16).v());
		}
		op_t read_pair(Reg16 reg16, int16_t offset= 0){
			return RAM[get_pair(reg16)+offset];
		}
		void load_op(auto op){
			load(op, get_pair(Reg16::PC));
			REGSET[RegSet::I.v()]=op[0];
			marg=MARG::opcode::set(marg,op[0]);
		}
		bool isHalt(){
			return MARG::opcode::get(marg)==OpCode::Ops::Halt::id::id;
		}
		void tick(bool debug=false){
			if(debug){std::cout<<std::bitset<19>(marg)<<std::endl;}
			mctrl=OpCode::Ops::all::gen(marg);
			if(debug){std::cout<<std::endl;}
			auto dir=static_cast<DirMode>(MCTRL::io::dir::get(mctrl));
			op_t A=0,B=0,F=0;
			std::string A_str="",B_str="",F_str="",fn_str="";
			std::tie(A,A_str)=std::pair{REGSET[RegSet::A.v()],get_regset_str(RegSet::A.v())};
			auto addr=(static_cast<addr_t>(REGSET[RegSet::H.v()])<<8)|REGSET[RegSet::L.v()];
			auto Bs=MCTRL::io::Bs::get(mctrl);
			auto Rs=MCTRL::io::Rs::get(mctrl);

			if (dir==DirMode::Br){
				std::tie(B,B_str)=std::pair{REG[Bs],get_reg_str(Bs)};
			}else if(dir==DirMode::Mr){
				std::tie(B,B_str)=std::pair{RAM[addr],get_ram_str(addr)};
			}

			auto [carry,O]=MCTRL::alu::run<8>(mctrl,A,B);
			fn_str=MCTRL::alu::get_fn_str(mctrl,A_str,B_str)+"="+std::to_string(O);
			switch (dir) {
				case DirMode::Br:
				case DirMode::Mr:
					F_str=get_regset_str(Rs);
					REGSET[Rs]=O;
					break;
				case DirMode::Bw:
					F_str=get_reg_str(Bs);
					REG[Bs]=O;
					break;
				case DirMode::Mw:
					F_str=get_ram_str(addr);
					RAM[addr]=O;
					break;
			}

			if(debug){
				std::cout<<F_str<<"="<<fn_str<<" carry="<<std::boolalpha<<(carry==MCTRL::alu::Carry::yes)<<std::endl;
			}

			marg=MARG::carry::set(0,static_cast<MARG::type>(carry));
			marg=MARG::state::set(marg,MCTRL::state::get(mctrl));
			marg=MARG::opcode::set(marg,REGSET[RegSet::I.v()]);
		}
		void tick_op(bool debug=false){
			do{
				tick(debug);
			}while(MARG::getIndex(marg)!=0);
			if(debug){
				std::cout<<std::endl;
			}
		}
	};
}*/
#endif //BBCPU_CPU_REGSET_SRAM_CPU_H
