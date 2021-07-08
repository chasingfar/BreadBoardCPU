//
// Created by chasingfar on 2021/2/1.
//

#ifndef BREADBOARDCPU_UTIL_H
#define BREADBOARDCPU_UTIL_H

#include <variant>
#include <functional>
#include <iostream>
#include <fstream>

namespace Util{
	// helper type for the std::variant visitor
	template<class... Ts> struct lambda_compose : Ts... { using Ts::operator()...; };
	// explicit deduction guide (not needed as of C++20)
	template<class... Ts> lambda_compose(Ts...) -> lambda_compose<Ts...>;

	template <typename T>
	struct flat_vector:public std::vector<T>{
		flat_vector(std::initializer_list<std::variant<T,flat_vector<T>>> vec){
			add(vec);
		}
		void add(std::initializer_list<std::variant<T,flat_vector<T>>> vec){
			for(auto& elem:vec){
				std::visit(lambda_compose{
					[&](flat_vector<T> v){this->insert(this->end(),v.begin(),v.end());},
					[&](T t){this->push_back(t);},
				},elem);
			}
		}
		flat_vector<T>& operator<<(flat_vector<T> v){
			this->insert(this->end(),v.begin(),v.end());
			return *this;
		}
	};

	template<typename IN,typename OUT,size_t S=0>
	struct TruthTable{
		std::function<OUT(IN)> fn;
		struct iterator{
			size_t in;
			TruthTable& table;
			iterator(TruthTable& table,size_t in):in(in),table(table){}
			OUT operator*() const { return table(in); }

			// Prefix increment
			iterator& operator++() { ++in; return *this; }

			friend bool operator== (const iterator& a, const iterator& b) { return (a.in == b.in)/*&&(a.table==b.table)*/; };
			friend bool operator!= (const iterator& a, const iterator& b) { return !(a==b); };
		};
		explicit TruthTable(std::function<OUT(IN)> fn):fn(fn){}
		OUT operator ()(IN in) const { return fn(in); }
		iterator begin() { return iterator(*this,0); }
		iterator end()   { return iterator(*this,IN(1ull<<S)); }
	};
	template<typename IN,typename OUT> TruthTable(std::function<OUT(IN)>)
	->TruthTable<typename IN::type,typename OUT::type,IN::size>;

	template<typename T>
	struct ROM{
		T data;
		explicit ROM(T data):data(data){}
		friend std::ostream& operator<<(std::ostream& os,ROM rom){
			os<<"v2.0 raw"<<std::endl;
			size_t i=0;
			for(auto out:rom.data){
				os<<std::hex<< static_cast<size_t>(out);
				if(i%8==7){
					os<<std::endl;
				}else{
					os<<" ";
				}
				++i;
			}
			return os;
		}
	};

	template<typename IN,typename OUT>
	void generateROM(std::ostream& os,std::function<OUT(IN)> program){
		os<<"v2.0 raw"<<std::endl;
		IN in{0};
		do{
			auto out=program(in);
			os<<std::hex<<out;
			if((in&7u)==7){
				os<<std::endl;
			}else{
				os<<" ";
			}
			if(in>= (1ull<<19u)-1){
				break;
			}
			in++;
		}while(true);
	}
}
#endif //BREADBOARDCPU_UTIL_H
