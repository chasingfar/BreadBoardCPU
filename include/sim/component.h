#ifndef BBCPU_SIM_COMPONENT_H
#define BBCPU_SIM_COMPONENT_H

#include "port.h"

namespace BBCPU::Sim{

	struct Component{
		std::string name{};
		explicit Component(std::string name=""):name(std::move(name)){}

		virtual bool update()=0;
		virtual void run()=0;
		virtual std::optional<std::string> is_floating() const=0;

		virtual Util::Printer print() const{
			return [](std::ostream& os){};
		}
		friend std::ostream& operator<<(std::ostream& os,const Component& comp){
			return os<<comp.print();
		}
	};
	
	struct PinsState{
		using state_t=std::vector<Level>;
		std::vector<std::pair<Mode,Wire*>> pins;
		state_t last_state;
		
		template<size_t ...Sizes>
		void add_ports(Port<Sizes>&... ports){
			pins.reserve((pins.size()+...+Sizes));
			([&](auto& port){
				port.offset=pins.size();
				for(auto& w:port.pins){
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
		enum struct Reason{Floating,NoChange};
		Util::Result<state_t,Reason> is_need_update() const{
			bool has_change=false;
			state_t state;
			state.reserve(pins.size());
			auto it=last_state.begin();
			auto end=last_state.end();
			for(auto [mode,wire]:pins){
				Level v=wire->get();
				switch(mode){
					case Mode::IN:
						if(!read(v).has_value()){ return Reason::Floating; }
					case Mode::IO:
						if(it==end||*it!=v){ has_change=true; }
					case Mode::OUT:
						state.emplace_back(v);
						if(it!=end){ ++it; }
                }
			}
			if(!has_change){ return Reason::NoChange; }
			return state;
		}
	};
	struct Chip:Component{
		inline static bool log_warning=false;
		inline static bool log_state=false;
		inline static bool log_change=false;
		inline static size_t run_count=0;
		inline static size_t err_count=0;
		static Util::Printer print_count(){ return [](std::ostream& os){os<<"run:"<<run_count<<",err:"<<err_count;};}

		PinsState ports;
		bool input_floating=false;

		explicit Chip(std::string name=""):Component(std::move(name)){}

		std::optional<std::string> is_floating() const override{
			if(input_floating){
				return name;
			}
			return {};
		}

		bool update() override{
			auto before = ports.is_need_update();
			if(!before){
				if(before.error()==PinsState::Reason::Floating){
					input_floating=true;
					return false;
				}
				input_floating=false;
				return false;
			}
			try{
				++run_count;
				run();
				input_floating=false;
			}catch(const std::bad_optional_access& e){
				++err_count;
				input_floating=true;
				if(log_warning){ std::cerr<<"[Warning]"<<name<<"Read Floating"<<std::endl; }
				return false;
			}
			auto new_state=ports.save();
			if(*before!=new_state){
				if(log_change){
					std::cout<<name;
					if(!ports.last_state.empty()){
						std::cout<<"{"<<print(ports.last_state)<<"}=>";
					}
					std::cout<<"{"<<print(*before)<<"}=>{"<<print(new_state)<<"}"<<std::endl; }
				ports.last_state=new_state;
				return true;
			}
			if(log_state){std::cout<<name<<"{"<<print(*before)<<"}"<<std::endl;}
			return false;
		}

		virtual Util::Printer print(std::span<const Level> state) const{
			return [](std::ostream& os){};
		}
		Util::Printer print() const override{
			return [&](std::ostream& os){
				os<<print(ports.save());
			};
		}
	};
	struct Circuit:Component{
		std::vector<Component*> comps;
		explicit Circuit(std::string name=""):Component(std::move(name)){}

		bool update() override{
			return std::any_of(comps.begin(),comps.end(),[](auto c){
				return c->update();
			});
		}
		void run() override {
			while(update()){}
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
