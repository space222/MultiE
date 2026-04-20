#include <print>
#include "arm946e.h"

#define SBIT (opc&BIT(20))
#define setNZ(a) cpu.cpsr.b.Z = ((a)==0?1:0); cpu.cpsr.b.N = ((a)>>31)&1

static auto undef_instr = [](arm& cpu, u32 opc) { std::println("${:X}: unimpl opc ${:X}", cpu.r[15]-(cpu.cpsr.b.T?4:8), opc); exit(1); };

void arm7_mul(arm& cpu, u32 opc);
void arm7_single_swap(arm& cpu, u32 opc);
void arm7_xfer_half(arm& cpu, u32 opc);
void arm7_xfer_signed(arm& cpu, u32 opc);
void arm7_mrs(arm& cpu, u32 opc);
void arm7_msr_reg(arm& cpu, u32 opc);
void arm7_msr_imm(arm& cpu, u32 opc);
u32 dataproc_op2(arm& cpu, u32 opc);
void arm7_and(arm& cpu, u32 opc);
void arm7_eor(arm& cpu, u32 opc);
void arm7_sub(arm& cpu, u32 opc);
void arm7_rsb(arm& cpu, u32 opc);
void arm7_add(arm& cpu, u32 opc);
void arm7_adc(arm& cpu, u32 opc);
void arm7_sbc(arm& cpu, u32 opc);
void arm7_rsc(arm& cpu, u32 opc);
void arm7_tst(arm& cpu, u32 opc);
void arm7_teq(arm& cpu, u32 opc);
void arm7_cmp(arm& cpu, u32 opc);
void arm7_cmn(arm& cpu, u32 opc);
void arm7_orr(arm& cpu, u32 opc);
void arm7_mov(arm& cpu, u32 opc);
void arm7_bic(arm& cpu, u32 opc);
void arm7_mvn(arm& cpu, u32 opc);
u32 arm7_ldst_shift(arm& cpu, u32 opc);
void arm7_ldst_reg(arm& cpu, u32 opc);
void arm7_ldst_imm(arm& cpu, u32 opc);
void arm7_ldst_m(arm& cpu, u32 opc);
void arm7_bx(arm&, u32);
void arm7_mul_long(arm&, u32);

void arm9_stcldc(arm& cpu, u32 opc)
{
	
}

void arm9_cdp(arm& cpu, u32 opc)
{
	
}

void arm9_mcr(arm& cpu, u32 opc)
{
	u32 cp_op = (opc>>21)&7;
	u32 Cn = (opc>>16)&15;
	u32 Rd = (opc>>12)&15;
	u32 Pn = (opc>>8)&15;
	u32 CP = (opc>>5)&7;
	u32 Cm = opc&15;
	if( Pn != 15 ) return; // ??
	
	if( opc & BIT(20) )
	{ // cop to arm
		if( cp_op == 0 && Cn == 0 && Cm == 0 && CP == 1 )
		{
			cpu.r[Rd] = 0x0F0D2112;
		} else {
			std::println("copread P{}, cp_op{}, Cn{}, Cm{}, CP{}, Rd(${:X})", Pn, cp_op, Cn, Cm, CP, cpu.r[Rd]);
		}
	} else {
		std::println("copwrite P{}, cp_op{}, Cn{}, Cm{}, CP{}, Rd(${:X})", Pn, cp_op, Cn, Cm, CP, cpu.r[Rd]);	
	}
}

void arm9_blx(arm& cpu, u32 opc)
{
	u32 retval = cpu.r[15] - 4;
	
	//u32 L = cpu.r[15] - 8;
	cpu.r[15] = cpu.r[opc&15];
	//std::println("${:X}: BLX to ${:X}", L, cpu.r[15]);
	cpu.cpsr.b.T = cpu.r[15]&1;
	cpu.r[15] &= ((cpu.r[15]&1)?~1:~3);
	cpu.flushp();
	 
	cpu.r[14] = retval;
}

void arm9_clz(arm& cpu, u32 opc)
{
	const u32 Rd = (opc>>12)&15;
	const u32 Rm = (opc&15);
	cpu.r[Rd] = std::countl_zero(cpu.r[Rm]);
}

