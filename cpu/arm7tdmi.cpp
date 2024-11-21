#include <cstdio>
#include <cstdlib>
#include <bit>
#include <algorithm>
#include "arm7tdmi.h"

// this file is step, reset, and thumb instructions.
// see arm7tdmi_arm for arm instructions

inline void setnz(arm& cpu, u32 v)
{
	cpu.cpsr.b.Z = ((v==0)?1:0);
	cpu.cpsr.b.N = ((v&BIT(31))?1:0);
}

void thumb_1_lsl(arm& cpu, u32 opc)
{
	u32 amt = (opc>>6)&0x1f;
	u64 Rs = cpu.r[(opc>>3)&7];
	if( amt )
	{
		Rs <<= amt;
		cpu.cpsr.b.C = (Rs>>32);
	}
	u32& Rd = cpu.r[opc&7];
	Rd = Rs;
	setnz(cpu, Rd);
}

void thumb_1_lsr(arm& cpu, u32 opc)
{
	u32 amt = (opc>>6)&0x1f;
	u32 Rs = cpu.r[(opc>>3)&7];
	if( amt )
	{
		cpu.cpsr.b.C = (Rs>>(amt-1))&1;
		Rs >>= amt;
	} else {
		cpu.cpsr.b.C = 0;
		Rs = 0;
	}
	u32& Rd = cpu.r[opc&7];
	Rd = Rs;
	setnz(cpu, Rd);
}

void thumb_1_asr(arm& cpu, u32 opc)
{
	u32 amt = (opc>>6)&0x1f;
	u32 Rs = cpu.r[(opc>>3)&7];
	if( amt )
	{
		cpu.cpsr.b.C = (Rs>>(amt-1))&1;
		Rs = s32(Rs) >> 1;
	} else {
		cpu.cpsr.b.C = Rs>>31;
		Rs = (cpu.cpsr.b.C ? 0xffffFFFFu : 0);
	}
	u32& Rd = cpu.r[opc&7];
	Rd = Rs;
	setnz(cpu, Rd);
}

void thumb_2_add(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	u32 Rn = ( (opc&BIT(10)) ? ((opc>>6)&7) : cpu.r[(opc>>6)&7] );
	u64 res = Rs;
	res += Rn;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rs)&BIT(31))?1:0);
	Rd = res;
	setnz(cpu,Rd);
}

void thumb_2_sub(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	u32 Rn = ~( (opc&BIT(10)) ? ((opc>>6)&7) : cpu.r[(opc>>6)&7] );
	u64 res = Rs;
	res += Rn;
	res += 1;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rs)&BIT(31))?1:0);
	Rd = res;
	setnz(cpu,Rd);
}

void thumb_3_cmp(arm& cpu, u32 opc)
{
	u32 Rd = cpu.r[(opc>>8)&7];
	u32 Rn = ~( opc&0xff );
	u64 res = Rd;
	res += Rn;
	res += 1;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rd)&BIT(31))?1:0);
	setnz(cpu,u32(res));
}

void thumb_3_mov(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[(opc>>8)&7];
	u32 Rn = ( opc&0xff );
	Rd = Rn;
	setnz(cpu,Rd);
}

void thumb_3_add(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[(opc>>8)&7];
	u32 Rn = ( opc&0xff );
	u64 res = Rd;
	res += Rn;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rd)&BIT(31))?1:0);
	Rd = res;
	setnz(cpu,Rd);
}

void thumb_3_sub(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[(opc>>8)&7];
	u32 Rn = ~( opc&0xff );
	u64 res = Rd;
	res += Rn;
	res += 1;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rd)&BIT(31))?1:0);
	Rd = res;
	printf("thumb3 sub r%i, $%X. r%i = $%X\n", (opc>>8)&7, opc&0xff, (opc>>8)&7, Rd);
	setnz(cpu,Rd);
}

void thumb_4_and(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd &= Rs;
	setnz(cpu,Rd);	
}

void thumb_4_eor(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd ^= Rs;
	setnz(cpu,Rd);	
}

