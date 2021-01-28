//
// Created by chasingfar on 2020/11/12.
//

#ifndef BREADBOARDCPU_OPCODE_H
#define BREADBOARDCPU_OPCODE_H

#include "mcode.h"
namespace BreadBoardCPU::OpCode {
	template <size_t Size,typename Ref,auto Id>
	using OPID=BitId<Id,BitField<Size,Ref,FollowMode::innerHigh> >;
	template <size_t Size,typename Ref>
	using OPField=BitField<Size,Ref,FollowMode::outerLow>;

	using layout_t=std::pair<std::string,size_t>;

	struct Base: MARG::opcode{
		template <typename T>
		static Reg16 getReg16(MCode ctx){ return static_cast<Reg16>(T::get(ctx.marg));}
		template <typename T>
		static UReg getUReg(MCode ctx){ return static_cast<UReg>(T::get(ctx.marg));}
	};

	template <auto V>
	struct Load:Base{//load value(8) from address(16) and push to stack
		inline static const std::string name="Load";
		using id    = OPID<5,Base,V>;
		using from  = OPField<3,id>;
		static layout_t parse(MCode ctx){
			Reg16 addr=getReg16<from>(ctx);
			return {
				name+" "+(addr==Reg16::TMP?"stack":std::string(addr)),
				addr==Reg16::IMM?3:1,
			};
		}
		static void gen(MCode& ctx){
			LOG("LOAD");
			Reg16 addr=getReg16<from>(ctx);
			if(addr==Reg16::IMM){//address from immediate value
				ctx.load_imm16(Reg16::TMP);
				addr=Reg16::TMP;
			}else if(addr==Reg16::TMP){//address from stack
				ctx.stack_pop16(Reg16::TMP);
			}
			ctx.load(addr,Reg::TMA);
			ctx.stack_push(Reg::TMA);
			ctx.next_op();
			ctx.end();
		}
	};
	template <auto V>
	struct Save:Base{//pop value(8) from stack and save to address(16)
		inline static const std::string name="Save";
		using id    = OPID<5,Base,V>;
		using to    = OPField<3,id>;
		static layout_t parse(MCode ctx){
			Reg16 addr=getReg16<to>(ctx);
			return {
				name+" "+(addr==Reg16::TMP?"stack":std::string(addr)),
				addr==Reg16::IMM?3:1,
			};
		}
		static void gen(MCode& ctx){
			LOG("Save");
			Reg16 addr=getReg16<to>(ctx);
			if(addr==Reg16::IMM){//address from immediate value
				ctx.load_imm16(Reg16::TMP);
				addr=Reg16::TMP;
			}else if(addr==Reg16::TMP){//address from stack
				ctx.stack_pop16(Reg16::TMP);
			}
			ctx.stack_pop(Reg::TMA);
			ctx.save(Reg::TMA,addr);
			ctx.next_op();
			ctx.end();
		}
	};
	template <auto V>
	struct ImmVal:Base{//push immediate value to stack
		inline static const std::string name="ImmVal";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,2};
		}
		static void gen(MCode& ctx){
			LOG("ImmVal");
			ctx.load_imm(Reg::TMA);
			ctx.stack_push(Reg::TMA);
			ctx.next_op();
			ctx.end();
		}
	};

	template <auto V>
	struct Calc:Base{
		inline static const std::string name="Calc";
		using id    = OPID<5,Base,V>;
		struct fn:OPField<3,id>{
			#define TABLE \
				X(SHL) X(SHR) \
				X(RCL) X(RCR) \
				X(ADD) X(SUB) \
				X(ADC) X(SUC) \
			
			enum fn_t{
				#define X(a) a,
					TABLE
				#undef X
			};
			inline static const std::string str[]={
				#define X(a) #a,
					TABLE
				#undef X
			};
			#undef TABLE
		};
		static layout_t parse(MCode ctx){
			return {name+" "+fn::str[fn::get(ctx.marg)],1};
		}
		static void gen(MCode& ctx){
			LOG("Calc");
			using Carry=MCTRL::alu::Carry;
			auto f = fn::template getAs<typename fn::fn_t>(ctx.marg);

			ctx.stack_pop(Reg::TML);
			if(f>=fn::ADD){
				ctx.stack_pop(Reg::TMH);
			}

			switch (f){
				case fn::SHL:
					ctx.shift_left(Reg::TML,Reg::TMA,0);
					break;
				case fn::SHR:
					ctx.shift_right(Reg::TML,Reg::TMA,0);
					break;
				case fn::RCL:
					ctx.shift_left(Reg::TML,Reg::TMA,MARG::getCF(ctx.marg));
					break;
				case fn::RCR:
					ctx.shift_right(Reg::TML,Reg::TMA,MARG::getCF(ctx.marg));
					break;
				case fn::ADD:
					ctx.add(Reg::TML,Reg::TMH,Reg::TMA);
					break;
				case fn::SUB:
					ctx.sub(Reg::TML,Reg::TMH,Reg::TMA);
					break;
				case fn::ADC:
					ctx.add(Reg::TML,Reg::TMH,Reg::TMA,MARG::getCF(ctx.marg));
					break;
				case fn::SUC:
					ctx.sub(Reg::TML,Reg::TMH,Reg::TMA,MARG::getCF(ctx.marg));
					break;
			}
			ctx.save_carry();
			ctx.stack_push(Reg::TMA);
			ctx.next_op();
			ctx.end();
		}
	};
	template <auto V>
	struct Logic:Base{
		inline static const std::string name="Logic";
		using id    = OPID<6,Base,V>;
		struct fn:OPField<2,id>{
			#define TABLE \
				X(NOT) \
				X(AND) \
				X(OR ) \
				X(XOR) \
			
			enum fn_t{
				#define X(a) a,
					TABLE
				#undef X
			};
			inline static const std::string str[]={
				#define X(a) #a,
					TABLE
				#undef X
			};
			#undef TABLE
		};
		static layout_t parse(MCode ctx){
			return {name+" "+fn::str[fn::get(ctx.marg)],1};
		}
		static void gen(MCode& ctx){
			LOG("Logic");
			auto f = fn::template getAs<typename fn::fn_t>(ctx.marg);

			ctx.stack_pop(Reg::TML);
			if(f>=fn::AND){
				ctx.stack_pop(Reg::TMH);
			}
			switch (f){
				case fn::NOT:
					ctx.logic_not(Reg::TML,Reg::TMA);
					break;
				case fn::AND:
					ctx.logic_and(Reg::TML,Reg::TMH,Reg::TMA);
					break;
				case fn::OR:
					ctx.logic_or(Reg::TML,Reg::TMH,Reg::TMA);
					break;
				case fn::XOR:
					ctx.logic_xor(Reg::TML,Reg::TMH,Reg::TMA);
					break;
			}
			ctx.stack_push(Reg::TMA);
			ctx.next_op();
			ctx.end();
		}
	};

	template <auto V>
	struct Push:Base{
		inline static const std::string name="Push";
		using id    = OPID<5,Base,V>;
		using from  = OPField<3,id>;
		static layout_t parse(MCode ctx){
			return {name+" "+UReg::str[from::get(ctx.marg)],1};
		}
		static void gen(MCode& ctx){
			LOG("Push");
			ctx.stack_push(Reg(getUReg<from>(ctx)));
			ctx.next_op();
			ctx.end();
		}
	};
	template <auto V>
	struct Pop:Base{
		inline static const std::string name="Pop";
		using id    = OPID<5,Base,V>;
		using to    = OPField<3,id>;
		static layout_t parse(MCode ctx){
			return {name+" "+UReg::str[to::get(ctx.marg)],1};
		}
		static void gen(MCode& ctx){
			LOG("Pop");
			ctx.stack_pop(Reg(getUReg<to>(ctx)));
			ctx.next_op();
			ctx.end();
		}
	};

	template <auto V>
	struct BranchZero:Base{
		inline static const std::string name="BranchZero";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,3};
		}
		static void gen(MCode& ctx){
			LOG("BranchZero");
			ctx.stack_pop(Reg::TMA);
			ctx.load_imm16(Reg16::TMP);
			ctx.branch_zero(Reg16::TMP,Reg::TMA,Reg::TMA);
			ctx.next_op();
			ctx.end();
		}
	};
	template <auto V>
	struct BranchCF:Base{
		inline static const std::string name="BranchCF";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,3};
		}
		static void gen(MCode& ctx){
			LOG("BranchCF");
			ctx.load_imm16(Reg16::TMP);
			ctx.branch(Reg16::TMP);
			ctx.next_op();
			ctx.end();
		}
	};
	template <auto V>
	struct Jump:Base{
		inline static const std::string name="Jump";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,3};
		}
		static void gen(MCode& ctx){
			LOG("Jump");
			ctx.load_imm16(Reg16::TMP);
			ctx.jump(Reg16::TMP);
			ctx.end();
		}
	};

	template <auto V>
	struct Call:Base{
		inline static const std::string name="Call";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,3};
		}
		static void gen(MCode& ctx){
			LOG("Call");
			ctx.load_imm16(Reg16::TMP);
			ctx.inc16(Reg16::PC);
			ctx.stack_push16(Reg16::PC);
			ctx.jump(Reg16::TMP);
			ctx.end();
		}
	};
	template <auto V>
	struct Return:Base{
		inline static const std::string name="Return";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,1};
		}
		static void gen(MCode& ctx){
			LOG("Return");
			ctx.stack_pop16(Reg16::TMP);
			ctx.jump(Reg16::TMP);
			ctx.end();
		}
	};
	template <auto V>
	struct Enter:Base{
		inline static const std::string name="Enter";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,2};
		}
		static void gen(MCode& ctx){
			LOG("Enter");
			ctx.stack_push16(Reg16::HL);
			ctx.copy16(Reg16::SP,Reg16::HL);
			ctx.load_imm(Reg::TMA);
			ctx.sub16(Reg16::SP,Reg::TMA,Reg16::SP);
			ctx.end();
		}
	};
	template <auto V>
	struct Adjust:Base{
		inline static const std::string name="Adjust";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,2};
		}
		static void gen(MCode& ctx){
			LOG("Adjust");
			ctx.load_imm(Reg::TMA);
			ctx.add16(Reg16::SP,Reg::TMA,Reg16::SP);
			ctx.end();
		}
	};
	template <auto V>
	struct Leave:Base{
		inline static const std::string name="Leave";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,1};
		}
		static void gen(MCode& ctx){
			LOG("Leave");
			ctx.copy16(Reg16::HL,Reg16::SP);
			ctx.stack_pop16(Reg16::HL);
			ctx.stack_pop16(Reg16::TMP);
			ctx.jump(Reg16::TMP);
			ctx.end();
		}
	};
	template <auto V>
	struct Local:Base{
		inline static const std::string name="Local";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,2};
		}
		static void gen(MCode& ctx){
			LOG("Leave");
			ctx.load_imm(Reg::TMA);
			ctx.add16(Reg16::HL,Reg::TMA,Reg16::TMP);
			ctx.stack_push16(Reg16::TMP);
			ctx.end();
		}
	};

	template <auto V>
	struct Interrupt:Base{
		inline static const std::string name="Interrupt";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,1};
		}
		static void gen(MCode& ctx){
			LOG("Interrupt");
			if(!ctx.isINT()){
				ctx.inc16(Reg16::PC);
			}
			ctx.stack_push16(Reg16::PC);
			ctx.set_zero(Reg::TMH);
			ctx.shift_left(Reg::OPR,Reg::TML,0);
			ctx.shift_left(Reg::TML,Reg::TML,0);
			ctx.shift_left(Reg::TML,Reg::TML,0);
			ctx.jump(Reg16::TMP);
			ctx.end();
		}
	};
	template <auto V>
	struct Initialize:Base{
		inline static const std::string name="Initialize";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,1};
		}
		static void gen(MCode& ctx){
			LOG("Init");
			ctx.dec16(Reg16::SP);// Reg16::SP-=1
			ctx.load_op();//load op from MEM[0]
			ctx.end();
		}
	};
	template <auto V>
	struct Halt:Base{
		inline static const std::string name="Halt";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,1};
		}
		static void gen(MCode& ctx){
			LOG("Halt");
			ctx.halt();
		}
	};
	struct Unknown:Base{
		using id    = OPID<8,Base,0>;
		static void gen(MCode& ctx){
			LOG("Unknown",std::bitset<8>(id::get(ctx.marg)));
			ctx.next_op();
			ctx.end();
		}
	};

	template <typename ...Ops>
	struct OpCode {
		template<typename T>
		static bool gen_if(MCode& ctx){
			if(T::id::test(ctx.marg)){
				T::gen(ctx);
				return true;
			}
			return false;
		}
		static bool gen_default(MCode& ctx){
			Unknown::gen(ctx);
			return true;
		}
		inline static size_t imax=0;
		static MCTRL::type gen(MARG::type marg) {
			MCode ctx(marg, 0, 0);
			LOG_START()
			( gen_if<Ops>(ctx) || ... || gen_default(ctx) );
			//((Ops::id::test(marg) && (Ops::gen(ctx), true))||...);
			if(ctx.i>imax){
				imax=ctx.i;
			}
			return ctx.mctrl;
		}
		template<typename T>
		static bool parse_if(MCode ctx,layout_t& result){
			if(T::id::test(ctx.marg)){
				result=T::parse(ctx);
				return true;
			}
			return false;
		}
		static layout_t parse(auto op){
			MCode ctx(MARG::opcode::set(0,op), 0, 0);
			layout_t result{"Unknown",1};
			( parse_if<Ops>(ctx,result) || ... );
			return result;
		}
	};
	
	namespace Ops{
		using Load       = Load       <0b11111>;
		using Save       = Save       <0b11110>;
		using Push       = Push       <0b11101>;
		using Pop        = Pop        <0b11100>;
		using Calc       = Calc       <0b11011>;
		using Logic      = Logic      <0b110101>;
		using BranchCF   = BranchCF   <0b11010011>;
		using BranchZero = BranchZero <0b11010010>;
		using ImmVal     = ImmVal     <0b11010001>;
		using Jump       = Jump       <0b11010000>;
		using Call       = Call       <0b11001111>;
		using Return     = Return     <0b11001110>;
		using Halt       = Halt       <0b11001101>;

		using Enter      = Enter      <0b11001100>;
		using Adjust     = Adjust     <0b11001011>;
		using Leave      = Leave      <0b11001010>;
		using Local      = Local      <0b11001001>;

		using INT0       = Interrupt  <0b00001000>;
		using INT1       = Interrupt  <0b00001001>;
		using INT2       = Interrupt  <0b00001010>;
		using INT3       = Interrupt  <0b00001011>;
		using INT4       = Interrupt  <0b00001100>;
		using INT5       = Interrupt  <0b00001101>;
		using INT6       = Interrupt  <0b00001110>;
		using INT7       = Interrupt  <0b00001111>;
		using Initialize = Initialize <0b00000000>;
		using all=OpCode<
				Load,Save,
				Push,Pop,
				Calc,Logic,
				BranchCF,BranchZero,
				ImmVal,
				Jump,Call,Return,
				Enter,Adjust,Leave,Local,
				Halt,
				INT0,INT1,INT2,INT3,
				INT4,INT5,INT6,INT7,
				Initialize
		>;
	}

	void generateOPROM(){
		std::ofstream fout("oprom.txt");
		if(!fout) {return;}
		Ops::all::imax=0;
		fout<<ROM(TruthTable<MARG::type,MCTRL::type,MARG::size>(std::function{[](MARG::type marg){
			std::cout<<std::bitset<MARG::size>(marg);
			auto mctrl=Ops::all::gen(marg);
			std::cout<<"=>"<<std::bitset<MCTRL::size>(mctrl)<<std::endl;
			return mctrl;
		}}));
		std::cout<<"imax="<<Ops::all::imax<<std::endl;
	}

};
#endif //BREADBOARDCPU_OPCODE_H