void arm9_qadd(arm& cpu, u32 opc)
{
	s32 Rn = cpu.r[(opc>>16)&15];
	s32 Rm = cpu.r[opc&15];
	u32& Rd = cpu.r[(opc>>12)&15];
	
	if( opc & BIT(22) ) { Rn *= 2; }
	
	s64 res = Rm;
	if( opc & BIT(21) )
	{
		res -= Rn;
	} else {
		res += Rn;
	}
	
	if( res > 0x7FFFffffLL  )
	{
		res = 0x7FFFffff;
		cpu.cpsr.b.Q = 1;
	} else if( res < -0x80000000LL ) {
		res = 0x80000000u;
		cpu.cpsr.b.Q = 1;
	}
	
	Rd = res;
}


arm7_instr arm946e::decode_arm(u32 opcode)
{
	//todo: with v5+ some opcodes can no longer be decoded with only the 12bits used later on
	//	or can they?
	


	u32 cc = ((opcode>>16)&0xff0) | ((opcode>>4)&15);
	if( (cc&0xfcf) == 9 ) { return arm7_mul; }
	if( (cc&0xf8f) == 0x89 ) { return arm7_mul_long; }
	//todo: fit the half-word muls here
	
	
	if( (cc&0xfbf) == 0x109) { return arm7_single_swap; }

	if( (cc&0xe0f) == 0xB ) { return arm7_xfer_half; }
	if( (cc&0xe1d) == 0x1D ){ return arm7_xfer_signed; }
	
	if( (cc&0xfbf) == 0x100 ) { return arm7_mrs; }
	if( (cc&0xfbf) == 0x120 ) { return arm7_msr_reg; }
	if( (cc&0xfb0) == 0x320 ) { return arm7_msr_imm; }
	if( (cc&0xfff) == 0x121 ) { return arm7_bx; }
	
	// 0001 0110 0001  CLZ
	if( (cc&0xfff) == 0x161 ) { return arm9_clz; }
	// 0001 0010 0011  BLX (register)
	if( (cc&0xfff) == 0x123 ) { return arm9_blx; }
	// 0001 0..0 0101 QADD,etc
	if( (cc&0xf9f) == 0x105 ) { return arm9_qadd; }
	
	if( (cc&0xfb0) == 0x300 ) { return undef_instr; } // the undef in the middle of dataproc
	if( (cc&0xe00) == 0x000 || (cc&0xe00) == 0x200 )
	{  // data proccessing
		switch( (cc>>5) & 15 )
		{
		case 0: return arm7_and;
		case 1: return arm7_eor;
		case 2: return arm7_sub;
		case 3: return arm7_rsb;
		case 4: return arm7_add;
		case 5: return arm7_adc;
		case 6: return arm7_sbc;
		case 7: return arm7_rsc;
		case 8: return arm7_tst;
		case 9: return arm7_teq;
		case 10: return arm7_cmp;
		case 11: return arm7_cmn;
		case 12: return arm7_orr;
		case 13: return arm7_mov;
		case 14: return arm7_bic;
		default: break;
		}
		return arm7_mvn;	
	}
	
	if( (cc&0xe00) == 0x400 ) return arm7_ldst_imm;
	if( (cc&0xe00) == 0x600 ) return arm7_ldst_reg;
	if( (cc&0xe00) == 0x800 ) return arm7_ldst_m;

	if( (cc&0xe00) == 0xa00 )
	{  // B / BL
		return [](arm& cpu, u32 opc) {
			if( opc & BIT(24) )
			{
				cpu.r[14] = cpu.r[15] - 4;
			}
			s32 offs = s32(opc<<8)>>8;
			cpu.r[15] += offs*4;
			cpu.flushp();
		};
	}
	
	if( (cc&0xe00) == 0xc00 ) { return arm9_stcldc; }
	if( (cc&0xf01) == 0xe00 ) { return arm9_cdp; }
	if( (cc&0xf01) == 0xe01 ) { return arm9_mcr; }

	if( (cc&0xf00) == 0xf00 )
	{   // SWI
		return [](arm& cpu, u32) {
			cpu.spsr_svc = cpu.cpsr.v;
			cpu.switch_to_mode(ARM_MODE_SUPER);
			cpu.cpsr.b.M = ARM_MODE_SUPER;
			cpu.cpsr.b.I = 1;
			cpu.r[14] = cpu.r[15]-4;
			cpu.r[15] = 8;
			cpu.flushp();
		};
	}

	std::println("Reached end of arm946e::decode_arm()");
	return undef_instr;
}

