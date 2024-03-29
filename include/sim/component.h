#ifndef BBCPU_SIM_COMPONENT_H
#define BBCPU_SIM_COMPONENT_H

#include "port.h"
#include <numeric>
#include <sstream>
#include <unordered_set>

namespace BBCPU::Sim{

	struct Component{
		std::string name{};
		explicit Component(std::string name=""):name(std::move(name)){}

		using affected_t=std::unordered_set<Chip*>;
		virtual affected_t update()=0;
		virtual void run()=0;
		virtual std::optional<std::string> is_floating() const=0;

		virtual Util::Printer print() const{
			return [](std::ostream& os){};
		}
		friend std::ostream& operator<<(std::ostream& os,const Component& comp){
			return os<<comp.print();
		}
	};
	struct Chip:Component{
		inline static bool log_warning=false;
		inline static bool log_state=false;
		inline static bool log_change=false;
		inline static size_t run_count=0;
		inline static size_t err_count=0;
		static Util::Printer print_count(){ return [](std::ostream& os){os<<"run:"<<run_count<<",err:"<<err_count;};}

		using state_t=std::vector<Level>;
		std::vector<std::pair<Mode,Wire*>> pins;
		std::vector<std::pair<std::string,size_t>> port_names;
		bool input_floating=false;

		explicit Chip(std::string name=""):Component(std::move(name)){}

		std::optional<std::string> is_floating() const override{
			if(input_floating){
				return name;
			}
			return {};
		}
		template<size_t ...Sizes>
		void add_ports(Port<Sizes>&... ports){
			pins.reserve((pins.size()+...+Sizes));
			([&](auto& port){
				port.offset=pins.size();
				port_names.emplace_back(port.name,port.pins.size());
				for(auto& w:port.pins){
					w.chip=this;
					pins.emplace_back(port.mode,&w);
				}
			}(ports),...);
		}
		state_t save() const{
			state_t state{pins.size()};
			std::transform(pins.begin(),pins.end(),state.begin(),[](auto pin){
				return pin.second->get();
			});
			return state;
		}
		state_t is_need_update() const{
			state_t state;
			state.reserve(pins.size());
			for(auto [mode,wire]:pins){
				auto v=wire->get();
				if(mode==Mode::IN&&!read(v)){
					return {};
				}
				state.push_back(v);
			}
			return state;
		}
		affected_t get_affected(const state_t& before){
			affected_t affected;
			auto it=before.begin();
			for(auto [mode,p]:pins){
				if(*it!=p->get()){
					p->each([&](Wire* w){
						if(w->chip!=this&&w->chip!=nullptr){
							affected.emplace(w->chip);
						}
					});
				}
				++it;
			}
			return affected;
		}
		affected_t update() override{
			auto before=is_need_update();
			if(before.empty()){
				input_floating=true;
				if(log_state){ std::cout<<name<<"{"<<print(before)<<"}"<<std::endl; }
				return {};
			}
			try{
				++run_count;
				run();
				input_floating=false;
			}catch(const std::bad_variant_access& e){
				++err_count;
				input_floating=true;
				if(log_state){ std::cout<<name<<"{"<<print(before)<<"}"<<std::endl; }
				if(log_warning){ std::cerr<<"[Warning]"<<name<<"Read Floating"<<std::endl; }
				return {};
			}
			affected_t affected=get_affected(before);
			if(log_change){
				std::cout<<name<<"{"<<print(before)<<"}";
				if(!affected.empty()){
					std::cout<<"=>{"<<print()<<"}";
				}
				std::cout<<std::endl;
			}
			return affected;
		}

		virtual Util::Printer print(std::span<const Level> state) const{
			return [](std::ostream& os){};
		}
		Util::Printer print() const override{
			return [&](std::ostream& os){
				os<<print(save());
			};
		}
		void print_port_names(std::ostream& os1,std::ostream& os2){
			std::stringstream ss;
			for(auto [name,size]:port_names){
				ss<<std::setw(std::max(name.size(),size))<<name<<"|";
			}
			auto str=ss.str();
			os1<<std::setw(std::max(str.size(),name.size()+1))<<name+"|";
			os2<<std::setw(std::max(str.size(),name.size()+1))<<str;
		}
		void print_port_status(std::ostream& os){
			auto it=pins.begin();
			std::stringstream ss;
			for(auto [name,size]:port_names){
				std::string str="";
				for(size_t i=0;i<size;++i){
					Level level=it->second->get();
					if(level==Level::Floating){
						str="F"+str;
					}else if(level==Level::Error){
						str="E"+str;
					}else  if(static_cast<int8_t>(level)>0){
						str="1"+str;
					}else {
						str="0"+str;
					}
					++it;
				}
				ss<<std::setw(std::max(name.size(),size))<<str<<"|";
			}
			auto str=ss.str();
			os<<std::setw(std::max(str.size(),name.size()+1))<<str;
		}
	};
	struct Circuit:Component{
		inline static bool log_port_status=false;
		std::vector<Component*> comps;
		explicit Circuit(std::string name=""):Component(std::move(name)){}

		affected_t update() override{
			return std::reduce(comps.begin(),comps.end(),
				affected_t{},
				[](affected_t res,Component* c){
					res.merge(c->update());
					return res;
				});
		}
		std::vector<Chip*> get_chips(){
			std::vector<Chip*> chips;
			for(auto* comp:comps){
				if(auto* chip=dynamic_cast<Chip*>(comp);chip){
					chips.emplace_back(chip);
				}else if(auto* circuit=dynamic_cast<Circuit*>(comp);circuit){
					auto vec=circuit->get_chips();
					chips.insert(chips.end(),vec.begin(),vec.end());
				}
			}
			return chips;
		}
		void print_port_names(){
			auto chips=get_chips();
			std::stringstream ss;
			for(auto* chip:chips){
				chip->print_port_names(std::cout, ss);
			}
			std::cout<<std::endl;
			std::cout<<ss.str()<<std::endl;
		}
		void print_port_status(){
			auto chips=get_chips();
			for(auto* chip:chips){
				chip->print_port_status(std::cout);
			}
			std::cout<<std::endl;
		}
		void run() override {
			affected_t affected=update();
			while(!affected.empty()){
				if(log_port_status){
					print_port_status();
				}
				affected=std::reduce(affected.begin(),affected.end(),
				affected_t{},
				[](affected_t res,Chip* c){
					res.merge(c->update());
					return res;
				});
			}
			if(auto name=is_floating();name){
				if(Chip::log_warning){std::cerr<<"[Warning][Floating]"<<*name<<std::endl;}
			}
		}
		std::optional<std::string> is_floating() const override{
			using res_t=std::optional<std::string>;
			return std::reduce(comps.begin(),comps.end(),res_t{},[](auto res,auto c){
				return c->is_floating().and_then([&](auto name)->res_t{
					return name+res.value_or("");
				}).or_else([&](){
					return res;
				});
			});
		}

		template<typename ...Ts>
		requires ((std::is_base_of_v<Component,Ts>)&&...)
		void add_comps(Ts&... c){
			(comps.push_back(&c),...);
		}
	};
}
#endif //BBCPU_SIM_COMPONENT_H
