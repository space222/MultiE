#include "EEngine.h"

typedef std::function<void(EEngine&, u32)> EEInstr;
#define INSTR return [](EEngine& cpu, u32 opc)
#define INSTR_NARG return [](EEngine&, u32)

#define SHIFT ((opc>>6)&0x1F)
#define D ((opc>>11)&0x1F)
#define T ((opc>>16)&0x1F)
#define S ((opc>>21)&0x1F)
#define RD cpu.r[D].d
#define RT cpu.r[T].d
#define RS cpu.r[S].d

#define SIMM16 ((s16)(opc&0xffff))
#define UIMM16 ((u16)(opc&0xffff))
#define BASE (RS+SIMM16)
#define BTARGET (cpu.npc+((SIMM16<<2)))
#define JTARGET ((cpu.npc&0xf0000000u)|((opc&0x03ffFFFFu)<<2))
#define LIKELY cpu.npc = cpu.nnpc; cpu.nnpc += 4

static EEInstr decode_COP2(u32 opcode)
{
	if( !(opcode & BIT(25)) ) switch( (opcode>>21) & 0x1F )
	{
	case 0x01: INSTR_NARG { std::println("QMFC2"); }; // QMFC2
	case 0x02: INSTR_NARG { std::println("CFC2"); }; // CFC2
	case 0x05: INSTR_NARG { std::println("QMTC2"); }; // QMTC2
	case 0x06: INSTR_NARG { std::println("CTC2"); }; // CTC2
	
	default:
		std::println("EE: Unimpl cop2 opc = ${:X}", (opcode>>21)&0x1F);
		return nullptr;
	}
	std::println("EE: Unimpl SIMD opc");
	INSTR_NARG{};	
}

static EEInstr decode_COP1(u32 opcode)
{
	if( !(opcode & BIT(25)) ) switch( (opcode>>21) & 0x1F )
	{
	case 0x02: INSTR_NARG { std::println("CFC1"); }; //todo: CFC1	
	case 0x04: INSTR { cpu.fpr[D] = std::bit_cast<float>((u32)RT); }; // MTC1
	
	case 0x06: INSTR_NARG { std::println("CTC1"); }; //todo: CTC1
	default:
		std::println("EE: Unimpl cop1 opc = ${:X}", (opcode>>21)&0x1F);
		return nullptr;
	}
	
	if( ((opcode>>21)&0x1F) == 0x10 ) switch( opcode & 0x3F )
	{
	case 0x18: INSTR_NARG { std::println("ADDA.S"); }; //todo: ADDA.S
	case 0x1C: INSTR_NARG { std::println("MADD.S"); }; //todo: MADD.S
	default:
		std::println("EE: Unipml FPU.S, bot6=${:X}", opcode&0x3F);
		return nullptr;
	}
	std::println("EE: Unimpl FPU opc");
	return nullptr;
}


static EEInstr decode_COP0(u32 opcode)
{
	if( ((opcode>>21)&0x1F) == 0 )
	{
		INSTR { RT = (s32)cpu.read_cop0(D); }; // MFC0
	}
	if( ((opcode>>21)&0x1F) == 4 )
	{
		INSTR { cpu.write_cop0(D, RT); }; // MTC0	
	}
	if( ((opcode>>21)&0x1F) == 0x10 )
	{
		u32 sub = opcode&0x3F;
		switch( sub )
		{
		case 2: INSTR_NARG {}; // TLBWI
		
		case 0x18:return [](EEngine& cpu,u32){ cpu.npc = cpu.cop0[14]; cpu.nnpc=cpu.npc+4; cpu.cop0[12]&=~BIT(1); }; //ERET
		case 0x38:return [](EEngine& cpu,u32){ cpu.cop0[12]|=BIT(16); };//EI
		case 0x39:return [](EEngine& cpu,u32){ cpu.cop0[12]&=~BIT(16);};//DI
		default:
			std::println("EE: Unimpl tlb opc, bot6=${:X}", sub);
			return nullptr;
		}
	}
	std::println("EE: Unimpl cop0 opc = ${:X}", ((opcode>>21)&0x1F));
	return nullptr;
}

static EEInstr decode_MMI0(u32 opcode)
{
	switch( (opcode>>6) & 0x1F )
	{
	case 0x12: INSTR 
		{ 
			auto d = cpu.r[D]; 
			d.w[0] = cpu.r[T].w[0];
			d.w[1] = cpu.r[S].w[0];
			d.w[2] = cpu.r[T].w[1];
			d.w[3] = cpu.r[S].w[1];
			cpu.r[D] = d;			
		}; // PEXTLW
	default:
		std::println("Unimpl MMI0, func=${:X}", (opcode>>6)&0x1F);
		return nullptr;
	}
}