void thumb_4_lsl(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 amt = cpu.r[(opc>>3)&7] & 0xff;
	if( amt )
	{
		u64 v = Rd;
		v <<= amt;
		cpu.cpsr.b.C = v>>32;
		Rd = v;	
	}
	setnz(cpu, Rd);
}

void thumb_4_lsr(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 amt = cpu.r[(opc>>3)&7] & 0xff;
	if( amt )
	{
		Rd >>= amt-1;
		cpu.cpsr.b.C = Rd&1;
		Rd >>= 1;	
	}
	setnz(cpu, Rd);
}

void thumb_4_asr(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 amt = cpu.r[(opc>>3)&7] & 0xff;
	if( amt )
	{
		if( amt > 31 )
		{
			Rd = ((Rd&BIT(31))? 0xffffFFFFu : 0);
			cpu.cpsr.b.C = Rd&1;
		} else {
			Rd = s32(Rd) >> (amt-1);
			cpu.cpsr.b.C = Rd&1;
			Rd = s32(Rd) >> 1;
		}
	}
	setnz(cpu, Rd);
}

void thumb_4_adc(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rn = ( cpu.r[(opc>>3)&7] );
	u64 res = Rd;
	res += Rn;
	res += cpu.cpsr.b.C;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rd)&BIT(31))?1:0);
	Rd = res;
	setnz(cpu,Rd);
}

void thumb_4_sbc(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rn = ~( cpu.r[(opc>>3)&7] );
	u64 res = Rd;
	res += Rn;
	res += cpu.cpsr.b.C;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rd)&BIT(31))?1:0);
	Rd = res;
	setnz(cpu,Rd);
}

void thumb_4_ror(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 amt = cpu.r[(opc>>3)&7] & 0xff;
	if( amt )
	{
		Rd = std::rotr(Rd, amt);
		cpu.cpsr.b.C = Rd>>31;	
	}
	setnz(cpu, Rd);
}

void thumb_4_tst(arm& cpu, u32 opc)
{
	u32 Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd &= Rs;
	setnz(cpu,Rd);	
}

void thumb_4_neg(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rn = ~( cpu.r[(opc>>3)&7] );
	u64 res = 0;
	res += Rn;
	res += 1;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res)&BIT(31))?1:0);
	Rd = res;
	setnz(cpu,Rd);
}

void thumb_4_mvn(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd = ~Rs;
	setnz(cpu,Rd);	
}

void thumb_4_mul(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd *= Rs;
	setnz(cpu,Rd);
}

void thumb_4_bic(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd &= ~Rs;
	setnz(cpu,Rd);	
}

void thumb_4_cmp(arm& cpu, u32 opc)
{
	u32 Rd = cpu.r[opc&7];
	u32 Rn = ~cpu.r[(opc>>3)&7];
	u64 res = Rd;
	res += Rn;
	res += 1;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rd)&BIT(31))?1:0);
	setnz(cpu,u32(res));
}

void thumb_4_cmn(arm& cpu, u32 opc)
{
	u32 Rd = cpu.r[opc&7];
	u32 Rn = cpu.r[(opc>>3)&7];
	u64 res = Rd;
	res += Rn;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^Rn)&(res^Rd)&BIT(31))?1:0);
	setnz(cpu,u32(res));
}

void thumb_4_orr(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 Rs = cpu.r[(opc>>3)&7];
	Rd |= Rs;
	setnz(cpu,Rd);	
}

void thumb_5_hireg_add(arm& cpu, u32 opc)
{
	u32 d = (opc&7)|((opc>>4)&8);
	u32 s = (opc>>3)&15;
	cpu.r[d] += cpu.r[s];
	if( d == 15 ) cpu.flushp();
}

void thumb_5_hireg_cmp(arm& cpu, u32 opc)
{
	u32 rd = cpu.r[(opc&7)|((opc>>4)&8)];
	u32 rs = ~cpu.r[(opc>>3)&15];
	u64 res = rd;
	res += rs;
	res += 1;
	cpu.cpsr.b.C = res>>32;
	cpu.cpsr.b.V = (((res^rd)&(res^rs)&BIT(31))?1:0);
	setnz(cpu, u32(res));
}

