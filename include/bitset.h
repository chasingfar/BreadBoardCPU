//
// Created by chasingfar on 2020/10/18.
//

#ifndef UTIL_BITSET_H
#define UTIL_BITSET_H

#include <bitset>
#include <climits>

namespace Util{
	template <size_t N>
	struct bitset_wrap : public std::bitset<N>{
		typedef std::bitset<N> base_t;
		using base_t::base_t;
		bitset_wrap<N>(const base_t & cx) : base_t(cx) {}
		using base_t::operator==;
		using base_t::operator[];
		using base_t::operator&=;
		using base_t::operator|=;
		using base_t::operator^=;
		using base_t::operator~;
		using base_t::operator<<=;
		using base_t::operator>>=;
		using base_t::operator<<;
		using base_t::operator>>;
		using base_t::test;
		using base_t::all;
		using base_t::any;
		using base_t::none;
		using base_t::count;
		using base_t::size;
		using base_t::set;
		using base_t::reset;
		using base_t::flip;
		using base_t::to_string;
		using base_t::to_ulong;
		using base_t::to_ullong;
		explicit operator unsigned long long(){
			return to_ullong();
		}
		template <size_t M>
		operator bitset_wrap<M>(){
			bitset_wrap<M> temp{0};
			for(size_t i=0;i<N;++i){
				temp.set(i,test(i));
			}
			return temp;
		}
		/*
		template <size_t M>
		bool operator ==(const bitset_wrap<M> &b) const{
			for(size_t i=0;i<())
		}*/
		template <typename T>
		bitset_wrap<N>& operator+=(T rhs){
			using U=unsigned long long;
			if constexpr (N < sizeof(U)*CHAR_BIT){
				if( std::is_convertible<T, U>::value){
					*this={to_ullong()+U(rhs)};
				}else{
					*this={to_ullong()+std::bitset<N>(rhs).to_ullong()};
				}
			}else{
				std::bitset<N> tmp(rhs);
				bool c = false;
				for(size_t i=0;i<N;++i){
					bool a = *this[i];
					bool b = tmp[i];
					tmp.set(i,a^b^c);
					c=(a&b)|(b&c)|(c&a);
				}
				*this=tmp;
			}
			return *this;
		}
		bitset_wrap<N>& operator++(){
			*this+=1;
			return *this;
		}
		bitset_wrap<N>  operator++(int){
			const auto temp = *this;
			++*this;
			return temp;
		}
		template <typename T>
		bitset_wrap<N>  operator+(T rhs){
			auto temp = *this;
			temp+=rhs;
			return temp;

		}
		template <class CharT, class Traits>
		friend std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os,
		                                              const bitset_wrap<N>& x){
			auto base=os.flags()& os.basefield;
			if(base != os.hex){
				return os<<x.to_string();
			}
			using U=unsigned long long;
			size_t S=(N+sizeof(U)*CHAR_BIT -1)/(sizeof(U)*CHAR_BIT);
			for(size_t i=0;i<S;++i){
				os<<(x>>((S-i-1)*(sizeof(U)*CHAR_BIT))).to_ullong();
			}
			return os;
		}
	};
}
#endif //UTIL_BITSET_H
