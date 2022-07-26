#ifndef BBCPU_SIM_CHIPS_H
#define BBCPU_SIM_CHIPS_H

#include "component.h"
#include "../alu.h"
#include <numeric>
#include <map>
#include <iomanip>
#include <sstream>
#include <ostream>
#include <string>

namespace BBCPU::Sim{
	// Basic clock-up register
	template<size_t Size>
	struct Reg:Chip{
		Clock clk;
		Port<Size> input{Mode::IN},output{0};
		val_t data{};
		explicit Reg(std::string name=""):Chip(std::move(name)){
			ports.add_ports(clk,input,output);
		}
		void run() override {
			if(clk.value() == 0){
				data=input.value();
			}else{
				output=data;
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
	// IC 74377
	template<size_t Size>
	struct RegCE: Reg<Size>{
		using Base=Reg<Size>;
		Enable ce;
		explicit RegCE(std::string name=""): Base(std::move(name)){
			Base::ports.add_ports(ce);
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
	// IC 74273
	template<size_t Size>
	struct RegCLR:Reg<Size>{
		using Base=Reg<Size>;
		Enable clr;
		explicit RegCLR(std::string name=""):Base(std::move(name)){
			Base::ports.add_ports(clr);
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
	// IC 7400
	template<size_t Size>
	struct Nand:Chip{
		Port<Size> A{Mode::IN},B{Mode::IN},Y{0};
		explicit Nand(std::string name=""):Chip(std::move(name)){
			ports.add_ports(A,B,Y);
		}
		void run() override{
			Y=~(A.value()&B.value());
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<"A("<<A(s)<<") NAND B("<<B(s)<<") = "<<Y(s);
			};
		}
	};
	template<size_t Size>
	struct Adder:Chip{
		Port<Size> A{Mode::IN},B{Mode::IN},O{0};
		explicit Adder(std::string name=""):Chip(std::move(name)){
			ports.add_ports(A,B,O);
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
	// IC 74181
	template<size_t Size=8>
	struct ALU:Chip{
		Port<Size> A{Mode::IN},B{Mode::IN},O{0};
		Port<6> CMS{Mode::IN};
		Port<1> Co{0};
		explicit ALU(std::string name=""):Chip(std::move(name)){
			ports.add_ports(A,B,CMS,O,Co);
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
	// Base on AS6C4008
	template<size_t ASize=19,size_t DSize=8,typename addr_t=size_t,typename data_t=val_t>
	struct RAM:Chip{
		static constexpr size_t data_size=1<<ASize;
		Enable ce,oe,we;
		Port<ASize> A{Mode::IN};
		Port<DSize> D;
		data_t data[data_size]{0};

		explicit RAM(std::string name=""):Chip(std::move(name)){
			ports.add_ports(ce,oe,we,A,D);
		}
		virtual void do_write(){
			if(auto v=D.get();v){
				data[A.value()]=*v;
			}else{
				if(log_warning){ std::cerr<<"[Error]"<<name<<" try write but read Floating"<<std::endl; }
			}
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
		void load(const std::vector<data_t>& new_data,addr_t start=0){
			std::copy_n(new_data.begin(), std::min(new_data.size(),data_size-start), &data[start]);
			run();
		}
		auto begin() { return &data[0]; }
		auto end()   { return ++(&data[data_size]); }
	};
	// Base on SST39SF040
	template<size_t ASize=19,size_t DSize=8,typename addr_t=size_t,typename data_t=val_t>
	struct ROM:RAM<ASize,DSize,addr_t,data_t>{
		using Base=RAM<ASize,DSize,addr_t,data_t>;
		explicit ROM(std::string name=""):Base(std::move(name)){}
		void do_write() override{
			std::cerr<<"[Warning]"<<this->name<<"Try write to ROM"<<std::endl;
		}
	};
	// Base on IC 74245
	template<size_t Size=8>
	struct Bus:Chip{
		Enable oe;
		Port<1> dir{Mode::IN};
		Port<Size> A,B;

		explicit Bus(std::string name=""):Chip(std::move(name)){
			ports.add_ports(oe,dir,A,B);
		}
		void run() override {
			A=Level::Floating;
			B=Level::Floating;
			if(oe.is_enable()){
				if(dir.value()==1){
					if(auto v=A.get();v){
						B=*v;
					}
				}else{
					if(auto v=B.get();v){
						A=*v;
					}
				}
			}
		}
		Util::Printer print(std::span<const Level> s) const override{
			return [=](std::ostream& os){
				os<<"BUS"<<A(s)<<(dir(s).value()==1?"->":"<-")<<B(s)<<"(oe="<<oe(s)<<")";
			};
		}
	};
	// Base on IC 74139
	template<size_t SelSize=2>
	struct Demux:Chip{
		static constexpr size_t output_size=1<<SelSize;
		Port<SelSize> S{Mode::IN};
		Enable G;
		Port<output_size> Y{Level::High};

		explicit Demux(std::string name=""):Chip(std::move(name)){
			ports.add_ports(S,G,Y);
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
	// Base on IC 74682
	template<size_t Size=8>
	struct Cmp:Chip{
		Port<Size> P{Mode::IN},Q{Mode::IN};
		Port<1> PgtQ{1},PeqQ{1};

		explicit Cmp(std::string name=""):Chip(std::move(name)){
			ports.add_ports(P,Q,PgtQ,PeqQ);
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
}
#endif //BBCPU_SIM_CHIPS_H
