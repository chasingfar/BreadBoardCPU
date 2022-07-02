//
// Created by chasingfar on 2022/4/26.
//

#ifndef BBCPU_COMPONENTS_H
#define BBCPU_COMPONENTS_H
#include "circuit.h"
#include "alu.h"

namespace Circuit{
	template<size_t Size>
	struct Reg:Component{
		Port<1> clk;
		Port<Size> input,output;
		val_t data{};
		void update() override {
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
		std::ostream& print(std::ostream& os) const override{
			return os<<input<<"=>"<<data<<"=>"<<output<<"(clk="<<clk<<")";
		}
	};
	template<size_t Size>
	struct RegEN:Reg<Size>{
		using Base=Reg<Size>;
		Enable en;
		void update() override {
			if(en.is_enable()){
				Base::update();
			}
		}
		std::ostream& print(std::ostream& os) const override{
			return Base::print(os)<<"(en="<<en<<")";
		}
	};
	template<size_t Size>
	struct RegCLR:Reg<Size>{
		using Base=Reg<Size>;
		Enable clr;
		void update() override {
			if(clr.is_enable()){
				Base::reset();
			} else {
				Base::update();
			}
		}
		std::ostream& print(std::ostream& os) const override{
			return Base::print(os)<<"(clr="<<clr<<")";
		}
	};
	template<size_t Size>
	struct Nand:Component{
		Port<Size> A,B,Y;
		void update() override{
			Y=~(A.get()&B.get());
		}
	};
	template<size_t Size>
	struct Adder:Component{
		Port<Size> A,B,O;
		void update() override{
			O=A.get()+B.get();
		}
		std::ostream& print(std::ostream& os) const override{
			return os<<A<<"+"<<B<<"="<<O;
		}
	};
	template<size_t Size=8>
	struct ALU:Component{
		Port<Size> A,B,O;
		Port<1> CMS;
		Port<1> Co;
		void update() override {
			auto [carry,o]=ALU74181::run<Size>(static_cast<ALU74181::Carry>(CMS.sub<1>(5).get()),
			                                static_cast<ALU74181::Method>(CMS.sub<1>(4).get()),
			                                CMS.sub<4>(0).get(),
			                                A.get(),
			                                B.get());
			Co=static_cast<val_t>(carry);
			O=o;
		}

		std::ostream &print(std::ostream &os) const override {
			return os<<ALU74181::get_fn_str(static_cast<ALU74181::Carry>(CMS.sub<1>(5).get()),
			                                static_cast<ALU74181::Method>(CMS.sub<1>(4).get()),
			                                CMS.sub<4>(0).get(),
			                                std::to_string(A.get()),
			                                std::to_string(B.get()));
		}
	};
	/*
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
	};*/
/*
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
 */
	struct Sim:Circuit{
		Adder<8> adder{};
		RegCLR<8> reg{};
		Sim(){
			add_comps(adder,reg);
			add_wires(
					adder.O.wire(reg.input),
					adder.A.wire(reg.output)
			);
			adder.B.set(1);
			reg.clr.set(1);
			reg.clk.set(0);
		}
		void update() override{
			reg.clk.set(~reg.clk.get());
			Circuit::update();
		}
	};
}
#endif //BBCPU_COMPONENTS_H
