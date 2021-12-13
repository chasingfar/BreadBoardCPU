//
// Created by chasingfar on 2020/11/12.
//

#ifndef BBCPU_CPU_OPCODE_H
#define BBCPU_CPU_OPCODE_H

#include "mcode.h"
namespace BBCPU::OpCode {
	template <size_t Size,typename Ref,auto Id>
	using OPID=BitId<Id,BitField<Size,Ref,FollowMode::innerHigh> >;
	template <size_t Size,typename As,typename Ref,typename Base=BitField<Size,Ref,FollowMode::outerLow>>
	struct OPField:Base{
		static auto set(auto o,As v){
			return Base::set(o,static_cast<decltype(o)>(v));
		}
		static auto get(auto o){
			return static_cast<As>(Base::get(o));
		}
	};

	using layout_t=std::pair<std::string,size_t>;

	struct Base: MARG::opcode{
		template <typename T>
		static Reg16 getReg16(MCode ctx){ return static_cast<Reg16>(T::get(ctx.marg));}
		template <typename T>
		static UReg getUReg(MCode ctx){ return static_cast<UReg>(T::get(ctx.marg));}
		template <typename T>
		static UReg16 getUReg16(MCode ctx){ return static_cast<UReg16>(T::get(ctx.marg));}
	};

	template <auto V>
	struct Load:Base{//load value(8) from address(16) with offset(8) and push to stack
		inline static const std::string name="Load";
		using id    = OPID<6,Base,V>;
		using from  = OPField<2,UReg16,id>;
		static layout_t parse(MCode ctx){
			return {name+" "+getUReg16<from>(ctx).str(),3};
		}
		static void gen(MCode& ctx){
			LOG("LOAD");
			UReg16 addr=getUReg16<from>(ctx);
			ctx.load_imm16(Reg16::TMP);
			ctx.add16(addr.toReg16(),Reg16::TMP,Reg16::TMP);
			ctx.load(Reg16::TMP,Reg::TMA);
			ctx.stack_push(Reg::TMA);
			ctx.next_op();
		}
	};
	template <auto V>
	struct Save:Base{//pop value(8) from stack and save to address(16)
		inline static const std::string name="Save";
		using id    = OPID<6,Base,V>;
		using to    = OPField<2,UReg16,id>;
		static layout_t parse(MCode ctx){
			return {name+" "+getUReg16<to>(ctx).str(),3};
		}
		static void gen(MCode& ctx){
			LOG("Save");
			UReg16 addr=getUReg16<to>(ctx);
			ctx.load_imm16(Reg16::TMP);
			ctx.add16(addr.toReg16(),Reg16::TMP,Reg16::TMP);
			ctx.stack_pop(Reg::TMA);
			ctx.save(Reg::TMA,Reg16::TMP);
			ctx.next_op();
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
		}
	};