void thumb_5_hireg_mov(arm& cpu, u32 opc)
{
	u32 d = (opc&7)|((opc>>4)&8);
	u32 s = (opc>>3)&15;
	cpu.r[d] = cpu.r[s];
	setnz(cpu, cpu.r[d]);
	if( d == 15 ) cpu.flushp();
}

void thumb_5_hireg_bx(arm& cpu, u32 opc)
{
	u64 Rs = cpu.r[(opc>>3)&15];
	cpu.cpsr.b.T = Rs&1;
	cpu.r[15] = Rs&~(cpu.cpsr.b.T?1:3);
	cpu.flushp();
}

void thumb_6_pcrel_ld(arm& cpu, u32 opc)
{
	cpu.icycles += 1;
	u32 offset = (opc & 0xff)<<2;
	cpu.r[(opc>>8)&7] = cpu.read((cpu.r[15]&~2) + offset, 32, ARM_CYCLE::N);
}

void thumb_7_ldst_reg(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 addr = cpu.r[(opc>>3)&7] + cpu.r[(opc>>6)&7];
	switch( (opc>>10)&3 )
	{
	case 0:	cpu.write(addr, Rd, 32, ARM_CYCLE::N); break;
	case 1:	cpu.write(addr, Rd, 8, ARM_CYCLE::N); break;
	case 2: Rd = cpu.read(addr, 32, ARM_CYCLE::N); cpu.icycles+=1; break;
	case 3: Rd = cpu.read(addr, 8, ARM_CYCLE::N); cpu.icycles+=1; break;
	}
}

void thumb_8_ldst_se(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 addr = cpu.r[(opc>>3)&7] + cpu.r[(opc>>6)&7];
	switch( (opc>>10)&3 )
	{
	case 0:	cpu.write(addr, Rd, 16, ARM_CYCLE::N); break;
	case 1: Rd = (s32)(s8)cpu.read(addr, 8, ARM_CYCLE::N); cpu.icycles+=1; break;
	case 2: Rd = cpu.read(addr, 16, ARM_CYCLE::N); cpu.icycles+=1; break;
	case 3: Rd = (s32)(s16)cpu.read(addr, 16, ARM_CYCLE::N); cpu.icycles+=1; break;
	}
}

void thumb_9_ldst_word_imm(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 base = cpu.r[(opc>>3)&7];
	u32 offset = (opc>>6)&0x1f;
	if( opc & BIT(11) )
	{
		Rd = cpu.read(base + (offset<<2), 32, ARM_CYCLE::N); cpu.icycles += 1;
	} else {
		cpu.write(base + (offset<<2), Rd, 32, ARM_CYCLE::N);
	}
}

void thumb_9_ldst_byte_imm(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 base = cpu.r[(opc>>3)&7];
	u32 offset = (opc>>6)&0x1f;
	if( opc & BIT(11) )
	{
		Rd = cpu.read(base + offset, 8, ARM_CYCLE::N); cpu.icycles += 1;
	} else {
		cpu.write(base + offset, Rd, 8, ARM_CYCLE::N);
	}
}

void thumb_10_ldst_half(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[opc&7];
	u32 base = cpu.r[(opc>>3)&7];
	u32 offset = ((opc>>6)&0x1f)<<1;
	if( opc & BIT(11) )
	{
		Rd = cpu.read(base + offset, 16, ARM_CYCLE::N); cpu.icycles += 1;
	} else {
		cpu.write(base + offset, Rd, 16, ARM_CYCLE::N);
	}
}

void thumb_11_sprel_ldst(arm& cpu, u32 opc)
{
	u32& Rd = cpu.r[(opc>>8)&7];
	u32 offset = (opc&0xff)<<2;
	if( opc & BIT(11) )
	{
		Rd = cpu.read(cpu.r[13] + offset, 32, ARM_CYCLE::N); cpu.icycles += 1;
	} else {
		cpu.write(cpu.r[13] + offset, Rd, 32, ARM_CYCLE::N);
	}
}

void thumb_12_ldaddr(arm& cpu, u32 opc)
{
	u32 offset = (opc&0xff)<<2;
	u32& Rd = cpu.r[(opc>>8)&7];
	if( opc & BIT(11) )
	{
		Rd = cpu.r[13] + offset;
	} else {
		Rd = (cpu.r[15]&~2) + offset;
	}
}

