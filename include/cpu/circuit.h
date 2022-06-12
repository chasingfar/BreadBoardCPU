//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_CIRCUIT_H
#define BBCPU_CIRCUIT_H

#include <set>
#include <functional>
#include <utility>
#include <bitset>
#include <memory>
#include <algorithm>
#include <format>
#include <ranges>
#include "regset_sram/opcode.h"

namespace Circuit{
	enum struct Status{
		Stable = 0,
		Updated,
		WarnningWriteROM,
		Warnning,
		ErrorWireConflict,
		Error,
	};
	enum struct PortMode{
		INPUT,
		OUTPUT,
	};
	struct IPort{
		using val_t = unsigned long long;
		std::size_t size{};
		PortMode mode{};
		explicit IPort(size_t size, PortMode mode=PortMode::INPUT): size(size), mode(mode){}
		virtual bool set(std::size_t index,bool value)=0;
		virtual bool get(std::size_t index)const=0;
	};
	struct SubPort: IPort{
		IPort *port;
		size_t offset;
		SubPort(IPort *port, size_t offset, size_t size): port(port), offset(offset), IPort(size, port->mode){}
		explicit SubPort(IPort *port, size_t offset=0): port(port), offset(offset), IPort(port->size - offset, port->mode){}

		bool set(std::size_t index,bool value) override{
			return port->set(index+offset,value);
		}
		bool get(std::size_t index) const override{
			return port->get(index);
		}
	};
	template<size_t Size>
	struct Port: IPort{
		using data_t = std::bitset<Size>;
		data_t data{0};
		std::vector<SubPort*> subs{};
		explicit Port(data_t data): IPort(Size), data(data){}
		explicit Port(val_t val=0): IPort(Size), data(val){}
		explicit Port(PortMode mode): IPort(Size, mode), data(0){}
		~Port(){
			for (auto sub:subs) {
				delete sub;
			}
		}
		val_t get_val() const{
			return data.to_ullong();
		}
		bool get(std::size_t index) const override {return data.test(index);}
		bool set(std::size_t index,bool value) override {
			if(data.test(index)!=value){
				data.set(index,value);
				return true;
			}
			return false;
		}

		auto& sub(size_t offset,size_t size){
			auto sub=new SubPort(this,offset,size);
			subs.push_back(sub);
			return *sub;
		}

		Port<Size>& operator=(data_t val){
			data=val;
			return *this;
		}
		Port& operator=(val_t val){
			*this=data_t(val);
			return *this;
		}
		operator val_t(){
			return get_val();
		}
		friend std::ostream& operator<<(std::ostream& os,const Port<Size>& port){
			return os<<port.data<<"("<<port.get_val()<<")";
		}
	};

	struct IWire{
		virtual ~IWire()=default;
		virtual bool update()=0;
	};
	template<size_t Size>
	struct Wire: IWire{
		std::vector<std::pair<IPort*,size_t>> port_offsets{};
		explicit Wire(std::vector<IPort*> ports){add(ports);}