	template <auto V>
	struct Calc:Base{
		inline static const std::string name="Calc";
		using id    = OPID<5,Base,V>;
#define ENUM_HIDE_TYPE struct
#define ENUM_NAME(...) FN##__VA_ARGS__
#define ENUM_TABLE \
		X(SHL) X(SHR) \
		X(RCL) X(RCR) \
		X(ADD) X(SUB) \
		X(ADC) X(SUC) \

#include "define_enum_x.h"
		using fn = OPField<3,FN,id>;
		static layout_t parse(MCode ctx){
			return {name+" "+std::string(fn::get(ctx.marg)),1};
		}
		static void gen(MCode& ctx){
			LOG("Calc");
			auto f = fn::get(ctx.marg);
#define ARG_1 ctx.stack_pop(Reg::TML);
#define ARG_2 ARG_1 ctx.stack_pop(Reg::TMH);
#define CALC_1(fn,name)  case FN::fn: ARG_1 ctx.name(Reg::TML,Reg::TMA,0);break;
#define CALC_1C(fn,name) case FN::fn: ARG_1 ctx.name(Reg::TML,Reg::TMA,MARG::getCF(ctx.marg));break;
#define CALC_2(fn,name)  case FN::fn: ARG_2 ctx.name(Reg::TMH,Reg::TML,Reg::TMA);break;
#define CALC_2C(fn,name) case FN::fn: ARG_2 ctx.name(Reg::TMH,Reg::TML,Reg::TMA,MARG::getCF(ctx.marg));break;

			switch (f){
				CALC_1( SHL,shift_left)
				CALC_1( SHR,shift_right)
				CALC_1C(RCL,shift_left)
				CALC_1C(RCR,shift_right)
				CALC_2( ADD,add)
				CALC_2( SUB,sub)
				CALC_2C(ADC,add)
				CALC_2C(SUC,sub)
			}
#undef ARG_1
#undef ARG_2
#undef CALC_1
#undef CALC_1C
#undef CALC_2
#undef CALC_2C
			ctx.save_carry();
			ctx.stack_push(Reg::TMA);
			ctx.next_op();
		}
	};
	template <auto V>
	struct Logic:Base{
		inline static const std::string name="Logic";
		using id    = OPID<6,Base,V>;
#define ENUM_HIDE_TYPE struct
#define ENUM_NAME(...) FN##__VA_ARGS__
#define ENUM_TABLE \
		X(NOT) \
		X(AND) \
		X(OR ) \
		X(XOR) \

#include "define_enum_x.h"
		using fn = OPField<2,FN, id>;
		static layout_t parse(MCode ctx){
			return {name+" "+fn::get(ctx.marg).str(),1};
		}
		static void gen(MCode& ctx){
			LOG("Logic");
			FN f = fn::get(ctx.marg);

#define ARG_1 ctx.stack_pop(Reg::TML);
#define ARG_2 ARG_1 ctx.stack_pop(Reg::TMH);
#define LOGIC_1(fn,name)  case FN::fn: ARG_1 ctx.name(Reg::TML,Reg::TMA);break;
#define LOGIC_2(fn,name)  case FN::fn: ARG_2 ctx.name(Reg::TMH,Reg::TML,Reg::TMA);break;
			switch (f){
				LOGIC_1(NOT,logic_not)
				LOGIC_2(AND,logic_and)
				LOGIC_2(OR ,logic_or )
				LOGIC_2(XOR,logic_xor)
			}
#undef ARG_1
#undef ARG_2
#undef LOGIC_1
#undef LOGIC_2

			ctx.stack_push(Reg::TMA);
			ctx.next_op();
		}
	};

