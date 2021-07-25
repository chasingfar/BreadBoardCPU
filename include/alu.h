//
// Created by chasingfar on 2021/1/26.
//

#ifndef BREADBOARDCPU_ALU_H
#define BREADBOARDCPU_ALU_H

#include <bitset>
#include <iostream>

namespace ALU74181 {
	enum struct Carry{
		yes = 0,
		no = 1,
	};
	inline std::ostream &operator<<(std::ostream &os, Carry carry) {
		return os<<(carry==Carry::yes?"carry":"no_carry");
	}
	enum struct Method{
		arithmetic = 0,
		logic = 1,
	};
	inline std::ostream &operator<<(std::ostream &os, Method method) {
		return os<<(method==Method::arithmetic?"arithmetic":"logic");
	}
	struct Logic{
		#define FN_TABLE \
			X(notA     ,= 0b0000u,"~$A"        ) \
			X(AnorB    ,= 0b0001u,"~($A|$B)"   ) \
			X(notAandB ,= 0b0010u,"(~$A)&$B"   ) \
			X(fill0    ,= 0b0011u,"0b00000000" ) \
			X(AnandB   ,= 0b0100u,"~($A&$B)"   ) \
			X(notB     ,= 0b0101u,"~$B"        ) \
			X(AxorB    ,= 0b0110u,"$A^$B"      ) \
			X(AandnotB ,= 0b0111u,"$A&(~$B)"   ) \
			X(notAorB  ,= 0b1000u,"~$A|$B"     ) \
			X(notAxorB ,= 0b1001u,"(~$A)^$B"   ) \
			X(B        ,= 0b1010u,"$B"         ) \
			X(AandB    ,= 0b1011u,"$A&$B"      ) \
			X(fill1    ,= 0b1100u,"0b11111111" ) \
			X(AornotB  ,= 0b1101u,"$A|(~$B)"   ) \
			X(AorB     ,= 0b1110u,"$A|$B"      ) \
			X(A        ,= 0b1111u,"$A"         ) \

		enum fn{
			#define X(a, b, c) a b,
				FN_TABLE
			#undef X
		};
		inline static const std::string fn_str[]={
			#define X(a, b, c) c,
				FN_TABLE
			#undef X
		};
		#undef FN_TABLE
		fn S;
		Logic() = default;
		constexpr Logic(fn S) : S(S) { }
		constexpr bool operator==(Logic a) const { return S == a.S; }
		constexpr bool operator!=(Logic a) const { return S != a.S; }
		operator std::string() const {
			return fn_str[S];
		}
	};
	struct Arith{
		#define FN_TABLE \
			X(A                ,= 0b0000u,"$A"               ) \
			X(AorB             ,= 0b0001u,"$A|$B"             ) \
			X(AornotB          ,= 0b0010u,"$A|(~$B)"          ) \
			X(minus1           ,= 0b0011u,"-1"               ) \
			X(AplusAandnotB    ,= 0b0100u,"$A+($A&(~$B))"     ) \
			X(AorBplusAandnotB ,= 0b0101u,"($A|B)+($A&(~$B))" ) \
			X(AminusBminus1    ,= 0b0110u,"$A-$B-1"           ) \
			X(AandnotBminus1   ,= 0b0111u,"($A&(~$B))-1"      ) \
			X(AplusAandB       ,= 0b1000u,"$A+($A&$B)"        ) \
			X(AplusB           ,= 0b1001u,"$A+$B"             ) \
			X(AornotBplusAandB ,= 0b1010u,"($A|(~$B))+($A&$B)" ) \
			X(AandBminus1      ,= 0b1011u,"($A&$B)-1"         ) \
			X(AplusA           ,= 0b1100u,"$A+$A"            ) \
			X(AorBplusA        ,= 0b1101u,"($A|$B)+$A"        ) \
			X(AornotBplusA     ,= 0b1110u,"($A|(~$B))+$A"     ) \
			X(Aminus1          ,= 0b1111u,"$A-1"             ) \

		enum fn{
			#define X(a, b, c) a b,
				FN_TABLE
			#undef X
		};
		inline static const std::string fn_str[]={
			#define X(a, b, c) c,
				FN_TABLE
			#undef X
		};
		#undef FN_TABLE
		fn S;
		Arith() = default;
		constexpr Arith(fn S) : S(S) { }
		constexpr bool operator==(Arith a) const { return S == a.S; }
		constexpr bool operator!=(Arith a) const { return S != a.S; }
		operator std::string() const {
			return fn_str[S];
		}
	};

	template<size_t size,typename T,typename U>
	static std::pair<Carry,U> run(Carry Cn,Method M,T _S,U _A,U _B){
		std::bitset<4> S{_S};
		std::bitset<size> F,A{_A},B{_B};
		std::bitset<size+1> C;
		bool M_=M==Method::arithmetic;
		C[0]=Cn==Carry::no;
		for(size_t i=0;i<size;++i){
			bool P_=!A[i] && (!S[1] || B[i]) && (!S[0] || !B[i]);
			bool G_=(!S[3] || !A[i] || !B[i]) && (!S[2] || !A[i] || B[i]);
			C[i+1]=(C[i]&&G_)||P_;
			F[i]=(G_&&!P_)^!(C[i]&&M_);
		}
		return {C[size]? Carry::no : Carry::yes, static_cast<U>(F.to_ullong())};
	}

	template<typename T,typename U>
	static std::string get_fn_str(Carry Cn,Method M,T S,U A,U B){
		std::string str{M==Method::logic?Logic::fn_str[S]:Arith::fn_str[S]};
		auto s=str.find("$A");
		while(s!=std::string::npos){
			str.replace(s,2,A);
			s=str.find("$A");
		}
		s=str.find("$B");
		while(s!=std::string::npos){
			str.replace(s,2,B);
			s=str.find("$B");
		}
		if(M==Method::arithmetic&&Cn==Carry::yes){
			s=str.rfind("-1");
			if(s!=std::string::npos){
				str.erase(s,2);
				if(str.empty()){
					str="0";
				}
			}else{
				str+="+1";
			}
		}
		return str;
	}
}
#endif //BREADBOARDCPU_ALU_H
