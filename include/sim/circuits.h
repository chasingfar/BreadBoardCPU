#ifndef BBCPU_SIM_CIRCUITS_H
#define BBCPU_SIM_CIRCUITS_H
#include <numeric>
#include <map>
#include <iomanip>
#include <sstream>
#include <ostream>
#include <string>
#include "../alu.h"
#include "chips.h"

namespace BBCPU::Sim{

	template<size_t ASize,size_t DSize,size_t STSize,size_t STInOff,size_t STOutOff>
	struct CUBase:Circuit{
		Clock clk;
		Port<1> clr;

		RegCLR<ASize> sreg{name+"[sReg]"};
		ROM<ASize,DSize> tbl{name+"[TBL]"};
		explicit CUBase(std::string name=""):Circuit(std::move(name)){
			add_comps(sreg,tbl);

			clk.wire(sreg.clk);
			clr.wire(sreg.clr);

			(tbl.D.template sub<STSize>(STOutOff)).wire(sreg.input.template sub<STSize>(STInOff));
			sreg.output.wire(tbl.A);

			tbl.ce.set(0);
			tbl.we.set(1);
			tbl.oe.set(0);
		}
	};
	template<
			size_t ASize,size_t DSize,
			size_t CSize,size_t COff,size_t CVal,
			typename addr_t=size_t,typename data_t=val_t
	>
	struct Memory:Circuit{
		constexpr static addr_t addr_min=0;
		constexpr static addr_t addr_max=(1<<ASize)-1;

		constexpr static addr_t ram_max=addr_max;
		constexpr static addr_t ram_min=(CVal+1)<<COff;

		constexpr static addr_t dev_max=ram_min-1;
		constexpr static addr_t dev_min=CVal<<COff;

		constexpr static addr_t rom_max=dev_min-1;
		constexpr static addr_t rom_min=0;

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
		void load(const std::vector<data_t>& new_data,addr_t start=0){
			size_t end=start+new_data.size();
			if(is_rom(start)&&is_rom(end)){
				rom.load(new_data,start);
			}
			if(is_ram(start)&&is_ram(end)){
				ram.load(new_data,start);
			}
		}
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
#endif //BBCPU_SIM_CIRCUITS_H