void arm946e::flushp() 
{
	if( cpsr.b.T )
	{
		r[15] &= ~1;
		decode = inst_fetch(r[15], 16, ARM_CYCLE::N);
		r[15] += 2;
		fetch = inst_fetch(r[15], 16, ARM_CYCLE::S);	
	} else {
		r[15] &= ~3;
		decode = inst_fetch(r[15], 32, ARM_CYCLE::N);
		r[15] += 4;
		fetch = inst_fetch(r[15], 32, ARM_CYCLE::S);
	}
}

void arm946e::step()
{
	if( irq_line && cpsr.b.I==0 )
	{
		spsr_irq = cpsr.v;
		switch_to_mode(ARM_MODE_IRQ);
		cpsr.b.M = ARM_MODE_IRQ;
		cpsr.b.I = 1;
		r[14] = r[15] + (cpsr.b.T?2:0);
		cpsr.b.T = 0;
		r[15] = 0x18;
		flushp();
	}

	execute = decode;
	decode = fetch;
	r[15] += (cpsr.b.T ? 2 : 4);
	fetch = inst_fetch(r[15]&(cpsr.b.T?~1:~3), (cpsr.b.T ? 16 : 32), ARM_CYCLE::X);
	u32 opc = execute;
	
	//std::println("${:X}:{:X}: opc = ${:X}", r[15] - (cpsr.b.T?4:8), u32(cpsr.b.T), opc);
	
	if( r[15] >= 0x10000000u )
	{
		std::println("${:X} too big, halting", (r[15]&(cpsr.b.T?~1:~3)) - (cpsr.b.T?4:8));
		exit(1);
	}
	
	if( cpsr.b.T )
	{
		decode_thumb(opc)(*this, opc);
	} else {
		if( (opc>>28) == 0xF )
		{
			std::println("condF! opc = ${:X}", opc);
			if( ((opc>>25)&7) == 5 )
			{
				u32 retval = r[15] - 4;
				s32 offs = s32(opc<<8)>>8;
				u32 L = r[15] - 8;
				r[15] += offs*4 + ((opc&BIT(24)) ? 2:0);
				std::println("${:X}: BLX imm to ${:X}", L, r[15]);
				cpsr.b.T = 1;
				flushp();
		 		r[14] = retval;
	 		}
		} else if( isCond(opc>>28) ) {
 			decode_arm(opc)(*this, opc);
		}
	}
	stamp+=1;
}

void arm946e::reset()
{
	//todo: copied from my old emu, need to double check
	//std::println("arm7di::reset()");
	cpsr.b.M = ARM_MODE_USER;
	r[13] = 0x03007F00;
	r13_svc = 0x03007FE0; 
	r13_irq =  0x03007FA0;
	switch_to_mode(ARM_MODE_SUPER);
	spsr_svc = 0x9f;
	r[15] = 0xffff0000u;
	cpsr.b.T = 0;
	cpsr.b.I = 0;
	stamp = 0;
	flushp();
	irq_line = false;
}

bool arm946e::isCond(u8 cc)
{
	switch(cc)
	{
	case 0: return cpsr.b.Z==1;
	case 1: return cpsr.b.Z==0;
	case 2: return cpsr.b.C==1;
	case 3: return cpsr.b.C==0;
	case 4: return cpsr.b.N==1;
	case 5: return cpsr.b.N==0;
	case 6: return cpsr.b.V==1;
	case 7: return cpsr.b.V==0;
	case 8: return cpsr.b.C==1 && cpsr.b.Z==0;
	case 9: return cpsr.b.C==0 || cpsr.b.Z==1;
	case 10: return cpsr.b.N==cpsr.b.V;
	case 11: return cpsr.b.N!=cpsr.b.V;
	case 12: return cpsr.b.Z==0 && cpsr.b.N==cpsr.b.V;
	case 13: return cpsr.b.Z==1 || cpsr.b.N!=cpsr.b.V; 
	case 14: return true;
	default: break;
	}
	return true; 
}

