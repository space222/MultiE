#include <cstdio>
#include <cstdlib>
#include <bit>
#include <algorithm>
#include "arm7tdmi.h"

void arm::switch_to_mode(u32 m)
{
	if( m == cpsr.b.M ) return;
	if( (m == ARM_MODE_USER || m == ARM_MODE_SYSTEM) && (cpsr.b.M == ARM_MODE_USER || cpsr.b.M == ARM_MODE_SYSTEM) )
	{
		cpsr.b.M = m;
		return;
	}
	if( cpsr.b.M != ARM_MODE_USER && cpsr.b.M != ARM_MODE_SYSTEM )
	{
		switch( cpsr.b.M )
		{
		case ARM_MODE_IRQ:
			std::swap(r13_irq, r[13]);
			std::swap(r14_irq, r[14]);
			break;
		case ARM_MODE_ABORT:
			std::swap(r13_abt, r[13]);
			std::swap(r14_abt, r[14]);
			break;
		case ARM_MODE_SUPER:
			std::swap(r13_svc, r[13]);
			std::swap(r14_svc, r[14]);
			break;
		case ARM_MODE_UNDEF:
			std::swap(r13_und, r[13]);
			std::swap(r14_und, r[14]);
			break;
		case ARM_MODE_FIQ:
			std::swap(fiq[8], r[8]);
			std::swap(fiq[9], r[9]);
			std::swap(fiq[10], r[10]);
			std::swap(fiq[11], r[11]);
			std::swap(fiq[12], r[12]);
			std::swap(fiq[13], r[13]);
			std::swap(fiq[14], r[14]);	
			break;
		}	
	}
	if( m != ARM_MODE_USER && m != ARM_MODE_SYSTEM )
	{
		switch( m )
		{
		case ARM_MODE_IRQ:
			std::swap(r13_irq, r[13]);
			std::swap(r14_irq, r[14]);
			break;
		case ARM_MODE_ABORT:
			std::swap(r13_abt, r[13]);
			std::swap(r14_abt, r[14]);
			break;
		case ARM_MODE_SUPER:
			std::swap(r13_svc, r[13]);
			std::swap(r14_svc, r[14]);
			break;
		case ARM_MODE_UNDEF:
			std::swap(r13_und, r[13]);
			std::swap(r14_und, r[14]);
			break;
		case ARM_MODE_FIQ:
			std::swap(fiq[8], r[8]);
			std::swap(fiq[9], r[9]);
			std::swap(fiq[10], r[10]);
			std::swap(fiq[11], r[11]);
			std::swap(fiq[12], r[12]);
			std::swap(fiq[13], r[13]);
			std::swap(fiq[14], r[14]);	
			break;
		}	
	}
}

void arm::swi()
{
	switch_to_mode(ARM_MODE_SUPER);
	r[14] = r[15] - (cpsr.b.T ? 2 : 4);
	spsr_svc = cpsr.v;
	r[15] = 8;
	cpsr.b.T = 0;
	cpsr.b.I = 1;
	flushp();
}

void thumb_1_lsl(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u64 Rs = cpu.r[(opc>>3)&7];
	u32 shift = (opc>>6) & 0x1f;
	Rs <<= shift;
	cpu.cpsr.b.C = (Rs>>32);
	Rd = Rs;
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	return;
}