static EEInstr decode_MMI1(u32 opcode)
{
	switch( (opcode>>6) & 0x1F )
	{
	case 0x10: INSTR { for(u32 i=0; i<4; ++i) { cpu.r[D].w[i] = std::min<u64>(cpu.r[S].w[i]+cpu.r[T].w[i], 0xffffFFFFu); }}; //PADDUW
	default:
		std::println("Unimpl MMI1, func=${:X}", (opcode>>6)&0x1F);
		return nullptr;
	}
}

static EEInstr decode_MMI2(u32 opcode)
{
	switch( (opcode>>6) & 0x1F )
	{
	case 0x08: INSTR { cpu.r[D].q = (u128(cpu.hi1)<<64)|cpu.hi; }; // PMFHI
	case 0x09: INSTR { cpu.r[D].q = (u128(cpu.lo1)<<64)|cpu.lo; }; // PMFLO
	
	case 0x0E: INSTR { cpu.r[D].q = (u128(cpu.r[S].ud[0])<<64)|RT; }; //PCPYLD
	default:
		std::println("Unimpl MMI2, func=${:X}", (opcode>>6)&0x1F);
		return nullptr;
	}
}

static EEInstr decode_MMI3(u32 opcode)
{
	switch( (opcode>>6) & 0x1F )
	{
	case 0x0E: INSTR { cpu.r[D].q = (u128(cpu.r[T].ud[1])<<64) | u128(cpu.r[S].ud[1]); }; // PCPYUD
	
	case 0x12: INSTR { cpu.r[D].q = cpu.r[T].q | cpu.r[S].q; }; // POR
	default:
		std::println("Unimpl MMI3, func=${:X}", (opcode>>6)&0x1F);
		return nullptr;
	}
}

static EEInstr decode_MMI(u32 opcode)
{
	switch( opcode&0x3F )
	{
	case 0x04: INSTR { 
			u32 a = cpu.r[S].w[0];
			u32 b = cpu.r[S].w[1];
			if( a & BIT(31) )
			{
				a = std::countl_one(a)-1;
			} else {
				a = std::countl_zero(a)-1;
			}
			if( b & BIT(31) )
			{
				b = std::countl_one(b)-1;
			} else {
				b = std::countl_zero(b)-1;
			}
			cpu.r[D].w[0] = a;
			cpu.r[D].w[1] = b;	
		}; // PLZCW
	
	case 0x08: return decode_MMI0(opcode);
	case 0x09: return decode_MMI2(opcode);

	case 0x10: INSTR { RD = cpu.hi1; }; // MFHI1
	case 0x11: INSTR { cpu.hi1 = RS; }; // MTHI1
	case 0x12: INSTR { RD = cpu.lo1; }; // MFLO1
	case 0x13: INSTR { cpu.lo1 = RS; }; // MTLO1

	case 0x18: INSTR { s64 a =s32(RT); a*=s32(RS); cpu.hi1=s32(a>>32); RD=cpu.lo1=s32(a); }; // MULT1
	case 0x1B: INSTR {
			if( RT == 0 )
			{
			
			} else {
				cpu.lo1 = s32( u32(RS)/u32(RT) );
				cpu.hi1 = s32( u32(RS)%u32(RT) );
			}
		}; // DIVU1
		
	case 0x28: return decode_MMI1(opcode);
	case 0x29: return decode_MMI3(opcode);
	default:
		std::println("EE: Undef MMI opc, bot6=${:X}", opcode&0x3F);
		return nullptr;
	}
}