	template <auto V>
	struct Push:Base{
		inline static const std::string name="Push";
		using id    = OPID<5,Base,V>;
		using from  = OPField<3,UReg,id>;
		static layout_t parse(MCode ctx){
			return {name+" "+from::get(ctx.marg).str(),1};
		}
		static void gen(MCode& ctx){
			LOG("Push");
			ctx.stack_push(user(getUReg<from>(ctx)));
			ctx.next_op();
		}
	};
	template <auto V>
	struct Pop:Base{
		inline static const std::string name="Pop";
		using id    = OPID<5,Base,V>;
		using to    = OPField<3,UReg,id>;
		static layout_t parse(MCode ctx){
			return {name+" "+to::get(ctx.marg).str(),1};
		}
		static void gen(MCode& ctx){
			LOG("Pop");
			ctx.stack_pop(user(getUReg<to>(ctx)));
			ctx.next_op();
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
			ctx.branch_zero(Reg16::TMP,Reg::TMA);
			ctx.next_op();
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
		}
	};
	template <auto V>
	struct Enter:Base{
		inline static const std::string name="Enter";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,3};
		}
		static void gen(MCode& ctx){
			LOG("Enter");
			ctx.stack_push16(Reg16::HL);
			ctx.copy16(Reg16::SP,Reg16::HL);
			ctx.load_imm16(Reg16::TMP);
			ctx.sub16(Reg16::SP,Reg16::TMP,Reg16::SP);
			ctx.next_op();
		}
	};
	template <auto V>
	struct Adjust:Base{
		inline static const std::string name="Adjust";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,3};
		}
		static void gen(MCode& ctx){
			LOG("Adjust");
			ctx.load_imm16(Reg16::TMP);
			ctx.add16(Reg16::SP,Reg16::TMP,Reg16::SP);
			ctx.next_op();
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
		}
	};
	template <auto V>
	struct Local:Base{
		inline static const std::string name="Local";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,3};
		}
		static void gen(MCode& ctx){
			LOG("Local");
			ctx.load_imm16(Reg16::TMP);
			ctx.add16(Reg16::HL,Reg16::TMP,Reg16::TMP);
			ctx.stack_push16(Reg16::TMP);
			ctx.next_op();
		}
	};
	template <auto V>
	struct PushSP:Base{
		inline static const std::string name="Push SP";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,1};
		}
		static void gen(MCode& ctx){
			LOG("PushSP");
			ctx.copy16(Reg16::SP,Reg16::TMP);
			ctx.stack_push16(Reg16::TMP);
			ctx.next_op();
		}
	};
	template <auto V>
	struct PopSP:Base{
		inline static const std::string name="Pop SP";
		using id    = OPID<8,Base,V>;
		static layout_t parse(MCode ctx){
			return {name,1};
		}
		static void gen(MCode& ctx){
			LOG("PopSP");
			ctx.stack_pop16(Reg16::TMP);
			ctx.copy16(Reg16::TMP,Reg16::SP);
			ctx.next_op();
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
			ctx.set_minus_one16(Reg16::SP);//Reg16::SP=0xFFFF
			ctx.set_zero16(Reg16::PC);//Reg16::PC=0x0000
			ctx.load_op();//load op from MEM[0]
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
			MCode ctx(marg, -1, 0);
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
			MCode ctx(MARG::opcode::set(0,op), -1, 0);
			layout_t result{"Unknown",1};
			( parse_if<Ops>(ctx,result) || ... );
			return result;
		}
	};
	
	namespace Ops{
		using Halt       = Halt       <0b11111111>;
		using ImmVal     = ImmVal     <0b11111110>;
		using BranchCF   = BranchCF   <0b11111101>;
		using BranchZero = BranchZero <0b11111100>;
		using Load       = Load       <0b111110>;
		using Save       = Save       <0b111101>;
		using Logic      = Logic      <0b111100>;
		using Calc       = Calc       <0b11101>;
		using Push       = Push       <0b11100>;
		using Pop        = Pop        <0b11011>;
		using Jump       = Jump       <0b11010111>;
		using Call       = Call       <0b11010110>;
		using Return     = Return     <0b11010101>;

		using Adjust     = Adjust     <0b11010100>;
		using PushSP     = PushSP     <0b11010011>;
		using PopSP      = PopSP      <0b11010010>;
		/*using Enter    = Enter      <0b11001100>;
		using Adjust     = Adjust     <0b11001011>;
		using Leave      = Leave      <0b11001010>;
		using Local      = Local      <0b11001001>;*/

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
				Adjust,PushSP,PopSP,
				Halt,
				INT0,INT1,INT2,INT3,
				INT4,INT5,INT6,INT7,
				Initialize
		>;
	}

	inline void generateOPROM(const std::string& name,size_t size=Util::Bitwise::BitSize<MCTRL::type>,size_t i=0){
		std::ofstream fout(name);
		if(!fout) {return;}
		Ops::all::imax=0;
		fout<<ROM(TruthTable<MARG::type,MCTRL::type,MARG::size>(std::function{[=](MARG::type marg){
			//std::cout<<std::bitset<MARG::size>(marg);
			auto mctrl=Ops::all::gen(marg);
			//std::cout<<"=>"<<std::bitset<MCTRL::size>(mctrl)<<std::endl;
			return (mctrl>>(i*size)) % (1<<size);
		}}));
		std::cout<<"imax="<<Ops::all::imax<<std::endl;
	}

};
#endif //BBCPU_CPU_OPCODE_H