void thumb_13_addsp(arm& cpu, u32 opc)
{
	u32 offset = (opc&0x7f)<<2;
	if( opc & BIT(7) )
	{
		cpu.r[13] = cpu.r[13] - offset;
	} else {
		cpu.r[13] = cpu.r[13] + offset;
	}
}

void thumb_14_pushpop(arm& cpu, u32 opc)
{
	ARM_CYCLE ct = ARM_CYCLE::N;
	u32 SP = cpu.r[13];
	if( opc & BIT(11) )
	{  // pop
		for(u32 i = 0; i < 8; ++i)
		{
			if( opc & BIT(i) )
			{
				cpu.r[i] = cpu.read(SP&~3, 32, ct);
				ct = ARM_CYCLE::S;
				SP += 4;
			}
		}
		if( opc & BIT(8) )
		{
			cpu.r[15] = cpu.read(SP&~3, 32, ct) & ~1;
			cpu.flushp();
			SP += 4;
		}
	} else { // push
		if( opc & BIT(8) )
		{
			SP -= 4;
			cpu.write(SP&~3, cpu.r[14], 32, ct);
			ct = ARM_CYCLE::S;
		}
		for(u32 i = 0; i < 8; ++i)
		{
			if( opc & BIT(7-i) )
			{
				SP -= 4;
				cpu.write(SP&~3, cpu.r[7-i], 32, ct);
			}		
		}
	}
	cpu.r[13] = SP;
}

void thumb_15_multi_ldst(arm& cpu, u32 opc)
{
	ARM_CYCLE ct = ARM_CYCLE::N;
	if( opc & BIT(11) )
	{ // load
		cpu.icycles += 1;
		u32 base = cpu.r[(opc>>8)&7];
		cpu.r[(opc>>8)&7] += 4*std::popcount(opc&0xff);
		for(u32 i = 0; i < 8; ++i)
		{
			if( opc & BIT(i) )
			{
				cpu.r[i] = cpu.read(base, 32, ct);
				printf("ldmia: r%i = $%X\n", i, cpu.r[i]);
				base += 4;
				ct = ARM_CYCLE::S;
			}
		}
	} else {
		u32& Rb = cpu.r[(opc>>8)&7];
		for(u32 i = 0; i < 8; ++i)
		{
			if( opc & BIT(i) )
			{
				cpu.write(Rb, cpu.r[i], 32, ct);
				Rb += 4;
				ct = ARM_CYCLE::S;
			}
		}
	}
}

void thumb_16_cond_branch(arm& cpu, u32 opc)
{
	if( cpu.isCond((opc>>8)&0xf) )
	{
		s32 offset = s8(opc&0xff);
		offset <<= 1;
		cpu.r[15] += offset;
		cpu.flushp();
	}
}

void thumb_17_swi(arm& cpu, u32)
{
	printf("Thumb swi\n");
	exit(1);
}

void thumb_18_uncond_branch(arm& cpu, u32 opc)
{
	s16 offset = opc<<5;
	offset >>= 4;
	cpu.r[15] += offset;
	cpu.flushp();
}