static EEInstr decode_special(u32 opcode)
{
	switch( opcode & 0x3F )
	{
	case 0: INSTR { RD = s32(RT << SHIFT); }; // SLL
	case 2: INSTR { RD = s32(u32(RT) >> SHIFT); }; // SRL
	case 3: INSTR { RD = s32(RT) >> SHIFT; }; // SRA
	case 4: INSTR { RD = s32(RT << (RS&0x1F)); }; // SLLV
	case 6: INSTR { RD = s32(u32(RT) >> (RS&0x1F)); }; // SRLV
	case 7: INSTR { RD = s32(RT) >> (RS&0x1F); }; // SRAV
	case 8: INSTR { cpu.branch(RS); }; // JR
	case 9: INSTR { u32 t=RS; RD=cpu.nnpc; cpu.branch(t); }; // JALR
	case 0xA: INSTR { if(RT==0) { RD=RS; } }; //MOVZ
	case 0xB: INSTR { if(RT!=0) { RD=RS; } }; //MOVN
	case 0xC: return [](EEngine& cpu, u32) { cpu.exception(8); }; //SYSCALL
	// break
	case 0xF: INSTR_NARG {}; // SYNC
	
	case 0x10: INSTR { RD = cpu.hi; }; // MFHI
	case 0x11: INSTR { cpu.hi = RS; }; // MTHI
	case 0x12: INSTR { RD = cpu.lo; }; // MFLO
	case 0x13: INSTR { cpu.lo = RS; }; // MTLO
	case 0x14: INSTR { RD = RT << (RS&0x3F); }; // DSLLV
	case 0x16: INSTR { RD = RT >> (RS&0x3F); }; // DSRLV
	case 0x17: INSTR { RD = s64(RT) >> (RS&0x3F); }; // DSRAV
	
	case 0x18: INSTR { s64 a = s32(RT); a*=s32(RS); cpu.lo = s32(a); RD=cpu.lo; cpu.hi=s32(a>>32); }; // MULT
	case 0x19: INSTR { u64 a = u32(RT); a*=u32(RS); cpu.lo = s32(a); RD=cpu.lo; cpu.hi=s32(a>>32); }; // MULTU
	case 0x1A: INSTR { 
			if( RT == 0 )
			{
			
			} else {
				cpu.lo = s32(RS)/s32(RT);
				cpu.hi = s32(RS)%s32(RT);
			}
		}; // DIV
	case 0x1B: INSTR { 
			if( RT == 0 )
			{
			
			} else {
				cpu.lo = s32( u32(RS)/u32(RT) );
				cpu.hi = s32( u32(RS)%u32(RT) );
			}
		}; // DIVU
	
	case 0x20: INSTR { RD = s32(RS) + s32(RT); }; // ADD
	case 0x21: INSTR { RD = s32(RS) + s32(RT); }; // ADDU
	case 0x22: INSTR { RD = s32(RS) - s32(RT); }; // SUB
	case 0x23: INSTR { RD = s32(RS) - s32(RT); }; // SUBU
	case 0x24: INSTR { RD = RS & RT; }; // AND
	case 0x25: INSTR { RD = RS | RT; }; // OR
	case 0x26: INSTR { RD = RS ^ RT; }; // XOR
	case 0x27: INSTR { RD = ~(RS | RT); }; // NOR
	
	case 0x28: INSTR { RD = cpu.sa; }; // MFSA
	case 0x29: INSTR { cpu.sa = RS; }; // MTSA
	case 0x2A: INSTR { RD = s64(RS) < s64(RT); }; // SLT
	case 0x2B: INSTR { RD = RS < RT; }; // SLTU
	case 0x2C: INSTR { RD = RS + RT; }; // DADD
	case 0x2D: INSTR { RD = RS + RT; }; // DADDU
	case 0x2E: INSTR { RD = RS - RT; }; // DSUB
	case 0x2F: INSTR { RD = RS - RT; }; // DSUBU
	//case 0x30-37 traps
	
	case 0x38: INSTR { RD = RT << SHIFT; }; // DSLL
	case 0x3A: INSTR { RD = RT >> SHIFT; }; // DSRL
	case 0x3B: INSTR { RD = s64(RT) >> SHIFT; }; // DSRA
	case 0x3C: INSTR { RD = RT << (SHIFT+32); }; // DSLL32
	case 0x3E: INSTR { RD = RT >> (SHIFT+32); }; // DSRL32
	case 0x3F: INSTR { RD = s64(RT) >> (SHIFT+32); }; // DSRA32
	default:
		std::println("EE: Undef special opc, bot6=${:X}", opcode&0x3F);
		return nullptr;
	}
}

