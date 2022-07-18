//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_COMPONENTS_H
#define BBCPU_COMPONENTS_H
#include "circuit.h"
#include "alu.h"
#include <numeric>
#include <map>
#include <iomanip>
#include <sstream>
#include <ostream>
#include <string>

namespace Circuit{
	template<size_t Size>
	struct Reg:Chip{
		Clock clk;
		Port<Size> input,output;
		val_t data{};
		explicit Reg(std::string name=""):Chip(std::move(name)){
			add_ports(clk,input,output);
		}
		void run() override {
			if(clk.value() == 0){
				output=data;
			}else{
				data=input.value();
			}
		}
		void reset() {
			data=0;
			output=0;
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<input(s)<<"=>"<<data<<"=>"<<output(s)<<"(clk="<<clk(s)<<")";
			};
		}
	};
	template<size_t Size>
	struct RegCE: Reg<Size>{
		using Base=Reg<Size>;
		Enable ce;
		explicit RegCE(std::string name=""): Base(std::move(name)){
			Base::add_ports(ce);
		}
		void run() override {
			Base::run();
			if(!ce.is_enable()){
				Base::data=Base::output.value();
			}
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os << Base::print(s) << "(en=" << ce(s) << ")";
			};
		}
	};
	template<size_t Size>
	struct RegCLR:Reg<Size>{
		using Base=Reg<Size>;
		Enable clr;
		explicit RegCLR(std::string name=""):Base(std::move(name)){
			Base::add_ports(clr);
		}
		void run() override {
			if(clr.is_enable()){
				Base::reset();
			} else {
				Base::run();
			}
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<Base::print(s)<<"(clr="<<clr(s)<<")";
			};
		}
	};
	template<size_t Size>
	struct Nand:Chip{
		Port<Size> A,B,Y;
		explicit Nand(std::string name=""):Chip(std::move(name)){
			add_ports(A,B,Y);
		}
		void run() override{
			Y=~(A.value()&B.value());
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<"A("<<A(s)<<") NAND ("<<B(s)<<") = "<<Y(s);
			};
		}
	};
	template<size_t Size>
	struct Adder:Chip{
		Port<Size> A,B,O;
		explicit Adder(std::string name=""):Chip(std::move(name)){
			add_ports(A,B,O);
		}
		void run() override{
			O=A.value()+B.value();
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<A(s)<<"+"<<B(s)<<"="<<O(s);
			};
		}
	};
	template<size_t Size=8>
	struct ALU:Chip{
		Port<Size> A,B,O;
		Port<6> CMS;
		Port<1> Co;
		explicit ALU(std::string name=""):Chip(std::move(name)){
			add_ports(A,B,O,CMS,Co);
		}
		void run() override {
			auto Cn=CMS.sub<1>(5).value();
			auto M=CMS.sub<1>(4).value();
			auto S=CMS.sub<4>(0).value();
			auto Ai=A.value();
			auto Bi=B.value();
			auto [carry,o]=ALU74181::run<Size>(static_cast<ALU74181::Carry>(Cn),
			                                static_cast<ALU74181::Method>(M),
			                                S,
			                                Ai,
			                                Bi);
			Co=static_cast<val_t>(carry);
			O=o;
		}

		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				std::string fn_str;
				auto cms=CMS(s);
				try{
					fn_str=ALU74181::get_fn_str(
						static_cast<ALU74181::Carry>(cms.sub<1>(5).value()),
						static_cast<ALU74181::Method>(cms.sub<1>(4).value()),
						cms.sub<4>(0).value(),
						"A",
						"B");
				}catch(const std::bad_optional_access& e){}
				os<<"CMS="<<cms<<"(O="<<fn_str<<"),A="<<A(s)<<",B="<<B(s)<<",O="<<O(s)<<",Co="<<Co(s);
			};
		}
	};
	
	template<size_t ASize=19,size_t DSize=8,typename addr_t=size_t,typename data_t=val_t>
	struct RAM:Chip{
		static constexpr size_t data_size=1<<ASize;
		Enable ce,oe,we;
		Port<ASize> A;
		Port<DSize> D;
		data_t data[data_size]{0};

		explicit RAM(std::string name=""):Chip(std::move(name)){
			add_ports(ce,oe,we,A,D);
		}
		virtual void do_write(){
			data[A.value()]=D.value();
		}
		virtual void do_read(){
			D=data[A.value()];
		}
		void run() override {
			D=Level::Floating;
			if(ce.is_enable()){
				if(we.is_enable()){
					do_write();
				} else if(oe.is_enable()) {
					do_read();
				}
			}
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<"data["<<A(s)<<"]="<<D(s)<<"(cow="<<ce(s)<<oe(s)<<we(s)<<")";
			};
		}
		void load(const std::vector<data_t>& new_data){
			std::copy_n(new_data.begin(), std::min(new_data.size(),data_size), data);
		}
		auto begin() { return &data[0]; }
		auto end()   { return ++(&data[data_size]); }
	};
	template<size_t ASize=19,size_t DSize=8,typename addr_t=size_t,typename data_t=val_t>
	struct ROM:RAM<ASize,DSize,addr_t,data_t>{
		using Base=RAM<ASize,DSize,addr_t,data_t>;
		explicit ROM(std::string name=""):Base(std::move(name)){}
		void do_write() override{
			std::cout<<"[Warning]Try write to ROM"<<std::endl;
		}
	};
	template<size_t Size=8>
	struct Bus:Chip{
		Enable oe;
		Port<1> dir;
		Port<Size> A,B;

		explicit Bus(std::string name=""):Chip(std::move(name)){
			add_ports(oe,dir,A,B);
		}
		void run() override {
			A=Level::Floating;
			B=Level::Floating;
			if(oe.is_enable()){
				if(dir.value()==1){
					B=A.value();
				}else{
					A=B.value();
				}
			}
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<"BUS"<<A(s)<<(dir(s).value()==1?"->":"<-")<<B(s)<<"(oe="<<oe(s)<<")";
			};
		}
	};
	template<size_t SelSize=2>
	struct Demux:Chip{
		static constexpr size_t output_size=1<<SelSize;
		Port<SelSize> S;
		Enable G;
		Port<output_size> Y;

		explicit Demux(std::string name=""):Chip(std::move(name)){
			add_ports(S,G,Y);
		}
		void run() override {
			Y=-1;
			if(G.is_enable()){
				Y.template sub<1>(S.value()).set(0);
			}
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<"Demux["<<S(s)<<"]"<<Y(s)<<"(G="<<G(s)<<")";
			};
		}
	};
	template<size_t Size=8>
	struct Cmp:Chip{
		Port<Size> P,Q;
		Port<1> PgtQ,PeqQ;

		explicit Cmp(std::string name=""):Chip(std::move(name)){
			add_ports(P,Q,PgtQ,PeqQ);
		}
		void run() override{
			PgtQ=!(P.value()>Q.value());
			PeqQ=!(P.value()==Q.value());
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<"Demux"<<P(s)<<"<=>"<<Q(s)<<"(P>Q:"<<PgtQ(s)<<",P==Q:"<<PeqQ<<")";
			};
		}
	};

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
				input.wire(regs[I].input...);
				(output[I].wire(regs[I].output),...);
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
		explicit CUBase(std::string name=""):Circuit(std::move(name)){
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
		explicit IOControl(std::string name=""):Circuit(std::move(name)){
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
	template<
		size_t ASize=16,size_t DSize=8,
		size_t CSize=8,size_t COff=8,size_t CVal=1,
		typename addr_t=size_t,typename data_t=val_t
	>
	struct Memory:Circuit{
		constexpr static addr_t addr_min=0;
		constexpr static addr_t addr_max=(1<<ASize)-1;
		Port<ASize> addr;
		Port<DSize> data;
		Port<1> oe,we;

		Cmp<CSize> cmp{name+"[CMP]"};
		Nand<1> nand{name+"[NAND]"};
		RAM<ASize,DSize,addr_t,data_t> ram{name+"[RAM]"};
		ROM<ASize,DSize,addr_t,data_t> rom{name+"[ROM]"};
		explicit Memory(std::string name=""):Circuit(std::move(name)){
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
		static constexpr bool is_ram(addr_t index) { return (index>>COff)>CVal;}
		static constexpr bool is_dev(addr_t index) { return (index>>COff)==CVal;}
		static constexpr bool is_rom(addr_t index) { return (index>>COff)<CVal;}
		std::optional<data_t> get_data(addr_t index) const{
			if(is_ram(index)){
				return ram.data[index];
			}else if(is_rom(index)){
				return rom.data[index];
			}
			return {};
		}
		std::string get_data_str(addr_t index) const{
			if(auto v=get_data(index);v){
				return std::to_string(*v);
			}else{
				return "DEV";
			}
		}
		static std::vector<addr_t> get_ranges(const std::multimap<addr_t,std::string>& ptrs,addr_t d=2){
			std::vector<addr_t> addrs;
			for(auto [v,name]:ptrs){

				addr_t s=(v-std::min(addr_min,v)>d)?v-d:std::min(addr_min,v);
				addr_t e=(std::max(addr_max,v)-v>d)?v+d:std::max(addr_max,v);
				
				size_t mid=addrs.size();
				addrs.resize(mid+1+e-s);
				
				std::iota(addrs.begin()+mid,addrs.end(),s);
		        std::inplace_merge(addrs.begin(), addrs.begin()+mid, addrs.end());
			}
		    addrs.erase(std::unique(addrs.begin(), addrs.end()), addrs.end());
			return addrs;
		}
		static std::string get_names(const std::multimap<addr_t,std::string>& ptrs,addr_t v,std::string dem="/"){
			auto [first,last] = ptrs.equal_range(v);
			std::string names;
			for(auto it = first; it != last; ++it ){
				if(it!=first){ names += dem; }
				names += it->second;
			}
			return names;
		}
		void print_ptrs(std::ostream& os,const std::multimap<addr_t,std::string>& ptrs,addr_t d=2) const{
			std::vector<addr_t> addrs=get_ranges(ptrs,d);
			std::stringstream addr_ss,data_ss;

			addr_ss<<std::hex;
			size_t addr_max_size=1+((ASize-1)>>2);
			for ( auto addr_it = addrs.begin(); addr_it != addrs.end(); ++addr_it ){
				std::string names=get_names(ptrs,*addr_it,"/");
				std::string data_str=get_data_str(*addr_it);

				size_t col_size=std::max({names.size(),addr_max_size,data_str.size()});
				
				     os<<std::setw(col_size)<<names;
				addr_ss<<std::setw(col_size)<<*addr_it;
				data_ss<<std::setw(col_size)<<data_str;

				if(auto it_next=std::next(addr_it);it_next!=addrs.end()){
					if(*it_next-*addr_it>1){
						     os<<" ... ";
						addr_ss<<" ... ";
						data_ss<<" ... ";
					}else{
						     os<<" ";
						addr_ss<<" ";
						data_ss<<" ";
					}
				}else{
					             os<<std::endl
					<<addr_ss.str()<<std::endl
					<<data_ss.str()<<std::endl;
				}
			}
		}
	};
	template<size_t Size>
	struct Counter:Circuit{
		Clock clk{Level::PullDown};
		Port<1> clr{Level::PullUp};
		Adder<Size> adder{name+"[Adder]"};
		RegCLR<Size> reg{name+"[Reg]"};
		explicit Counter(std::string name=""):Circuit(std::move(name)){
			add_comps(adder,reg);

			clk.wire(reg.clk);
			clr.wire(reg.clr);
			adder.O.wire(reg.input);
			adder.A.wire(reg.output);
			adder.B.set(1);
		}
		Util::Printer print() const override{
			return [&](std::ostream& os){
				os<<"adder="<<adder<<"reg="<<reg;
			};
		}
	};
}
#endif //BBCPU_COMPONENTS_H