void thumb_19_long_branch(arm& cpu, u32 opc)
{
	if( opc & BIT(11) )
	{
		u32 temp = cpu.r[15] - 2;
		cpu.r[15] = cpu.r[14] + ((opc&0x7ff)<<1);
		printf("thumb long br to $%X\n", cpu.r[15]);
		if( cpu.r[15] < 0x0800'0000 ) exit(1);
		cpu.flushp();
		cpu.r[14] = temp|1;
	} else {
		u32 offs = (opc&0x7ff)<<12;
		if( offs & 0x400000 ) { offs |= 0xFF800000; }
		cpu.r[14] = cpu.r[15] + offs;
	}
}

arm7_instr arm7tdmi::decode_thumb(u16 opc)
{
	u32 top = (opc>>13);
	if( top == 0 )
	{
		switch( ((opc>>11)&3) )
		{
		case 0: return thumb_1_lsl;
		case 1: return thumb_1_lsr;
		case 2: return thumb_1_asr;
		default: return (opc&BIT(9)) ? thumb_2_sub : thumb_2_add;
		}
	}
	
	if( top == 1 )
	{
		switch( ((opc>>11)&3) )
		{
		case 0: return thumb_3_mov;
		case 1: return thumb_3_cmp;
		case 2: return thumb_3_add;
		default: return thumb_3_sub;
		}		
	}


	switch( opc >> 12 )
	{
	case 4: 
		if( opc&BIT(11) ) return thumb_6_pcrel_ld;
		if( opc&BIT(10) ) 
		{
			switch( (opc>>8)&3 )
			{
			case 0: return thumb_5_hireg_add;
			case 1: return thumb_5_hireg_cmp;
			case 2: return thumb_5_hireg_mov;
			default: return thumb_5_hireg_bx;
			}		
		}
		switch( (opc>>6) & 15 )
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
		case 10:return thumb_4_cmp;
		case 11:return thumb_4_cmn;
		case 12:return thumb_4_orr;
		case 13:return thumb_4_mul;
		case 14:return thumb_4_bic;		
		default: return thumb_4_mvn;
		}		
	case 5: return (opc&BIT(9)) ? thumb_8_ldst_se  : thumb_7_ldst_reg;
	case 6: return thumb_9_ldst_word_imm;
	case 7: return thumb_9_ldst_byte_imm;
	case 8: return thumb_10_ldst_half;
	case 9: return thumb_11_sprel_ldst;
	case 10: return thumb_12_ldaddr;
	case 11: return ( opc & BIT(10)  ) ? thumb_14_pushpop : thumb_13_addsp;
	case 12: return thumb_15_multi_ldst;
	case 13: return ( ((opc>>8)&15) == 15 ) ? thumb_17_swi : thumb_16_cond_branch;
	case 14: return thumb_18_uncond_branch;
	case 15: return thumb_19_long_branch;
	}
	return nullptr;
}

u64 arm7tdmi::step()
{
	icycles = 0;
	
	exec = decode;
	decode = fetch;
	r[15] += cpsr.b.T ? 2 : 4;
	
	//todo: check for IRQs here	
	arm7_instr instr = nullptr;
	if( cpsr.b.T )
	{
		printf("$%X: thumb opc = $%04X, r5 = $%X\n", r[15]-4, exec, r[5]);
		if( r[15]-4 < 0x0800'0000u ) exit(1);
		instr = decode_thumb(exec);
		if( ! instr )
		{
			printf("ARM7:$%X: thumb unimpl. opc = $%04X\n", r[15]-4, exec);
			exit(1);		
		}
		instr(*this, exec);
	} else {
		printf("arm7:$%X: about to decode $%X\n", r[15]-8, exec);
		instr = decode_arm(exec);
		if( ! instr )
		{
			printf("ARM7:$%X: arm unimpl. opc = $%08X\n", r[15]-8, exec);
			exit(1);		
		}
		if( isCond(exec>>28) ) instr(*this, exec);
	}
	
	fetch = read(r[15]&~(cpsr.b.T?1:3), cpsr.b.T?16:32, next_cycle_type);
		
	return icycles;
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
	case 0xF: printf("Unimpl isCond case $%X\n", cc); dump_regs(); exit(1);
	}
	printf("arm7::isCond, shouldn't get here\n");
	exit(1);
	return true;
}

void arm7tdmi::reset()
{
	r[15] = 0x0800'0000u;
	cpsr.v = 0x80|ARM_MODE_SUPER;
	r[13] = 0x03007F00;
	r13_svc = 0x03007FE0; 
	r13_irq =  0x03007FA0;
	spsr_svc = 0x9f;
	
	decode = read(r[15], 32, ARM_CYCLE::S);
	fetch = read(r[15]+4, 32, ARM_CYCLE::S);
	r[15] += 4;
}

void arm7tdmi::flushp()
{
	decode = read(r[15], (cpsr.b.T ? 16 : 32), ARM_CYCLE::N);
	r[15] += (cpsr.b.T ? 2 : 4);
}

void arm::switch_to_mode(u32)
{
}

void arm::swi()
{
}

