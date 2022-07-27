#ifndef BBCPU_SIM_WIRE_H
#define BBCPU_SIM_WIRE_H

#include <optional>
#include "../util.h"

namespace BBCPU::Sim{
	struct Chip;
	enum struct Level:int8_t{
		Low      = -2,
		PullDown = -1,
		Floating =  0,
		PullUp   =  1,
		High     =  2,
		Error    = INT8_MAX,
	};
	inline Level& operator+=(Level& o,Level n){
		/*
o\n	L	H	D	U	Z	E
L	L1	E3	L4	L4	L4	E2
H	E3	H1	H4	H4	H4	E2
D	L2	H2	D1	E3	D4	E2
U	L2	H2	E3	U1	U4	E2
Z	L2	H2	D2	U2	Z1	E2
E	E4	E4	E4	E4	E4	E1
		*/
		if(n==o){return o;}//1
		int cmp=abs(static_cast<int8_t>(n))-abs(static_cast<int8_t>(o));
		if(cmp>0){//2
			return o=n;
		}else if(cmp==0){//3
			return o=Level::Error;
		}
		return o;//4
	}
	inline Level operator+(Level o,Level n){
		Level t=o;
		return t+=n;
	}
	inline Util::Result<uint8_t,Level> read(const Level level){
		if(level==Level::Floating || level==Level::Error){
			return level;
		}
		return static_cast<uint8_t>(static_cast<int8_t>(level) > 0 ? 1 : 0);
	}
	struct Wire:Util::CircularList<Wire>{
		Level level=Level::Floating;
		Chip* chip=nullptr;
		Level get() const{
			Level tmp=level;
			each([&tmp](const Wire* cur){
				tmp+=cur->level;
			});
			return tmp;
		}
		void set(Level new_level){
			if(level!=new_level){
				level=new_level;
			}
		}
		Wire& operator=(Level new_level){
			set(new_level);
			return *this;
		}
	};
	inline Util::Result<uint8_t,Level> read(const Wire& wire){
		return read(wire.get());
	}
}

#endif //BBCPU_SIM_WIRE_H
