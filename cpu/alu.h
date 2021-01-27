//
// Created by chasingfar on 2021/1/26.
//

#ifndef BREADBOARDCPU_ALU_H
#define BREADBOARDCPU_ALU_H

namespace BreadBoardCPU {
	namespace ALU74181 {
		enum struct Carry{
			yes = 0,
			no = 1,
		};
		std::ostream &operator<<(std::ostream &os, Carry carry) {
			return os<<static_cast<unsigned>(carry);
		}
		enum struct Method{
			arithmetic = 0,
			logic = 1,
		};
		namespace Logic{
			enum struct fn{
				notA     = 0b0000u,
				AnorB    = 0b0001u,
				notAandB = 0b0010u,
				fill0    = 0b0011u,
				AnandB   = 0b0100u,
				notB     = 0b0101u,
				AxorB    = 0b0110u,
				AandnotB = 0b0111u,
				notAorB  = 0b1000u,
				notAxorB = 0b1001u,
				B        = 0b1010u,
				AandB    = 0b1011u,
				fill1    = 0b1100u,
				AornotB  = 0b1101u,
				AorB     = 0b1110u,
				A        = 0b1111u,
			};
			std::string fn_str[]={
					"~$A",
					"~($A|$B)",
					"(~$A)&$B",
					"0b00000000",
					"~($A&$B)",
					"~$B",
					"$A^$B",
					"$A&(~$B)",
					"~$A|$B",
					"(~$A)^$B",
					"$B",
					"$A&$B",
					"0b11111111",
					"$A|(~$B)",
					"$A|$B",
					"$A",
			};
		}
		namespace Arith{
			enum struct fn{
				A                     = 0b0000u,
				AorB                  = 0b0001u,
				AornotB               = 0b0010u,
				minus1                = 0b0011u,
				AplusAandnotB         = 0b0100u,
				AorBplusAandnotB      = 0b0101u,
				AminusBminus1         = 0b0110u,
				AandnotBminus1        = 0b0111u,
				AplusAandB            = 0b1000u,
				AplusB                = 0b1001u,
				AornotBplusAandB      = 0b1010u,
				AandBminus1           = 0b1011u,
				AplusA                = 0b1100u,
				AorBplusA             = 0b1101u,
				AornotBplusA          = 0b1110u,
				Aminus1               = 0b1111u,
			};
			std::string fn_str[]={
					"$A",
					"$A|B",
					"$A|(~B)",
					"-1",
					"$A+($A&(~B))",
					"($A|B)+($A&(~B))",
					"$A-B-1",
					"($A&(~B))-1",
					"$A+($A&B)",
					"$A+B",
					"($A|(~B))+($A&B)",
					"($A&B)-1",
					"$A+$A",
					"($A|B)+$A",
					"($A|(~B))+$A",
					"$A-1",
			};
		}
		template<size_t size,typename T,typename U>
		static std::pair<Carry,U> run(Carry Cn,Method M,T _S,U _A,U _B){
			/*U S_=U(S);
			size_t O=0;
			S_=(S_&(0b1100))|((S_&0b0001)<<1)|((S_&0b0010)>>1);
			S_^=(M==Method::logic?0b0011:0b0000);
			for(size_t i=0;i<size;i++){
				auto a=((A>>i)&0b1);
				auto b=((B>>i)&0b1);
				auto c=((a<<1)|b);
				auto d=((S_>>c)&0b1)<<i;
				O|=d;
			}
			if(M==Method::logic){
				return {Carry::no,O};
			}else{
				O+=A+(Cn==Carry::yes?1:0);
				return {(O>>size)&0b1u?Carry::yes:Carry::no,O};
			}*/
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
}
#endif //BREADBOARDCPU_ALU_H