		template<typename ...Ts>
		explicit Wire(Ts... ports){(add(&ports,0),...);}
		void add(const std::vector<IPort*>& ports){
			for(auto port:ports){
				port_offsets.emplace_back(port,0);
			}
		}
		void add(const std::vector<std::pair<IPort*,size_t>>& ports){
			for(auto port:ports){
				port_offsets.push_back(port);
			}
		}
		void add(IPort* port, size_t offset){
			port_offsets.emplace_back(port,offset);
		}
		virtual bool get(size_t i) const{
			bool data,floating=true;
			for(auto [port,offset]:port_offsets){
				if(port->mode==PortMode::OUTPUT && offset<=i && i<offset+Size){
					if(floating){
						data=port->get(i-offset);
						floating=false;
					}else if(data!=port->get(i-offset)){
						throw Status::ErrorWireConflict;
					}
				}
			}
			return data;
		}
		virtual std::bitset<Size> get() const{
			std::bitset<Size> data{};
			for (size_t i = 0; i < Size; ++i) {
				data.set(i,get(i));
			}
			return data;
		}
		bool update() override{
			bool updated=false;
			auto data=get();
			for (size_t i = 0; i < Size; ++i) {
				for(auto [port,offset]:port_offsets){
					if(port->mode==PortMode::INPUT && offset<=i && i<offset+Size){
						updated=updated||port->set(i-offset,data.test(i));
					}
				}
			}
			return updated;
		}
		friend std::ostream& operator<<(std::ostream& os,const Wire<Size>& wire){
			return os<<wire.get()<<"("<<wire.get().to_ullong()<<")";
		}
	};
	template<size_t Size,typename Port<Size>::val_t val>
	struct WireConst:Wire<Size>{
		static constexpr std::bitset<Size> data{val};
		bool get(size_t i) const override{
			return data.test(i);
		}
		std::bitset<Size> get() const override{
			return data;
		}
	};
	struct Circuit{
		Status status{Status::Stable};
		virtual void update()=0;
		virtual ~Circuit()= default;
	};
	struct Adder:Circuit{
		Port<8> A,B,O{PortMode::OUTPUT};
		void update() override {
			auto v=A+B;
			O=v;
		}
		friend std::ostream& operator<<(std::ostream& os,const Adder& adder){
			return os<<adder.A<<"+"<<adder.B<<"=>"<<adder.O;
		}
	};
	template<size_t Size>
	struct RegEN:Circuit{
		Port<1> clk,en;
		Port<Size> input,output{PortMode::OUTPUT};
		std::bitset<Size> data{};
		void update() override {
			if(en.get(0) == false){
				if(clk.get(0)){
					output=data;
				}else{
					data=input.data;
				}
			}
		}
		friend std::ostream& operator<<(std::ostream& os,const RegEN& reg){
			return os<<reg.input<<"=>"<<reg.data<<"=>"<<reg.output<<"("<<reg.clk<<","<<reg.en<<")";
		}
	};
	template<size_t Size>
	struct RegCLR:Circuit{
		Port<1> clk,clr;
		Port<Size> input,output{PortMode::OUTPUT};
		std::bitset<Size> data{};
		void update() override {
			if(clr.get(0)){
				if(clk.get(0)){
					output=data;
				}else{
					data=input.data;
				}
			} else {
				data=0;
				output=0;
			}
		}
		friend std::ostream& operator<<(std::ostream& os,const RegCLR& reg){
			return os<<reg.input<<"=>"<<reg.data<<"=>"<<reg.output<<"("<<reg.clk<<","<<reg.en<<")";
		}
	};
	struct ConstPort:IPort{
		val_t val;
		explicit ConstPort(val_t val):IPort(sizeof(val_t),PortMode::OUTPUT),val(val){}
		bool set(std::size_t index,bool value) override{return false;}
		bool get(std::size_t index)const override{
			return (val>>index)&1;
		}
	};
	template<IPort::val_t val>
	inline static ConstPort const_port{val};
	template<size_t Size>
	struct Const:Circuit{

		std::bitset<Size> data{};
		Port<Size> port{PortMode::OUTPUT};
		void update() override {
			port=data;
		}
		friend std::ostream& operator<<(std::ostream& os,const Const<Size>& circuit){
			return os<<circuit.data<<"("<<circuit.data.to_ullong()<<")";
		}
	};
	/*struct Wires{
		std::vector<IWire*> wires{};
		template<size_t Size,typename ...Ts>
		Wire<Size>& wire(Ts&... ports){
			auto wire=new Wire<Size>({&ports...});
			wires.emplace_back(wire);
			return *wire;
		}
	};
	struct Circuits{
		std::vector<IWire*> wires{};
		template<size_t Size,typename ...Ts>
		Wire<Size>& wire(Ts&... ports){
			auto wire=new Wire<Size>({&ports...});
			wires.emplace_back(wire);
			return *wire;
		}
	};*/
	template<template<typename> typename Ptr=std::shared_ptr,size_t max_iter=1024>
	struct CompositeCircuit:Circuit{
		std::vector<Ptr<IWire>> wires{};
		std::vector<Ptr<Circuit>> circuits{};
		Status ingnore_level{Status::Stable};

		template<typename T>
		Ptr<T> make_circuit(){
			Ptr<T> circuit{new T()};
			circuits.emplace_back(circuit);
			return circuit;
		}
		template<typename ...Ts>
		auto make(){
			return std::tuple{make_circuit<Ts>()...};
		}

		template<size_t Size,typename ...Ts>
		auto wire(Ts&... ports){
			//auto wire=std::make_shared<Wire<Size>>(&ports...);
			Ptr<Wire<Size>> wire{new Wire<Size>(&ports...)};
			wires.emplace_back(wire);
			return wire;
		}