void arm946e::switch_to_mode(u32 mode) 
{
	mode &= 0x1f;
	mode |= 0x10; // ?? check
	switch( cpsr.b.M )
	{
	case ARM_MODE_IRQ:
		std::swap(r[14], r14_irq);
		std::swap(r[13], r13_irq);
		break;
	case ARM_MODE_ABORT:
		std::swap(r[14], r14_abt);
		std::swap(r[13], r13_abt);
		break;
	case ARM_MODE_SUPER:
		std::swap(r[14], r14_svc);
		std::swap(r[13], r13_svc);
		break;
	case ARM_MODE_UNDEF:
		std::swap(r[14], r14_und);
		std::swap(r[13], r13_und);
		break;
	case ARM_MODE_FIQ:
		for(u32 i = 8; i < 15; ++i) std::swap(fiq[i], r[i]);
		break;
	default: break;
	}
	
	switch( mode )
	{
	case ARM_MODE_IRQ:
		std::swap(r[14], r14_irq);
		std::swap(r[13], r13_irq);
		break;
	case ARM_MODE_ABORT:
		std::swap(r[14], r14_abt);
		std::swap(r[13], r13_abt);
		break;
	case ARM_MODE_SUPER:
		std::swap(r[14], r14_svc);
		std::swap(r[13], r13_svc);
		break;
	case ARM_MODE_UNDEF:
		std::swap(r[14], r14_und);
		std::swap(r[13], r13_und);
		break;
	case ARM_MODE_FIQ:
		for(u32 i = 8; i < 15; ++i) std::swap(fiq[i], r[i]);
		break;
	default: break;
	}
	
	cpsr.b.M = mode;
}

void arm946e::swi() { }

void thumb1_lsl(arm&, u32);
void thumb1_lsr(arm&, u32);
void thumb1_asr(arm&, u32);
void thumb2_addsub(arm&, u32);
void thumb3_mov(arm&, u32);
void thumb3_cmp(arm&, u32);
void thumb3_add(arm&, u32);
void thumb3_sub(arm&, u32);
void thumb4_and(arm&, u32);
void thumb4_eor(arm&, u32);
void thumb4_lsl(arm&, u32);
void thumb4_lsr(arm&, u32);
void thumb4_asr(arm&, u32);
void thumb4_adc(arm&, u32);
void thumb4_sbc(arm&, u32);
void thumb4_ror(arm&, u32);
void thumb4_tst(arm&, u32);
void thumb4_neg(arm&, u32);
void thumb4_cmp(arm&, u32);
void thumb4_cmn(arm&, u32);
void thumb4_orr(arm&, u32);
void thumb4_mul(arm&, u32);
void thumb4_bic(arm&, u32);
void thumb4_mvn(arm&, u32);
void thumb5_add(arm&, u32);
void thumb5_cmp(arm&, u32);
void thumb5_mov(arm&, u32);
void thumb5_bx(arm&, u32);
void thumb6_ldrpc(arm&, u32);
void thumb7_str(arm&, u32);
void thumb7_strb(arm&, u32);
void thumb7_ldr(arm&, u32);
void thumb7_ldrb(arm&, u32);
void thumb8_strh(arm&, u32);
void thumb8_ldsb(arm&, u32);
void thumb8_ldsh(arm&, u32);
void thumb8_ldrh(arm&, u32);
void thumb9_str(arm&, u32);
void thumb9_ldr(arm&, u32);
void thumb9_strb(arm&, u32);
void thumb9_ldrb(arm&, u32);
void thumb10_ldrh(arm&, u32);
void thumb10_strh(arm&, u32);
void thumb11_ldr(arm&, u32);
void thumb11_str(arm&, u32);
void thumb12_addsppc(arm&, u32);
void thumb13_addspnn(arm&, u32);
void thumb14_push(arm&, u32);
void thumb14_pop(arm&, u32);
void thumb15_stmia(arm&, u32);
void thumb15_ldmia(arm&, u32);
void thumb16_bcc(arm&, u32);
void thumb17_swi(arm&, u32);
void thumb18_b(arm&, u32);