static EEInstr decode_regimm(u32 opcode)
{
	switch( (opcode>>16)&0x1F )
	{
	case 0: INSTR { cpu.ndelay=true; if( s64(RS) < 0 ) { cpu.branch(BTARGET); } }; // BLTZ
	case 1: INSTR { cpu.ndelay=true; if( s64(RS) >=0 ) { cpu.branch(BTARGET); } }; // BGEZ
	case 2: INSTR { if( s64(RS) < 0 ) { cpu.branch(BTARGET); } else { LIKELY; } }; // BLTZL
	case 3: INSTR { if( s64(RS) >=0 ) { cpu.branch(BTARGET); } else { LIKELY; } }; // BGEZL
	default:
		std::println("EE: Undef regimm opc, T=${:X}", (opcode>>16)&0x1F);
		return nullptr;
	}
}

// masks and these instructions are adapted from PCSX2
static const u64 LDL_MASK[8] =
{	0x00ffffffffffffffULL, 0x0000ffffffffffffULL, 0x000000ffffffffffULL, 0x00000000ffffffffULL,
	0x0000000000ffffffULL, 0x000000000000ffffULL, 0x00000000000000ffULL, 0x0000000000000000ULL
};
static const u64 LDR_MASK[8] =
{	0x0000000000000000ULL, 0xff00000000000000ULL, 0xffff000000000000ULL, 0xffffff0000000000ULL,
	0xffffffff00000000ULL, 0xffffffffff000000ULL, 0xffffffffffff0000ULL, 0xffffffffffffff00ULL
};
static const u64 SDL_MASK[8] =
{	0xffffffffffffff00ULL, 0xffffffffffff0000ULL, 0xffffffffff000000ULL, 0xffffffff00000000ULL,
	0xffffff0000000000ULL, 0xffff000000000000ULL, 0xff00000000000000ULL, 0x0000000000000000ULL
};
static const u64 SDR_MASK[8] =
{	0x0000000000000000ULL, 0x00000000000000ffULL, 0x000000000000ffffULL, 0x0000000000ffffffULL,
	0x00000000ffffffffULL, 0x000000ffffffffffULL, 0x0000ffffffffffffULL, 0x00ffffffffffffffULL
};

