#ifndef BBCPU_SIM_COMPONENT_H
#define BBCPU_SIM_COMPONENT_H

#include "port.h"

namespace BBCPU::Sim{

	struct Component{
		std::string name{};
		explicit Component(std::string name=""):name(std::move(name)){}

		virtual bool update()=0;
		virtual void run()=0;

		virtual Util::Printer print() const{
			return [](std::ostream& os){};
		}
		friend std::ostream& operator<<(std::ostream& os,const Component& comp){
			return os<<comp.print();
		}
	};
	struct Chip:Component{
		inline static bool log_read_floating=false;
		inline static bool log_state=false;
		inline static bool log_change=false;

		std::vector<Wire*> pins{};
		std::vector<Level> last_state{};

		explicit Chip(std::string name=""):Component(std::move(name)){}

		auto save() const{
			std::vector<Level> state{pins.size()};
			std::transform(pins.begin(),pins.end(),state.begin(),[](Wire* w){
				return w->get();
			});
			return state;
		}
		bool update() override{
			auto before=save();
			if(before==last_state){
				return false;
			}
			try{
				run();
			}catch(const std::bad_optional_access& e){
				if(log_read_floating){ std::cout<<"[Warning]"<<name<<"Read Floating"<<std::endl; }
				return false;
			}
			auto new_state=save();
			if(before!=new_state){
				last_state=new_state;
				if(log_change){ std::cout<<name<<"{"<<print(before)<<"}=>{"<<print(new_state)<<"}"<<std::endl; }
				return true;
			}
			if(log_state){std::cout<<name<<"{"<<print(before)<<"}"<<std::endl;}
			return false;
		}

		template<size_t ...Sizes>
		void add_ports(Port<Sizes>&... ports){
			pins.reserve((pins.size()+...+Sizes));
			([&](auto& port){
				port.offset=pins.size();
				for(auto& w:port.pins){
					pins.push_back(&w);
				}
			}(ports),...);
		}
		virtual Util::Printer print(std::span<const Level> state) const{
			return [](std::ostream& os){};
		}
		Util::Printer print() const override{
			return [&](std::ostream& os){
				os<<print(save());
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
		}

		template<typename ...Ts>
		requires ((std::is_base_of_v<Component,Ts>)&&...)
		void add_comps(Ts&... c){
			(comps.push_back(&c),...);
		}
	};
}
#endif //BBCPU_SIM_COMPONENT_H
