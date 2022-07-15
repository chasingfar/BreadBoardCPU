//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_COMPONENTS_H
#define BBCPU_COMPONENTS_H
#include "circuit.h"
#include "alu.h"
#include <ostream>
#include <string>

namespace Circuit{
	template<size_t Size>
	struct Reg:Component{
		Clock clk;
		Port<Size> input,output;
		val_t data{};
		Reg(std::string name=""):Component(std::move(name)){
			add_ports(clk,input,output);
		}
		void run() override {
			if(clk.get() == 0){
				output=data;
			}else{
				data=input.get();
			}
		}
		void reset() override{
			data=0;
			output=0;
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<input(s)<<"=>"<<data<<"=>"<<output(s)<<"(clk="<<clk(s)<<")";
			};
		}
	};
	template<size_t Size>
	struct RegEN:Reg<Size>{
		using Base=Reg<Size>;
		Enable en;
		RegEN(std::string name=""):Base(std::move(name)){
			Base::add_ports(en);
		}
		void run() override {
			if(Base::clk.get() == 0){
				Base::output=Base::data;
			}else if(en.is_enable()){
				Base::data=Base::input.get();
			}
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<Base::print(s)<<"(en="<<en(s)<<")";
			};
		}
	};
	template<size_t Size>
	struct RegCLR:Reg<Size>{
		using Base=Reg<Size>;
		Enable clr;
		RegCLR(std::string name=""):Base(std::move(name)){
			Base::add_ports(clr);
		}
		void run() override {
			if(clr.is_enable()){
				Base::reset();
			} else {
				Base::run();
			}
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<Base::print(s)<<"(clr="<<clr(s)<<")";
			};
		}
	};
	template<size_t Size>
	struct Nand:Component{
		Port<Size> A,B,Y;
		Nand(std::string name=""):Component(std::move(name)){
			add_ports(A,B,Y);
		}
		void run() override{
			Y=~(A.get()&B.get());
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<"A("<<A(s)<<") NAND ("<<B(s)<<") = "<<Y(s);
			};
		}
	};
	template<size_t Size>
	struct Adder:Component{
		Port<Size> A,B,O;
		Adder(std::string name=""):Component(std::move(name)){
			add_ports(A,B,O);
		}
		void run() override{
			O=A.get()+B.get();
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<A(s)<<"+"<<B(s)<<"="<<O(s);
			};
		}
	};
	template<size_t Size=8>
	struct ALU:Component{
		Port<Size> A,B,O;
		Port<6> CMS;
		Port<1> Co;
		ALU(std::string name=""):Component(std::move(name)){
			add_ports(A,B,O,CMS,Co);
		}
		void run() override {
			auto Cn=CMS.sub<1>(5).get();
			auto M=CMS.sub<1>(4).get();
			auto S=CMS.sub<4>(0).get();
			auto Ai=A.get();
			auto Bi=B.get();
			auto [carry,o]=ALU74181::run<Size>(static_cast<ALU74181::Carry>(Cn),
			                                static_cast<ALU74181::Method>(M),
			                                S,
			                                Ai,
			                                Bi);
			Co=static_cast<val_t>(carry);
			O=o;
		}

		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				std::string fn_str="";
				auto cms=CMS(s);
				try{
					fn_str=ALU74181::get_fn_str(
						static_cast<ALU74181::Carry>(cms.sub<1>(5).get()),
						static_cast<ALU74181::Method>(cms.sub<1>(4).get()),
						cms.sub<4>(0).get(),
						"A",
						"B");
				}catch(const ReadFloating& e){}
				os<<"CMS="<<cms<<"(O="<<fn_str<<"),A="<<A(s)<<",B="<<B(s)<<",O="<<O(s)<<",Co="<<Co(s);
			};
		}
	};
	
	template<size_t ASize=19,size_t DSize=8>
	struct RAM:Component{
		static constexpr size_t data_size=1<<ASize;
		Enable ce,oe,we;
		Port<ASize> A;
		Port<DSize> D;
		val_t data[data_size]{0};

		RAM(std::string name=""):Component(std::move(name)){
			add_ports(ce,oe,we,A,D);
		}
		void run() override {
			D=Level::Floating;
			if(ce.is_enable()){
				if(we.is_enable()){
					data[A.get()]=D.get();
				} else if(oe.is_enable()) {
					D=data[A.get()];
				}
			}else{
				D=Level::Floating;
			}
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<"RAM["<<A(s)<<"]="<<D(s)<<"(cow="<<ce(s)<<oe(s)<<we(s)<<")";
			};
		}
	};
	template<size_t ASize=19,size_t DSize=8>
	struct ROM:Component{
		static constexpr size_t data_size=1<<ASize;
		Enable ce,oe,we;
		Port<ASize> A;
		Port<DSize> D;
		val_t data[data_size]{0};

		ROM(std::string name=""):Component(std::move(name)){
			add_ports(ce,oe,we,A,D);
		}
		void load(const std::vector<val_t>& new_data){
			std::copy_n(new_data.begin(), std::min(new_data.size(),data_size), data);
		}

		void run() override {
			if(ce.is_enable()){
				if(we.is_enable()){
					std::cout<<"[Warning]Try write to ROM"<<std::endl;
				} else if(oe.is_enable()) {
					D=data[A.get()];
				}
			}else{
				D=Level::Floating;
			}
		}
		auto begin() { return &data[0]; }
		auto end()   { return ++(&data[data_size]); }
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<"ROM["<<A(s)<<"]="<<D(s)<<"(cow="<<ce(s)<<oe(s)<<we(s)<<")";
			};
		}
	};
	template<size_t Size=8>
	struct Bus:Component{
		Enable oe;
		Port<1> dir;
		Port<Size> A,B;

		Bus(std::string name=""):Component(std::move(name)){
			add_ports(oe,dir,A,B);
		}
		void run() override {
			A=Level::Floating;
			B=Level::Floating;
			if(oe.is_enable()){
				if(dir.get()==1){
					B=A.get();
				}else{
					A=B.get();
				}
			}
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<"BUS"<<A(s)<<(dir(s).get()==1?"->":"<-")<<B(s)<<"(oe="<<oe(s)<<")";
			};
		}
	};
	template<size_t SelSize=2>
	struct Demux:Component{
		static constexpr size_t output_size=1<<SelSize;
		Port<SelSize> S;
		Enable G;
		Port<output_size> Y;

		Demux(std::string name=""):Component(std::move(name)){
			add_ports(S,G,Y);
		}
		void run() override {
			Y=-1;
			if(G.is_enable()){
				Y.template sub<1>(S.get()).set(0);
			}
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<"Demux["<<S(s)<<"]"<<Y(s)<<"(G="<<G(s)<<")";
			};
		}
	};
	template<size_t Size=8>
	struct Cmp:Component{
		Port<Size> P,Q;
		Port<1> PgtQ,PeqQ;

		Cmp(std::string name=""):Component(std::move(name)){
			add_ports(P,Q,PgtQ,PeqQ);
		}
		void run() override{
			PgtQ=!(P.get()>Q.get());
			PeqQ=!(P.get()==Q.get());
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<"Demux"<<P(s)<<"<=>"<<Q(s)<<"(P>Q:"<<PgtQ(s)<<",P==Q:"<<PeqQ<<")";
			};
		}
	};

	template<size_t SelSize=2,size_t Size=8>
	struct RegENSet:Circuit{
		static constexpr size_t regs_num=1<<SelSize;
		Clock clk;
		Enable en;
		Port<SelSize> sel;
		Port<Size> input,output[regs_num];

		Demux<SelSize> demux{name+"[DeMux]"};
		RegEN<Size> regs[regs_num];
		RegENSet(std::string name=""):Circuit(std::move(name)){
			for(size_t i=0;i<regs_num;++i){
				regs[i].name=name+"[Reg"+std::to_string(i)+"]";
			}
			[&]<size_t ...I>(std::index_sequence<I...>){
				add_comps(demux,regs[I]...);

				en.wire(demux.G);
				sel.wire(demux.S);
				((demux.Y.template sub<1>(I)).wire(regs[I].en),...);
				clk.wire(regs[I].clk...);
				input.wire(regs[I].input...);
				(output[I].wire(regs[I].output),...);
			}(std::make_index_sequence<regs_num>{});
		}
	};
	template<size_t ASize=19,size_t DSize=32,size_t OPSize=8>
	struct CUBase:Circuit{
		static constexpr size_t CSize=1;
		static constexpr size_t STSize=ASize-OPSize-CSize;
		Port<OPSize> op;
		Clock clk,clk_;
		Port<1> clr,Ci;

		RegCLR<CSize> creg{name+"[cReg]"};
		RegCLR<ASize> sreg{name+"[sReg]"};
		ROM<ASize,DSize> tbl{name+"[TBL]"};
		CUBase(std::string name=""):Circuit(std::move(name)){
			add_comps(creg,sreg,tbl);

			clk.wire(creg.clk);
			clk_.wire(sreg.clk);
			clr.wire(creg.clr,sreg.clr);
			Ci.wire(creg.input);
			op.wire(sreg.input.template sub<OPSize>(CSize+STSize));
			creg.output.wire(sreg.input.template sub<CSize>(0));
			(tbl.D.template sub<STSize>(0)).wire(sreg.input.template sub<STSize>(CSize));
			sreg.output.wire(tbl.A);

			tbl.ce.set(0);
			tbl.we.set(1);
			tbl.oe.set(0);
		}
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
		IOControl(std::string name=""):Circuit(std::move(name)){
			add_comps(demux,nand,RiBo,RoFi,MiBo,MoFi);

			nand.A.sub<1>(1).wire(demux.Y.sub<1>(0));
			nand.A.sub<1>(0).wire(demux.Y.sub<1>(1),RoFi.oe,reg_we);
			nand.B.sub<1>(0).wire(demux.Y.sub<1>(2));
			nand.B.sub<1>(1).wire(demux.Y.sub<1>(3),MoFi.oe,mem_we);
			nand.Y.sub<1>(0).wire(RiBo.oe,reg_oe);
			nand.Y.sub<1>(1).wire(MiBo.oe,mem_oe);

			RiBo.dir.set(1);
			RoFi.dir.set(0);
			MiBo.dir.set(1);
			MoFi.dir.set(0);

			R.wire(RiBo.A,RoFi.A);
			M.wire(MiBo.A,MoFi.A);
			B.wire(RiBo.B,MiBo.B);
			F.wire(RoFi.B,MoFi.B);
			
			dir.wire(demux.S);
			demux.G.set(0);
		}
	};
	template<size_t ASize=16,size_t DSize=8,size_t CSize=8>
	struct Memory:Circuit{
		Port<ASize> addr;
		Port<DSize> data;
		Port<1> oe,we;

		Cmp<CSize> cmp{"[MEM][CMP]"};
		Nand<1> nand{"[MEM][NAND]"};
		RAM<ASize,DSize> ram{"[MEM][RAM]"};
		ROM<ASize,DSize> rom{"[MEM][ROM]"};
		Memory(size_t COff=8,size_t CVal=1,std::string name=""):Circuit(std::move(name)){
			add_comps(cmp,nand,ram,rom);

			ram.oe.wire(oe);
			ram.we.wire(we);
			
			rom.oe.set(0);
			rom.we.set(1);
			
			cmp.P.set(CVal);

			addr.wire(ram.A,rom.A);
			data.wire(ram.D,rom.D);
			cmp.Q.wire(addr.template sub<CSize>(COff));
			nand.A.wire(cmp.PeqQ);
			nand.B.wire(cmp.PgtQ,rom.ce);
			nand.Y.wire(ram.ce);
		}
	};
	template<size_t Size>
	struct Counter:Circuit{
		Clock clk{Level::PullDown};
		Port<1> clr{Level::PullUp};
		Adder<Size> adder{};
		RegCLR<Size> reg{};
		Counter(){
			add_comps(adder,reg);

			clk.wire(reg.clk);
			clr.wire(reg.clr);
			adder.O.wire(reg.input);
			adder.A.wire(reg.output);
			adder.B.set(1);
		}
		Util::Printer print(const std::vector<Level>& s) const override{
			return [&](std::ostream& os){
				os<<"adder="<<adder.print(adder.save())
					<<"reg="<<reg.print(reg.save());
			};
		}
	};
}
#endif //BBCPU_COMPONENTS_H