void arm9_thumb19_bl(arm& cpu, u32 opc)
{
	if( ((opc>>11)&3) == 0b10 )
	{
		cpu.r[14] = cpu.r[15] + ((opc&0x7ff)<<12) + ((opc&0x400)?0xff800000u:0);
	} else {
		u32 temp = (cpu.r[15]-2)|1;
		cpu.r[15] = cpu.r[14] + ((opc&0x7ff)<<1);
		//std::println("thumb bl to ${:X}", cpu.r[15]);
		cpu.r[15] |= 1;
		cpu.r[14] = temp;
		if( (opc&BIT(12)) == 0 ) { cpu.r[15] &= ~3; cpu.cpsr.b.T = 0; }
		cpu.flushp();
	}
}

arm7_instr arm946e::decode_thumb(u16 opc)
{
	//std::println("${:X}: thumbopc ${:X}", r[15]-4, opc);
	switch( opc>>13 )
	{
	case 0:
		switch( opc>>11 )
		{
		case 0: return thumb1_lsl;
		case 1: return thumb1_lsr;
		case 2: return thumb1_asr;
		default: break;
		}
		return thumb2_addsub;
	case 1:
		switch( (opc>>11)&3 )
		{
		case 0: return thumb3_mov;
		case 1: return thumb3_cmp;
		case 2: return thumb3_add;
		default: break;
		}	
		return thumb3_sub;
	case 2:
		if( ((opc>>10)&7) == 0 )
		{
			switch((opc>>6)&15)
			{
			case 0: return thumb4_and;
			case 1: return thumb4_eor;
			case 2: return thumb4_lsl;
			case 3: return thumb4_lsr;
			case 4: return thumb4_asr;
			case 5: return thumb4_adc;
			case 6: return thumb4_sbc;
			case 7: return thumb4_ror;
			case 8: return thumb4_tst;
			case 9: return thumb4_neg;
			case 0xA: return thumb4_cmp;
			case 0xB: return thumb4_cmn;
			case 0xC: return thumb4_orr;
			case 0xD: return thumb4_mul;
			case 0xE: return thumb4_bic;
			default: break;
			}
			return thumb4_mvn;
		}
		if( ((opc>>10)&7) == 1 )
		{
			switch( (opc>>8)&3 )
			{
			case 0: return thumb5_add;
			case 1: return thumb5_cmp;
			case 2: return thumb5_mov;
			default: break;
			}
			return thumb5_bx;	
		}
		if( ((opc>>11)&3) == 1 ) return thumb6_ldrpc;
		if( opc & BIT(9) )
		{
			switch( (opc>>10) & 3 )
			{
			case 0: return thumb8_strh;
			case 1: return thumb8_ldsb;
			case 2: return thumb8_ldrh;
			default: break;
			}
			return thumb8_ldsh;
		}
		switch( (opc>>10)&3 )
		{
		case 0: return thumb7_str;
		case 1: return thumb7_strb;
		case 2: return thumb7_ldr;		
		default: break;
		}
		return thumb7_ldrb;
	case 3:
		switch( (opc>>11) & 3 )
		{
		case 0: return thumb9_str;
		case 1: return thumb9_ldr;
		case 2: return thumb9_strb;
		default: break;
		}
		return thumb9_ldrb;
	case 4:
		if( opc & BIT(12) )
		{
			return (opc&BIT(11)) ? thumb11_ldr : thumb11_str;
		}
		return (opc&BIT(11)) ? thumb10_ldrh : thumb10_strh;		
	case 5:
		if( !(opc&BIT(12)) ) return thumb12_addsppc;
		if( !((opc>>8)&15) ) return thumb13_addspnn;
		if( opc & BIT(11) ) return thumb14_pop;
		return thumb14_push;
	case 6: 
		if( opc & BIT(12) ) 
		{
			if( ((opc>>8)&15) == 15 ) return thumb17_swi;
			return thumb16_bcc;
		}
		return (opc&BIT(11)) ? thumb15_ldmia : thumb15_stmia;
	case 7:
		if( ((opc>>11)&3) == 0 )
		{
			return thumb18_b;
		}
		return arm9_thumb19_bl;
	default: break;
	}
	std::println("thumb_decode error");
	exit(1);
	return nullptr;
}