void thumb_1_lsr(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	u32 shift = (opc>>6) & 0x1f;
	if( shift )
	{	
		Rs >>= shift - 1;
		cpu.cpsr.b.C = Rs&1;
		Rs >>= 1;
	}
	Rd = Rs;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_1_asr(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	u32 shift = (opc>>6) & 0x1f;
	if( shift )
	{	
		Rs = s32(Rs) >> (shift - 1);
		cpu.cpsr.b.C = Rs&1;
		Rs = s32(Rs) >> 1;
	}
	Rd = Rs;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_2_add(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	u32 Rn = ((opc&BIT(10)) ? ((opc>>6)&7) : cpu.r[((opc>>6)&7)]);
	u64 res = Rs;
	res += Rn;
	Rd = res;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.Z = (Rd==0)?1:0;
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	cpu.cpsr.b.V = ((Rd^Rs)&(Rd^Rn)&BIT(31))?1:0;	
	return;
}

void thumb_2_sub(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	u32 Rn = ~((opc&BIT(10)) ? ((opc>>6)&7) : cpu.r[((opc>>6)&7)]);
	u64 res = Rs;
	res += Rn;
	res += 1;
	Rd = res;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.Z = (Rd==0)?1:0;
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	cpu.cpsr.b.V = ((Rd^Rs)&(Rd^Rn)&BIT(31))?1:0;	
	return;
}

void thumb_3_mov(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[(opc>>8)&7];
	Rd = opc&0xff;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = 0;
	return;
}

void thumb_3_cmp(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[(opc>>8)&7];
	u32 s = ~(opc&0xff);
	u64 res = Rd;
	res += s;
	res += 1;
	cpu.cpsr.b.V = (((res^s)&(res^Rd)&BIT(31))?1:0);
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_3_add(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[(opc>>8)&7];
	u32 s = (opc&0xff);
	u64 res = Rd;
	res += s;
	res += 1;
	cpu.cpsr.b.V = (((res^s)&(res^Rd)&BIT(31))?1:0);
	cpu.cpsr.b.C = res>>32;
	Rd = res;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
}

void thumb_3_sub(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[(opc>>8)&7];
	u32 s = ~(opc&0xff);
	u64 res = Rd;
	res += s;
	res += 1;
	cpu.cpsr.b.V = (((res^s)&(res^Rd)&BIT(31))?1:0);
	cpu.cpsr.b.C = res>>32;
	Rd = res;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_mvn(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd = ~Rs;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_bic(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd &= ~Rs;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_eor(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd ^= Rs;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_orr(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd |= Rs;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_tst(arm& cpu, u32 opc)
{
	u32 Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd &= Rs;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_ror(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd = std::rotr(Rd, Rs);
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.C = cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);	
	return;
}

void thumb_4_cmp(arm& cpu, u32 opc)
{
	u32 Rd = cpu.r[opc&7];
	u32 Rs = ~cpu.r[(opc>>3)&7];
	u64 res = Rd;
	res += Rs;
	res += 1;
	cpu.cpsr.b.C = (res>>32);
	cpu.cpsr.b.V = (((res^Rd)&(res^Rs)&BIT(31))?1:0);
	cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
	cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	return;
}

void thumb_4_neg(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd = -Rs;	
	//todo: better neg flags
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_cmn(arm& cpu, u32 opc)
{
	u32 Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	u64 res = Rd;
	res += Rs;
	cpu.cpsr.b.C = (res>>32);
	cpu.cpsr.b.V = (((res^Rd)&(res^Rs)&BIT(31))?1:0);
	cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
	cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	return;
}

void thumb_4_mul(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd *= Rs;
	//todo: mul flags
	return;
}

void thumb_4_adc(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	u64 res = Rd;
	res += Rs;
	cpu.cpsr.b.C = (res>>32);
	cpu.cpsr.b.V = (((res^Rd)&(res^Rs)&BIT(31))?1:0);
	cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
	cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	Rd = res;
	return;
}

void thumb_4_sbc(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = ~cpu.r[(opc>>3)&7];
	u64 res = Rd;
	res += Rs;
	res += 1;
	cpu.cpsr.b.C = (res>>32);
	cpu.cpsr.b.V = (((res^Rd)&(res^Rs)&BIT(31))?1:0);
	cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
	cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	Rd = res;
	return;
}

void thumb_4_lsr(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 shift = cpu.r[(opc>>3)&7] & 0x1f;
	if( shift ) cpu.cpsr.b.C = (Rd>>(shift-1))&1;	
	Rd >>= shift;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = 0;
	return;
}

void thumb_4_asr(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 shift = cpu.r[(opc>>3)&7] & 0x1f;
	if( shift ) cpu.cpsr.b.C = (Rd>>(shift-1))&1;	
	Rd = s32(Rd) >> shift;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_lsl(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 shift = cpu.r[(opc>>3)&7] & 0x1f;
	if( shift )
	{
		cpu.cpsr.b.C = (u64(Rd)<<shift)>>32;	
	}
	Rd <<= shift;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_4_and(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd &= Rs;
	cpu.cpsr.b.Z = ((Rd==0)?1:0);
	cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);
	return;
}

void thumb_5_hireg_add(arm& cpu, u32 opc)
{
	u32 dstreg = (opc&7)|((opc>>4)&8);
	u32& Rd = cpu.r[(opc&7)|((opc>>4)&8)];
	u32& Rs = cpu.r[((opc>>3)&15)];
	Rd += Rs;
	if( dstreg == 15 ) cpu.flushp();
	return;
}

void thumb_5_hireg_cmp(arm& cpu, u32 opc)
{
	u32 d = cpu.r[(opc&7)|((opc>>4)&8)];
	u32 s = cpu.r[((opc>>3)&15)];
	u64 res = d;
	res += ~s;
	res += 1;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.Z = (u32(res)==0)?1:0;
	cpu.cpsr.b.N = ((res&BIT(31))?1:0);
	cpu.cpsr.b.V = ((res^d)&(res^s)&BIT(31))?1:0;
	return;
}

void thumb_5_hireg_mov(arm& cpu, u32 opc)
{
	u32 dstreg = (opc&7)|((opc>>4)&8);
	u32& Rd = cpu.r[(opc&7)|((opc>>4)&8)];
	u32& Rs = cpu.r[((opc>>3)&15)];
	Rd = Rs;
	if( dstreg == 15 ) cpu.flushp();
	return;
}

void thumb_5_hireg_bx(arm& cpu, u32 opc)
{
	u32& Rs = cpu.r[((opc>>3)&15)];
	cpu.r[15] = Rs&~1;
	cpu.cpsr.b.T = Rs&1;
	cpu.flushp();
	return;
}

void thumb_6_pcrel_ld(arm& cpu, u32 opc)
{
	cpu.r[(opc>>8)&7] = cpu.read((cpu.r[15]&~BIT(1)) + ((opc&0xff)<<2), 32, ARM_CYCLE::N);
	return;
}

void thumb_7_ldst_regoffs(arm& cpu, u32 opc)
{
	u32 addr = cpu.r[(opc>>6)&7] + cpu.r[(opc>>3)&7];
	if( opc & BIT(11) )
	{
		cpu.r[opc&7] = cpu.read(addr, (opc&BIT(10))?8:32, ARM_CYCLE::N);
	} else {
		cpu.write(addr, cpu.r[opc&7], (opc&BIT(10))?8:32, ARM_CYCLE::N);
		cpu.next_cycle_type = ARM_CYCLE::N;
	}
	return;
}

void thumb_8_ldst_sextbh(arm& cpu, u32 opc)
{
	u32 addr = cpu.r[(opc>>6)&7] + cpu.r[(opc>>3)&7];
	switch( (opc>>10) & 3 )
	{
	case 0:
		cpu.write(addr, cpu.r[opc&7], 16, ARM_CYCLE::N);
		cpu.next_cycle_type = ARM_CYCLE::N;
		break;
	case 1:
		cpu.r[opc&7] = (s32)(s8)cpu.read(addr, 8, ARM_CYCLE::N);
		break;
	case 2:
		cpu.r[opc&7] = (u16)cpu.read(addr, 16, ARM_CYCLE::N);
		break;
	case 3:
		cpu.r[opc&7] = (s32)(s16)cpu.read(addr, 16, ARM_CYCLE::N);
		break;	
	}
	return;
}

void thumb_9_ldst_immoffs(arm& cpu, u32 opc)
{
	u32 offs = (opc>>6)&0x1f;
	if( !(opc & BIT(12)) ) offs <<= 2;
	if( opc & BIT(11) )
	{
		cpu.r[opc&7] = cpu.read(cpu.r[(opc>>3)&7]+offs, (opc&BIT(12))?8:32, ARM_CYCLE::N);
	} else {
		cpu.write(cpu.r[(opc>>3)&7]+offs, cpu.r[opc&7], (opc&BIT(12))?8:32, ARM_CYCLE::N);
		cpu.next_cycle_type = ARM_CYCLE::N;
	}
	return;
}

void thumb_10_ldst_half(arm& cpu, u32 opc)
{
	u32& dst = cpu.r[opc&7];
	u32& base = cpu.r[(opc>>3)&7];
	u32 addr = (((opc>>6)&0x1f)<<1) + base;
	if( opc & BIT(11) )
	{
		dst = cpu.read(addr, 16, ARM_CYCLE::N);
	} else {
		cpu.write(addr, dst, 16, ARM_CYCLE::N);
		cpu.next_cycle_type = ARM_CYCLE::N;
	}
	return;
}

void thumb_11_sprel_ldst(arm& cpu, u32 opc)
{
	if( opc & BIT(11) )
	{
		cpu.r[(opc>>8)&7] = cpu.read(cpu.r[13] + ((opc&0xff)<<2), 32, ARM_CYCLE::N);
	} else {
		cpu.write(cpu.r[13] + ((opc&0xff)<<2), cpu.r[(opc>>8)&7], 32, ARM_CYCLE::N);
		cpu.next_cycle_type = ARM_CYCLE::N;
	}
	return;
}

void thumb_14_pushpop(arm& cpu, u32 opc)
{
	ARM_CYCLE ct = ARM_CYCLE::N;
	if( opc & BIT(11) )
	{
		for(u32 i = 0; i < 8; ++i)
		{
			if( opc & (1<<i) )
			{
				cpu.r[i] = cpu.read(cpu.r[13], 32, ct);
				cpu.r[13] += 4;			
				ct = ARM_CYCLE::S;
			}	
		}
		if( opc & BIT(8) )
		{
			cpu.r[15] = cpu.read(cpu.r[13], 32, ct);
			cpu.r[13] += 4;
			cpu.flushp();
		}
	} else {
		for(u32 i = 0; i < 8; ++i)
		{
			if( opc & (1<<i) )
			{
				cpu.r[13] -= 4;			
				cpu.write(cpu.r[13], cpu.r[i], 32, ct);
				ct = ARM_CYCLE::S;
			}	
		}
		if( opc & BIT(8) )
		{
			cpu.r[13] -= 4;
			cpu.write(cpu.r[13], cpu.r[14], 32, ct);
		}	
	}
	return;
}

void thumb_13_add_sp(arm& cpu, u32 opc)
{
	if( opc & BIT(7) )
	{
		cpu.r[13] -= (opc & 0x7f) << 2;
	} else {
		cpu.r[13] += (opc & 0x7f) << 2;
	}
	return;
}

void thumb_12_ld_addr(arm& cpu, u32 opc)
{
	u32 val = (opc&0xff)<<2;
	if( opc & BIT(11) )
	{
		val += cpu.r[13];
	} else {
		val += cpu.r[15]&~BIT(1);
	}
	cpu.r[(opc>>8)&7] = val;
	return;
}

void thumb_17_swi(arm& cpu, u32 opc)
{
	cpu.swi();
	return;
}

void thumb_16_cond_branch(arm& cpu, u32 opc)
{
	if( cpu.isCond((opc>>8)&0xf) )
	{
		cpu.r[15] += s8(opc);
		cpu.flushp();
	}
	return;
}

void thumb_15_multi_ldst(arm& cpu, u32 opc)
{
	ARM_CYCLE ct = ARM_CYCLE::N;
	if( opc & BIT(11) )
	{
		u32 base = cpu.r[(opc>>8)&7];
		cpu.r[(opc>>8)&7] += std::popcount(opc&0xff)*4;
		for(u32 i = 0; i < 8; ++i)
		{
			if( opc & BIT(i) )
			{
				cpu.r[i] = cpu.read(base, 32, ct);
				ct = ARM_CYCLE::S;			
			}		
		}
	} else {
		u32 base = cpu.r[(opc>>8)&7];
		cpu.r[(opc>>8)&7] += std::popcount(opc&0xff)*4;
		for(u32 i = 0; i < 8; ++i)
		{
			if( opc & BIT(i) )
			{
				cpu.write(base, cpu.r[i], 32, ct);
				ct = ARM_CYCLE::S;			
			}		
		}	
	}
	return;
}

void thumb_18_uncond_branch(arm& cpu, u32 opc)
{
	cpu.r[15] += (s16(opc<<5)>>4);
	cpu.flushp();
	return;
}

void thumb_19_long_branch(arm& cpu, u32 opc)
{
	if( opc & BIT(11) )
	{
		cpu.r[14] += (opc & 0x7ff)<<1;
		u32 temp = cpu.r[15] - 2;
		cpu.r[15] = cpu.r[14];
		cpu.r[14] = temp;
		cpu.flushp();
		printf("thumb long branch to $%X\n", cpu.r[15]);
	} else {
		cpu.r[14] = cpu.r[15] + ((opc&0x7ff) << 12);	
	}
	return;
}

arm7_instr arm7tdmi::decode_thumb(u16 opc)
{
	switch( opc >> 13 )
	{
	case 0:
		switch( (opc>>11) & 3 )
		{
		case 0: return thumb_1_lsl;
		case 1: return thumb_1_lsr;
		case 2: return thumb_1_asr;		
		case 3: return (opc&BIT(9)) ? thumb_2_sub : thumb_2_add;
		}
	case 1:
		switch( (opc>>11) & 3 )
		{
		case 0: return thumb_3_mov;
		case 1: return thumb_3_cmp;
		case 2: return thumb_3_add;
		case 3:	return thumb_3_sub;
		}
	case 2: 
		if( opc & BIT(12) )
		{
			if( opc & BIT(9) ) return thumb_8_ldst_sextbh;
			return thumb_7_ldst_regoffs;
		}
		if( opc & BIT(11) ) return thumb_6_pcrel_ld;
		if( opc & BIT(10) )
		{
			switch( (opc>>8) & 3 )
			{
			case 0: return thumb_5_hireg_add;
			case 1: return thumb_5_hireg_cmp;
			case 2: return thumb_5_hireg_mov;
			case 3: return thumb_5_hireg_bx;			
			}		
		}
		switch( ((opc>>6) & 0xf) )
		{
		case 0: return thumb_4_and;
		case 1: return thumb_4_eor;
		case 2: return thumb_4_lsl;
		case 3: return thumb_4_lsr;
		case 4: return thumb_4_asr;
		case 5: return thumb_4_adc;
		case 6: return thumb_4_sbc;
		case 7: return thumb_4_ror;
		case 8: return thumb_4_tst;
		case 9: return thumb_4_neg;
		case 10: return thumb_4_cmp;
		case 11: return thumb_4_cmn;
		case 12: return thumb_4_orr;
		case 13: return thumb_4_mul;
		case 14: return thumb_4_bic;
		case 15: return thumb_4_mvn;
		}	
	case 3: return thumb_9_ldst_immoffs;	
	case 4:
		if( opc & BIT(12) ) return thumb_11_sprel_ldst;
		return thumb_10_ldst_half;
	case 5:
		if( opc & BIT(12) )
		{
			if( ((opc>>8) & 0xf) == 0 ) return thumb_13_add_sp;
			return thumb_14_pushpop;
		}
		return thumb_12_ld_addr;
	case 6:
		if( opc & BIT(12) )
		{
			if( ((opc>>8) & 0xf) == 0xf ) return thumb_17_swi;
			return thumb_16_cond_branch;
		}
		return thumb_15_multi_ldst;
	case 7:
		if( opc & BIT(12) )
		{
			return thumb_19_long_branch;
		}
		return thumb_18_uncond_branch;
	}
	printf("decode_thumb failed to decode\n");
	exit(1);
	return nullptr;
}

u64 arm7tdmi::step()
{
	icycles = 0;
	
	//todo: check for IRQs here	
	
	exec = decode;
	decode = fetch;
	r[15] += (cpsr.b.T ? 2 : 4);
	
	arm7_instr instr = nullptr;
	if( cpsr.b.T )
	{
		instr = thumb_funcs[exec>>6];
	} else {
		instr = decode_arm(exec);
	}
	
	next_cycle_type = ARM_CYCLE::S;
	flushed = false;

	if( cpsr.b.T || isCond(exec>>28) ) if( instr ) instr(*this, exec);
	if( !flushed ) fetch = read(r[15], cpsr.b.T ? 16 : 32, next_cycle_type);
	
	printf("ARM7:%X: opc = $%X\n", r[15], exec);
	
	return icycles;
}

void arm7tdmi::flushp()
{
	decode = read(r[15], cpsr.b.T ? 16 : 32, ARM_CYCLE::N);
	r[15] += (cpsr.b.T ? 2 : 4);
	fetch = read(r[15], cpsr.b.T ? 16 : 32, ARM_CYCLE::S);
	flushed = true;
	return;
}

bool arm7tdmi::isCond(u8 cc)
{
	switch( cc & 0xf )
	{
	case 0x0: return cpsr.b.Z;
	case 0x1: return !cpsr.b.Z;
	case 0x2: return cpsr.b.C;
	case 0x3: return !cpsr.b.C;
	case 0x4: return cpsr.b.N;
	case 0x5: return !cpsr.b.N;
	case 0x6: return cpsr.b.V;
	case 0x7: return !cpsr.b.V;
	case 0x8: return cpsr.b.C && !cpsr.b.Z;
	case 0x9: return cpsr.b.Z || !cpsr.b.C;
	case 0xA: return cpsr.b.N == cpsr.b.V;
	case 0xB: return cpsr.b.N != cpsr.b.V;
	case 0xC: return !cpsr.b.Z && (cpsr.b.V == cpsr.b.N);
	case 0xD: return cpsr.b.Z || (cpsr.b.V != cpsr.b.N);	
	case 0xE: return true;
	case 0xF: printf("Unimpl isCond case $%X\n", cc); exit(1);
	}
	printf("arm7::isCond, shouldn't get here\n");
	exit(1);
	return true;
}

arm7tdmi::arm7tdmi()
{
	for(u32 i = 0; i < 0x400; ++i)
	{
		thumb_funcs[i] = decode_thumb(i<<6);
	}
	for(u32 i = 0; i < 0x1000; ++i)
	{
		u32 opc = (i&0xf)<<4;
		opc |= (i>>4)<<20;
		//todo: build arm table
	}
}