		/*template<size_t Size,typename T>
		auto constant(Port<Size>& port,T data){
			auto circuit=make_circuit<Const<Size>>();
			circuit->data=data;
			wire<Size>(circuit->port,port);
			return circuit;
		}*/
		bool step(){
			for(const auto& circuit:circuits){
				circuit->status=Status::Stable;
				circuit->update();
				if(ingnore_level<circuit->status){
					throw circuit;
				}
			};
			bool updated=false;
			for(auto& wire:wires){
				updated = updated || wire->update();
			}
			return updated;
		}
		void init(){

		}
		void update() override{
			for(size_t i=0;i<max_iter;++i){
				if(step()==false){return;}
			}
		}
	};
/*	template<template<typename> typename Ptr=std::shared_ptr>
	struct Simulation{


		template<size_t Size>
		Wire<Size>& make_wire(const std::vector<PortBase*>& ports){
			auto wire=new Wire<Size>(ports);
			wires.emplace_back(wire);
			return *wire;
		}
		template<size_t Size,typename ...Ts>
		auto make_wires(Ts... vals){
			return std::tuple{make_wire<Size>(vals)...};
		}
		template<typename T,typename ...Args>
		T& make_circuit(Args&& ...args){
			T* circuit=new T(args...);
			circuits.emplace_back(circuit);
			return *circuit;
		}

	};*/

	template<size_t Size=8>
	struct ALU:Circuit{
		Port<Size> A,B,O{PortMode::OUTPUT};
		Port<1> Cn;
		Port<1> M;
		Port<4> S;
		Port<1> Co{PortMode::OUTPUT};
		void update() override {
			auto [carry,o]=ALU74181::run<Size>(static_cast<ALU74181::Carry>(Cn.get_val()),
			                                static_cast<ALU74181::Method>(M.get_val()),
			                                S.get_val(),
			                                A.get_val(),
			                                B.get_val());
			Co=static_cast<IPort::val_t>(carry);
			O=o;
		}
		friend std::ostream& operator<<(std::ostream& os,const ALU& alu){
			auto fn_str=ALU74181::get_fn_str(static_cast<ALU74181::Carry>(alu.Cn.get_val()),
			                                static_cast<ALU74181::Method>(alu.M.get_val()),
			                                alu.S.get_val(),
			                                std::to_string(alu.A.get_val()),
			                                std::to_string(alu.B.get_val()));

			return os<<fn_str;
		}
	};
	template<size_t ASize=19,size_t DSize=8>
	struct RAM:Circuit{
		static constexpr size_t data_size=1<<ASize;
		Port<1> ce,oe,we;
		Port<ASize> A;
		Port<DSize> D;
		typename Port<DSize>::val_t data[data_size]{0};

		void update() override {
			D.mode=PortMode::INPUT;
			if(ce.get(0)==false){
				if(we.get(0)==false){
					data[A]=D;
				} else if(oe.get(0)==false) {
					D.mode=PortMode::OUTPUT;
					D=data[A];
				}
			}
		}
	};
	template<size_t ASize=19,size_t DSize=8>
	struct ROM:Circuit{
		using val_t = typename Port<DSize>::val_t;
		static constexpr size_t data_size=1<<ASize;
		Port<1> ce,oe,we;
		Port<ASize> A;
		Port<DSize> D{PortMode::OUTPUT};
		val_t data[data_size]{0};

		void load(const std::vector<typename Port<DSize>::val_t>& new_data){
			std::copy_n(new_data.begin(), std::min(new_data.size(),data_size), data);
		}

		void update() override {
			if(we.get(0)==false){
				status=Status::WarnningWriteROM;
			}
			if(ce.get(0)==false && oe.get(0)==false){
				D=data[A];
			}
		}
		auto begin() { return &data[0]; }
		auto end()   { return ++(&data[data_size]); }
	};
	template<size_t Size=8>
	struct Bus:Circuit{
		Port<1> oe,dir;
		Port<Size> A,B;
		void update() override {
			A.mode=PortMode::INPUT;
			B.mode=PortMode::INPUT;
			if(oe.get(0)==false){
				if(dir.get(0)){
					B=A;
					B.mode=PortMode::OUTPUT;
				}else{
					A.mode=PortMode::OUTPUT;
					A=B;
				}
			}
		}
	};
	template<size_t SelSize=2>
	struct Demux:Circuit{
		Port<SelSize> S;
		Port<1> G;
		Port<1<<SelSize> Y{PortMode::OUTPUT};