static EEInstr decode(u32 opcode)
{
	switch( opcode >> 26 )
	{
	case 0: return decode_special(opcode);
	case 1: return decode_regimm(opcode);
	case 2: INSTR { cpu.branch(JTARGET); }; // J
	case 3: INSTR { cpu.r[31].d = (s32)cpu.nnpc; cpu.branch(JTARGET); }; // JAL
	case 4: INSTR { cpu.ndelay=true; if( RS == RT ) { cpu.branch(BTARGET); } };//BEQ
	case 5: INSTR { cpu.ndelay=true; if( RS != RT ) { cpu.branch(BTARGET); } };//BEQ
	case 6: INSTR { cpu.ndelay=true; if( s64(RS) <= 0 ){ cpu.branch(BTARGET);}};//BLEZ
	case 7: INSTR { cpu.ndelay=true; if( s64(RS)>0 ){ cpu.branch(BTARGET);}};//BGTZ
	case 8: INSTR { RT=s32(RS)+SIMM16; }; // ADDI
	case 9: INSTR { RT=s32(RS)+SIMM16; }; // ADDIU
	case 0xA: INSTR { RT=s64(RS) < SIMM16;}; // SLTI
	case 0xB: INSTR { RT= RS < (u64)(s64)SIMM16;}; // SLTIU
	case 0xC: INSTR { RT = RS & UIMM16; }; // ANDI
	case 0xD: INSTR { RT = RS | UIMM16; }; // ORI
	case 0xE: INSTR { RT = RS ^ UIMM16; }; // XORI
	case 0xF: INSTR { RT = SIMM16<<16; }; // LUI
	case 0x10: return decode_COP0(opcode);
	case 0x11: return decode_COP1(opcode);
	case 0x12: return decode_COP2(opcode);
	case 0x14: INSTR { if(RT==RS) { cpu.branch(BTARGET); } else { LIKELY; }}; // BEQL
	case 0x15: INSTR { if(RT!=RS) { cpu.branch(BTARGET); } else { LIKELY; }}; // BNEL
	case 0x16: INSTR { if( s64(RS) <= 0 ){ cpu.branch(BTARGET);} else { LIKELY; }};//BLEZL
	case 0x17: INSTR { if( s64(RS)>0 ){ cpu.branch(BTARGET);} else { LIKELY; }};//BGTZL

	case 0x18: INSTR { RT = RS + SIMM16; }; // DADDI
	case 0x19: INSTR { RT = RS + SIMM16; }; // DADDIU
	case 0x1A: INSTR {
		u32 addr = BASE;
		u32 shift = addr & 7;

		u64 mem = cpu.read(addr & ~7, 64);

		RT = (RT & LDL_MASK[shift]) | (mem << ((shift^7)<<3));
		}; // LDL
	case 0x1B: INSTR {
		u32 addr = BASE;
		u32 shift = addr & 7;

		u64 mem = cpu.read(addr & ~7, 64);

		RT = (RT & LDR_MASK[shift]) | (mem >> (shift<<3));
		}; // LDR
	case 0x1C: return decode_MMI(opcode);
	//no 1D
	case 0x1E: INSTR { cpu.r[T].q = cpu.read(BASE, 128); }; // LQ
	case 0x1F: INSTR { cpu.write(BASE, cpu.r[T].q, 128); }; // SQ
	
	case 0x20: INSTR { RT = (s8)cpu.read(BASE, 8); }; // LB
	case 0x21: INSTR { RT = (s16)cpu.read(BASE, 16); }; // LH
	//case 0x22 LWL
	case 0x23: INSTR { RT = (s32)cpu.read(BASE, 32); }; // LW
	case 0x24: INSTR { RT = (u8)cpu.read(BASE, 8); }; // LBU
	case 0x25: INSTR { RT = (u16)cpu.read(BASE, 16); }; // LHU
	//case 0x26: LWR
	case 0x27: INSTR { RT = (u32)cpu.read(BASE, 32); }; // LWU

	case 0x28: INSTR { cpu.write(BASE, RT, 8); }; // SB
	case 0x29: INSTR { cpu.write(BASE, RT, 16); }; // SH
	//case 0x2A SWL
	case 0x2B: INSTR { cpu.write(BASE, RT, 32); }; // SW
	case 0x2C: INSTR {
		u32 addr = BASE;
		u32 shift = addr & 7;
		u64 mem = cpu.read(addr & ~7, 64);
		mem = (RT >> ((shift^7)<<3)) | (mem & SDL_MASK[shift]);
		cpu.write(addr & ~7, mem, 64);
		}; // SDL
	case 0x2D: INSTR {
		u32 addr = BASE;
		u32 shift = addr & 7;
		u64 mem = cpu.read(addr & ~7, 64);
		mem = (RT << (shift<<3)) | (mem & SDR_MASK[shift]);
		cpu.write(addr & ~7, mem, 64);
		}; // SDR
	//case 0x2E SWR
	case 0x2F: INSTR_NARG {}; // CACHE
	
	case 0x31: INSTR { cpu.fpr[T] = std::bit_cast<float>((u32)cpu.read(BASE, 32)); };//LWC1
	case 0x33: INSTR_NARG {}; // PREF
	case 0x37: INSTR { RT = cpu.read(BASE, 64); }; // LD
	
	case 0x39: INSTR { cpu.write(BASE, std::bit_cast<u32>(cpu.fpr[T]), 32); }; // SWC1
	
	case 0x3F: INSTR { cpu.write(BASE, RT, 64); }; // SD
	default:
		std::println("EE: undef opc, top6 = ${:X}", opcode>>26);
		return nullptr;
	}
}

extern bool logall;

void EEngine::step()
{
	r[0].q = 0;
	stamp += 1;
	cop0[9] += 2;
	
	delay = ndelay;
	ndelay = false;

	if( !delay && (cop0[12]&BIT(0)) && (cop0[12]&BIT(16)) && !(cop0[12]&BIT(1)) && !(cop0[12]&BIT(2))
		&& (cop0[12] & cop0[13] & (BIT(10)|BIT(11)|BIT(15))) )
	{
		std::println("EE IRQ");
		exception(0);
	} else {
		u32 opc = read(pc, 32);
		if( logall && pc >= 0x83B40u && pc < 0x85000u ) std::println("{:X}: {:08X}", pc, opc);
	
		auto I = decode(opc);
		if( !I ) exit(1);
		
		I(*this, opc);
	}
		
	pc = npc;
	npc = nnpc;
	nnpc += 4;
}

void EEngine::exception(u32 code)
{
	cop0[13] &=~0b1111100;
	cop0[13] |= code<<2;
	
	if( delay )
	{
		cop0[14] = pc-4;
		cop0[13] |= BIT(31);
	} else {
		cop0[14] = pc;
		cop0[13] &=~BIT(31);
	}
	cop0[12] |= BIT(1);
	
	if( code == 0 )
	{
		npc = 0x80000200u;
	} else {
		npc = 0x80000180u;
	}
	nnpc = npc + 4;
}