		void update() override {
			Y=-1;
			if(G.get(0)==false){
				Y.set(S, false);
			}
		}
	};
	/*struct Demux74139:Circuit{
		Port<2> AB1,AB2;
		Port<1> G1,G2;
		Port<4> Y1{PortMode::OUTPUT},Y2{PortMode::OUTPUT};

		void update() override {
			Y1=0b1111;
			Y2=0b1111;
			if(G1.get(0)==false){
				Y1.set(AB1, false);
			}
			if(G2.get(0)==false){
				Y2.set(AB2, false);
			}
		}
	};*/
	template<size_t Size=4>
	struct Nand:Circuit{
		Port<Size> A,B,Y{PortMode::OUTPUT};
		void update() override{
			Y=~(A&B);
		}
	};
	template<size_t Size=8>
	struct Cmp:Circuit{
		Port<Size> P,Q;
		Port<1> PgtQ{PortMode::OUTPUT},PeqQ{PortMode::OUTPUT};
		void update() override{
			PgtQ=!(P>Q);
			PeqQ=!(P==Q);
		}
	};

	struct CPU:CompositeCircuit<std::add_pointer_t>{
		Port<1> clk{PortMode::OUTPUT},clr{PortMode::OUTPUT};
		RAM<16,8> ram;
		ROM<16,8> rom;
		RAM<4,8> reg;
		enum {OP,AX,AL,AH};
		RegEN<8> regs[4];
		RegCLR<1> creg;
		RegCLR<19> sreg;
		ROM<19,32> cu;
		ALU<8> alu;
		Demux<2> demux_regs,demux_dir;
		Nand<4> nand;
		Cmp<8> cmp;
		Bus<8> RiBo,RoFi,MiBo,MoFi;

		Wire<1> const_enable,const_disable;
		Wire<1> wclr,wclk,wclk_,wc;
		Wire<10> state;
		Wire<8> op;
		Wire<19> marg;

		Wire<4> alu_s;
		Wire<1> alu_m,alu_c;

		Wire<8> reg_a;
		Wire<2> regs_s,dir_s;
		Wire<1> is_mem;
		Wire<1> regs_sel0,regs_sel1,regs_sel2,regs_sel3;
		Wire<8> A,addr_L,addr_H;
		Wire<8> cmp_addr;
		Wire<1> cmp_gt,cmp_eq,cmp_lt;
		Wire<1> dir_RiBo,dir_RoFi,dir_MiBo,dir_MoFi;
		Wire<1> rr,rw,mr,mw,ri,mi;
		Wire<8> F,B,R,M;


		explicit CPU():
			const_enable(const_port<0>,demux_dir.G,cu.oe),
			const_disable(const_port<1>,rom.we,cu.we),
			wclr(clr,creg.clr,sreg.clr),
			wclk(clk, creg.clk, regs[0].clk, regs[1].clk, regs[2].clk, regs[3].clk,
			     nand.A.sub(2, 1), nand.B.sub(2, 1)),
			wclk_(nand.Y.sub(2, 1), sreg.clk, reg.ce),
			wc(creg.output, sreg.input.sub(0, 1)),
			state(cu.D.sub(0, 10), sreg.input.sub(1, 10)),
			op(regs[OP].output, sreg.input.sub(11, 8)),
			marg(sreg.output, cu.A),
			alu_s(cu.D.sub(10, 4), alu.S),
			alu_m(cu.D.sub(14, 1), alu.M),
			alu_c(cu.D.sub(15, 1), alu.Cn),
			reg_a(cu.D.sub(16, 4), reg.A),
			regs_s(cu.D.sub(20, 2), demux_regs.S),
			dir_s(cu.D.sub(22, 2), demux_dir.S),
			is_mem(cu.D.sub(22, 1), demux_regs.G),
			regs_sel0(demux_regs.Y.sub(0, 1), regs[0].en),
			regs_sel1(demux_regs.Y.sub(1, 1), regs[1].en),
			regs_sel2(demux_regs.Y.sub(2, 1), regs[2].en),
			regs_sel3(demux_regs.Y.sub(3, 1), regs[3].en),
			A(regs[AX].output, alu.A),
			addr_L(regs[AL].output, ram.A.sub(0, 8), rom.A.sub(0, 8)),
			addr_H(regs[AH].output, ram.A.sub(8, 8), rom.A.sub(8, 8), cmp.Q),
			cmp_addr(cmp.P, const_port<1>),
			cmp_gt(cmp.PgtQ, nand.A.sub(3, 1), rom.ce),
			cmp_eq(cmp.PeqQ, nand.B.sub(3, 1)),
			cmp_lt(nand.Y.sub(3, 1), ram.ce),
			dir_RiBo(RiBo.dir, const_port<1>),
			dir_RoFi(RoFi.dir, const_port<0>),
			dir_MiBo(MiBo.dir, const_port<1>),
			dir_MoFi(MoFi.dir, const_port<0>),
			rr(demux_dir.Y.sub(0, 1), nand.A.sub(1, 1)),
			rw(demux_dir.Y.sub(1, 1), nand.A.sub(0, 1), RoFi.oe, reg.we),
			mr(demux_dir.Y.sub(2, 1), nand.B.sub(0, 1)),
			mw(demux_dir.Y.sub(3, 1), nand.B.sub(1, 1), MoFi.oe, ram.we),
			ri(nand.Y.sub(0, 1), RiBo.oe, reg.oe),
			mi(nand.Y.sub(1, 1), MiBo.oe, ram.oe, rom.oe),
			F(alu.O, RoFi.B, MoFi.B, regs[0].input, regs[1].input, regs[2].input, regs[3].input),
			B(alu.B, RiBo.B, MiBo.B),
			R(reg.D, RoFi.A, RiBo.A),
			M(ram.D, rom.D, MiBo.A, MoFi.A)
			{
			Circuit* c_arr[]{
				&ram, &rom, &reg, &cu, &alu,
				&creg,&sreg,
				&demux_regs, &demux_dir,
				&nand, &cmp,
				&RiBo, &RoFi, &MiBo, &MoFi,
			};
			for (auto c:c_arr) {
				circuits.push_back(c);
			}
			for (auto& c:regs) {
				circuits.push_back(&c);
			}
			IWire* w_arr[]{
				&const_enable,&const_disable,
				&wclr,&wclk,&wclk_,&wc,
				&state,
				&op,
				&marg,

				&alu_s,
				&alu_m,&alu_c,

				&reg_a,
				&regs_s,&dir_s,
				&is_mem,
				&regs_sel0,&regs_sel1,&regs_sel2,&regs_sel3,
				&A,&addr_L,&addr_H,
				&cmp_addr,
				&cmp_gt,&cmp_eq,&cmp_lt,
				&dir_RiBo,&dir_RoFi,&dir_MiBo,&dir_MoFi,
				&rr,&rw,&mr,&mw,&ri,&mi,
				&F,&B,&R,&M
			};
			for (auto w:w_arr) {
				wires.push_back(w);
			}
#if 0
				auto wclr = wire<1>(clr, creg.clr, sreg.clr);
				auto wclk = wire<1>(clk, creg.clk, regs[0].clk, regs[1].clk, regs[2].clk, regs[3].clk,
				                    nand.A.sub(2, 1), nand.B.sub(2, 1));
				auto wclk_ = wire<1>(nand.Y.sub(2, 1), sreg.clk, reg.ce);

				auto wc = wire<1>(creg.output, sreg.input.sub(0, 1));
				auto state = wire<10>(cu.D.sub(0, 10), sreg.input.sub(1, 10));
				auto op = wire<8>(regs[OP].output, sreg.input.sub(11, 8));
				auto marg = wire<19>(sreg.output, cu.A);

				auto alu_s = wire<4>(cu.D.sub(10, 4), alu.S);
				auto alu_m = wire<1>(cu.D.sub(14, 1), alu.M);
				auto alu_c = wire<1>(cu.D.sub(15, 1), alu.Cn);

				auto reg_a = wire<4>(cu.D.sub(16, 4), reg.A);

				auto regs_s = wire<2>(cu.D.sub(20, 2), demux_regs.S);
				auto dir_s = wire<2>(cu.D.sub(22, 2), demux_dir.S);
				auto is_mem = wire<1>(cu.D.sub(22, 1), demux_regs.G);
				//constant<1>(demux_dir.G, 0);

				wire<1>(demux_regs.Y.sub(0, 1), regs[0].en);
				wire<1>(demux_regs.Y.sub(1, 1), regs[1].en);
				wire<1>(demux_regs.Y.sub(2, 1), regs[2].en);
				wire<1>(demux_regs.Y.sub(3, 1), regs[3].en);

				auto alu_L = wire<8>(regs[AX].output, alu.A);
				auto addr_L = wire<8>(regs[AL].output, ram.A.sub(0, 8), rom.A.sub(0, 8));
				auto addr_H = wire<8>(regs[AH].output, ram.A.sub(8, 8), rom.A.sub(8, 8), cmp.Q);

				//constant<8>(cmp.P, 1);
				wire<1>(cmp.PgtQ, nand.A.sub(3, 1), rom.ce);
				wire<1>(cmp.PeqQ, nand.B.sub(3, 1));
				wire<1>(nand.Y.sub(3, 1), ram.ce);

				//constant<1>(RiBo.dir, 1);
				//constant<1>(RoFi.dir, 0);
				//constant<1>(MiBo.dir, 1);
				//constant<1>(MoFi.dir, 0);
				auto rr = wire<1>(demux_dir.Y.sub(0, 1), nand.A.sub(1, 1));
				auto rw = wire<1>(demux_dir.Y.sub(1, 1), nand.A.sub(0, 1), RoFi.oe, reg.we);
				auto mr = wire<1>(demux_dir.Y.sub(2, 1), nand.B.sub(0, 1));
				auto mw = wire<1>(demux_dir.Y.sub(3, 1), nand.B.sub(1, 1), MoFi.oe, ram.we);
				          wire<1>(nand.Y.sub(0, 1), RiBo.oe, reg.oe);
				          wire<1>(nand.Y.sub(1, 1), MiBo.oe, ram.oe);


				wire<8>(alu.O, RoFi.B, MoFi.B, regs[0].input, regs[1].input, regs[2].input, regs[3].input);
				wire<8>(alu.B, RiBo.B, MiBo.B);
				wire<8>(reg.D, RoFi.A, RiBo.A);
				wire<8>(ram.D, rom.D, MiBo.A, MoFi.A);
#endif
			auto tbl=BBCPU::OpCode::genOpTable();
			std::copy(tbl.begin(), tbl.end(), cu.data);
		}
		explicit CPU(const std::vector<uint8_t>& program):CPU(){
			std::copy(program.begin(),program.end(),rom.data);
		}
	};
	/*
	struct Circuit{
		virtual void update()=0;
		virtual ~Circuit()= default;
	};
	struct WireBase{
		using val_t = unsigned long long;
		std::size_t size{};
		bool updated=false;
		Circuit* source=nullptr;
		explicit WireBase(size_t size):size(size){}
		virtual ~WireBase()= default;
		virtual bool set(Circuit* from,std::size_t index,bool value)=0;
		virtual bool get(std::size_t index)const=0;
	};
	template<size_t Size>
	struct Wire:WireBase{
		using data_t = std::bitset<Size>;
		data_t data{};
		explicit Wire(data_t data):WireBase(Size),data(data){}
		explicit Wire(val_t val=0):WireBase(Size),data(val){}
		val_t get_val() const{
			return data.to_ullong();
		}
		bool get(std::size_t index) const override{return data.test(index);}
		bool set(Circuit* from,std::size_t index,bool value) override {
			if(data.test(index)!=value){
				data.set(index,value);
				return true;
			}
			return false;
		}

		Wire<Size>& operator=(data_t val){
			if(data!=val){
				if(updated){
					throw Error("Wire double write");
				}else{
					data=val;
				}
			}
			return *this;
		}
		Wire<Size>& operator=(val_t val){
			*this=data_t(val);
			return *this;
		}
		friend std::ostream& operator<<(std::ostream& os,const Wire<Size>& wire){
			return os<<wire.data<<"("<<wire.get_val()<<")";
		}
	};

	template<size_t Size>
	struct WireBind:Circuit{
		std::vector<WireBase*> A{},B{};
		void for_each_two(std::function<void(WireBase*,WireBase*,size_t,size_t)> fn){
			auto a=A.begin(),b=B.begin();
			size_t i=0,j=0;
			while(a!=A.end() && b!=B.end()){
				fn(*a,*b,i,j);
				if(++i>=(*a)->size){
					++a;
					i=0;
				}
				if(++j>=(*b)->size){
					++b;
					j=0;
				}
			}

		}
		WireBind(const std::vector<WireBase*>& A,const std::vector<WireBase*>& B):A(A),B(B){
			for_each_two([&](WireBase* a,WireBase* b,size_t i,size_t j){
				b->set(j,a->get(i));
			});
		}
		void propagate() override {
			for_each_two([&](WireBase* a,WireBase* b,size_t i,size_t j){
				if(a->updated){
					if(b->updated){
						throw Error("WireBind conflict");
					}else{
						b->set(j,a->get(i));
					}
				}else{
					if(b->updated){
						a->set(i,b->get(j));
					}
				}
			});
		}
	};
	struct Adder:Circuit{
		Wire<8> &A,&B,&O;
		Adder(Wire<8>& A,Wire<8>& B,Wire<8>& O):A(A),B(B),O(O){}
		void update() override {
			O=A.get_val()+B.get_val();
		}
	};
	struct Reg:Circuit{
		Wire<1> &clk,&en;
		Wire<8> &input,&output;
		std::bitset<8> data{};
		Reg(Wire<1>& clk,Wire<1>& en,Wire<8>& input,Wire<8>& output):clk(clk),en(en),input(input),output(output){}

		void update() override {
			if(en.get(0)== false){
				if(clk.get(0)){
					output=data;
				}else{
					data=input.data;
				}
			}
		}
	};
	struct Simulation{
		std::vector<WireBase*> wires{};
		std::vector<Circuit*> circuits{};
		~Simulation(){
			for(auto wire:wires){
				delete wire;
			}
			for(auto circuit:circuits){
				delete circuit;
			}
		}

		template<size_t Size>
		Wire<Size>& make_wire(WireBase::val_t val){
			auto wire=new Wire<Size>(val);
			wires.emplace_back(wire);
			return *wire;
		}
		template<size_t Size,typename ...Ts>
		auto make_wires(Ts... vals){
			return std::tuple{make_wire<Size>(vals)...};
		}
		template<typename T,typename ...Args>
		T& make_circuit(Args&& ...args){
			T* circuit=new T(args...);
			circuits.emplace_back(circuit);
			return *circuit;
		}

		void reset(){
			for(const auto& wire:wires){
				wire->updated=false;
			}
		}
		bool update(){
			for(const auto& circuit:circuits){
				circuit->update();
			};
			for(const auto& wire:wires){
				if(wire->updated){
					return true;
				}
			}
			return false;
		}
		void step(size_t max_iter=1024){
			for(size_t i=0;i<max_iter;++i){
				reset();
				if(update()==false){
					return;
				};
			}
		}
	};


	struct WireBase{
		std::vector<Circuit*> circuits{};
		std::size_t size{};
		Circuit* source= nullptr;
		explicit WireBase(std::size_t size):size(size){}
		virtual void set(std::size_t index,bool value)=0;
		virtual bool get(std::size_t index)=0;
		virtual bool is_changed(std::size_t index)=0;
		virtual bool is_changed()=0;
		virtual void backup()=0;
		void link(Circuit* circuit){
			circuits.emplace_back(circuit);
		}
		void propagate(Circuit* from){
			for(auto circuit:circuits){
				if(circuit!=from){
					circuit->propagate();
				}
			}
		}
		bool set_if(Circuit* from,std::size_t index,bool value){
			if(get(index)!=value){
				if(source!= nullptr && source!=from &&is_changed()){
					throw Error("wire conflict");
				} else {
					source=from;
					set(index,value);
					return true;
				}
			}
			return false;
		}
	};
	template<size_t Size>
	struct Wire:WireBase{
		using val_t = unsigned long long;
		using arr_t = std::array<bool, Size>;
		static arr_t to_arr(val_t data){
			std::array<bool,Size> arr{};
			for(auto& v:arr){
				v=((data&1ull)==1);
				data>>=1;
			}
			return arr;
		}
		static val_t to_val(arr_t arr){
			val_t res=0,i=1;
			for(auto v:arr){
				res|=v?i:0;
				i<<=1;
			}
			return res;
		}
		arr_t arr{},old{};
		explicit Wire():WireBase(Size){}
		explicit Wire(arr_t arr):WireBase(Size),arr{arr},old{arr}{}
		explicit Wire(val_t data):WireBase(Size){
			set(data);
			backup();
		}

		void set(size_t index,bool value) override{ arr[index]=value; }
		bool get(size_t index) override{ return arr[index]; }
		bool is_changed(size_t index) override{return arr[index]!=old[index];}
		bool is_changed() override{return arr!=old;}

		void set(val_t val){arr=to_arr(val);}
		val_t get() const{return to_val(arr);}

		void backup() override{old=arr;source= nullptr;}

		void set_if(Circuit* from,arr_t new_arr){
			bool flag=false;
			for(size_t i=0;i<Size;++i){
				flag|=WireBase::set_if(from,i,new_arr[i]);
			}
			if(flag){
				propagate(from);
			}
		}
		void set_if(Circuit* from,val_t new_val){set_if(from,to_arr(new_val));}
		friend std::ostream& operator<<(std::ostream& os,const Wire<Size>& wire){
			return os<<std::bitset<Size>(wire.get());
		}
		bool is_enable(){
			return !arr[0];
		}
		bool is_up(){
			return !old[0] && arr[0];
		}
		bool is_down(){
			return old[0] && !arr[0];
		}
	};
	struct Simulation{
		std::vector<WireBase*> wires{};
		explicit Simulation(std::vector<WireBase*> wires={}):wires(std::move(wires)){}

		template<size_t Size,typename Wire<Size>::val_t...vals>
		auto get_wires(){
			std::array<Wire<Size>, sizeof...(vals)> arr{vals...};
			return {};
		}
		void backup(){
			for(auto wire:wires){
				wire->backup();
			}
		}
		void clock(Wire<1>& clk){
			clk.set_if(nullptr,!clk.get(0));
			backup();
		}
	};

	template<size_t Size>
	struct WireBind:Circuit{
		std::vector<WireBase*> A{},B{};
		void for_each_two(std::function<void(WireBase*,WireBase*,size_t,size_t)> fn){
			auto a=A.begin(),b=B.begin();
			size_t i=0,j=0;
			while(a!=A.end() && b!=B.end()){
				fn(*a,*b,i,j);
				if(++i>=(*a)->size){
					++a;
					i=0;
				}
				if(++j>=(*b)->size){
					++b;
					j=0;
				}
			}

		}
		WireBind(const std::vector<WireBase*>& A,const std::vector<WireBase*>& B):A(A),B(B){
			for_each_two([&](WireBase* a,WireBase* b,size_t i,size_t j){
				b->set(j,a->get(i));
			});
		}
		void propagate() override {
			std::unordered_set<WireBase*> changed_wires{};
			for_each_two([&](WireBase* a,WireBase* b,size_t i,size_t j){
				if(a->is_changed(i)){
					if(b->is_changed(j)){
						throw Error("WireBind conflict");
					}else{
						b->set(j,a->get(i));
						changed_wires.insert(b);
					}
				}else{
					if(b->is_changed(j)){
						a->set(i,b->get(j));
						changed_wires.insert(a);
					}
				}
			});
			for(const auto& wire:changed_wires){
				wire->propagate(this);
			}
		}
	};

	struct Adder:Circuit{
		Wire<8>& A;
		Wire<8>& B;
		Wire<8>& O;
		Adder(Wire<8>& A,Wire<8>& B,Wire<8>& O):A(A),B(B),O(O){
			A.link(this);
			B.link(this);
			O.link(this);
		}
		void propagate() override {
			O.set_if(this,A.get()+B.get());
		}
	};
	struct Reg:Circuit{
		Wire<1>& clk,&en;
		Wire<8>& input,&output;
		Reg(Wire<1>& clk,Wire<1>& en,Wire<8>& input,Wire<8>& output)
				: clk(clk), en(en), input(input), output(output){
			clk.link(this);
			en.link(this);
			input.link(this);
			output.link(this);
		}

		void propagate() override {
			if(en.is_enable() && clk.is_up()){
				output.set_if(this,input.get());
			}
		}
	};
	template<size_t AddrSize>
	struct RAM:Circuit{
		Wire<1>& ce,oe,we;
		Wire<AddrSize>& A;
		Wire<8>& D;
		uint8_t data[1<<AddrSize]{0};
		RAM(Wire<1>& ce,Wire<1>& oe,Wire<1>& we,Wire<AddrSize>& A,Wire<8>& D)
				:ce(ce),oe(oe),we(we),A(A),D(data){
			ce.link(this);
			oe.link(this);
			we.link(this);
			A.link(this);
			D.link(this);
		}

		void propagate() override {
			if(ce.is_enable()){
				if(we.is_enable()){
					data[A.get()]=D.get();
				} else if(oe.is_enable()) {
					D.set_if(this,data[A.get()]);
				}
			}
		}
	};
	template<size_t AddrSize>
	struct ROM:Circuit{
		static constexpr size_t data_size=1<<AddrSize;
		Wire<1>& ce,oe,we;
		Wire<AddrSize>& A;
		Wire<8>& D;
		size_t addr;
		uint8_t data[data_size]{0};
		ROM(Wire<1>& ce,Wire<1>& oe,Wire<1>& we,Wire<AddrSize>& A,Wire<8>& D)
				:ce(ce),oe(oe),we(we),A(addr),D(data){
			ce.link(this);
			oe.link(this);
			we.link(this);
			A.link(this);
			D.link(this);
		}
		void load(const std::vector<uint8_t>& new_data){
			std::copy_n(new_data.begin(), std::min(new_data.size(),data_size), data);
		}

		void propagate() override {
			if(we.is_enable()){
				throw Error("try write to rom");
			}
			if(ce.is_enable() && oe.is_enable()){
				D.set_if(this,data[A.get()]);
			}
		}
	};
	struct Bus:Circuit{
		Wire<1>& oe,dir;
		Wire<8>& A,B;
		Bus(Wire<1>& oe,Wire<1>& dir,Wire<8>& A,Wire<8>& B)
				:oe(oe),dir(dir),A(A),B(B){
			oe.link(this);
			dir.link(this);
			A.link(this);
			B.link(this);
		}
		void propagate() override {
			if(oe.is_enable()){
				if(dir.val[0]){
					B=A.val;
				}else{
					A=B.val;
				}
			}
		}
	};
 */
}
#endif //BBCPU_CIRCUIT_H
