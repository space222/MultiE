#include <cstdlib>
#include <print>
#include "arm7tdmi.h"

#define setNZ(a) cpu.cpsr.b.N = (((a)&BIT(31))?1:0); cpu.cpsr.b.Z = (((a)==0)?1:0)

void thumb1_lsl(arm& cpu, u32 opc)
{
	u32 S = cpu.r[(opc>>3)&7];
	u32& D = cpu.r[opc&7];

	const u32 sh = (opc>>6)&0x1f;
	if( sh == 0 )
	{
		D = S;
		setNZ(D);
		return;
	}
	
	S <<= sh-1;
	cpu.cpsr.b.C = S>>31;
	S <<= 1;
	D = S;
	setNZ(D);
}

void thumb1_lsr(arm& cpu, u32 opc)
{
	u32 S = cpu.r[(opc>>3)&7];
	u32& D = cpu.r[opc&7];

	const u32 sh = (opc>>6)&0x1f;
	if( sh == 0 )
	{
		cpu.cpsr.b.C = S>>31; //?? or 0?
		D = 0;
		setNZ(D);
		return;
	}
	
	S >>= sh-1;
	cpu.cpsr.b.C = S&1;
	S >>= 1;
	D = S;
	setNZ(D);
}

void thumb1_asr(arm& cpu, u32 opc)
{
	u32 S = cpu.r[(opc>>3)&7];
	u32& D = cpu.r[opc&7];

	const u32 sh = (opc>>6)&0x1f;
	if( sh == 0 )
	{
		cpu.cpsr.b.C = S>>31;
		D = (S>>31)?0xffffFFFFu:0;
		setNZ(D);
		return;
	}
	
	S = s32(S) >> (sh-1);
	cpu.cpsr.b.C = S&1;
	S = s32(S) >> 1;
	D = S;
	setNZ(D);
}

void thumb2_addsub(arm& cpu, u32 opc)
{
	u32 Rn = (opc&BIT(10))? ((opc>>6)&7) : cpu.r[(opc>>6)&7];
	if( opc&BIT(9) ) Rn = ~Rn;
	const u32 Rs = cpu.r[(opc>>3)&7];
	u32& Rd = cpu.r[opc&7];
	u64 t = Rs;
	t += Rn;
	if( opc&BIT(9) ) t+=1;
	cpu.cpsr.b.V = ((t^Rs)&(t^Rn)&BIT(31))>>31;
	cpu.cpsr.b.C = (t>>32)&1;
	Rd = t;
	setNZ(Rd);
}

void thumb3_mov(arm& cpu, u32 opc)
{
	const u32 reg = (opc>>8)&7;
	cpu.r[reg] = opc&0xff;
	setNZ(cpu.r[reg]);
}

void thumb3_sub(arm& cpu, u32 opc)
{
	const u32 reg = (opc>>8)&7;
	u64 t = cpu.r[reg];
	u32 op2 = ~(opc&0xff);
	t += op2;
	t += 1;
	cpu.cpsr.b.V = (((t^cpu.r[reg]) & (t^op2) & BIT(31))?1:0);
	cpu.cpsr.b.C = (t>>32)&1;
	cpu.r[reg] = t;
	setNZ(u32(t));
}

void thumb3_cmp(arm& cpu, u32 opc)
{
	const u32 reg = (opc>>8)&7;
	u64 t = cpu.r[reg];
	u32 op2 = ~(opc&0xff);
	t += op2;
	t += 1;
	cpu.cpsr.b.V = (((t^cpu.r[reg]) & (t^op2) & BIT(31))?1:0);
	cpu.cpsr.b.C = (t>>32)&1;
	setNZ(u32(t));
}

void thumb3_add(arm& cpu, u32 opc)
{
	const u32 reg = (opc>>8)&7;
	u64 t = cpu.r[reg];
	t += (opc&0xff);
	cpu.cpsr.b.V = (((t^cpu.r[reg]) & (t^0) & BIT(31))?1:0);
	cpu.cpsr.b.C = (t>>32)&1;
	cpu.r[reg] = t;
	setNZ(cpu.r[reg]);
}

void thumb4_and(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	d &= s;
	setNZ(d);
}

void thumb4_eor(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	d ^= s;
	setNZ(d);
}

void thumb4_lsl(arm& cpu, u32 opc)
{
	const u32 sh = cpu.r[(opc>>3)&7] & 0xff;
	u32& d = cpu.r[opc&7];
	if( sh == 0 ) { setNZ(d); return; }
	if( sh >= 32 )
	{
		cpu.cpsr.b.C = ((sh==32) ? (d&1) : 0); //todo: ??
		d = 0;
		setNZ(d);
		return;
	}
	d <<= sh-1;
	cpu.cpsr.b.C = d>>31;
	d <<= 1;
	setNZ(d);
}

void thumb4_lsr(arm& cpu, u32 opc)
{
	const u32 sh = cpu.r[(opc>>3)&7] & 0xff;
	u32& d = cpu.r[opc&7];
	if( sh == 0 ) { setNZ(d); return; }
	if( sh >= 32 )
	{
		cpu.cpsr.b.C = ((sh == 32) ? (d>>31) : 0);
		d = 0;
		setNZ(d);
		return;
	}
	d >>= sh-1;
	cpu.cpsr.b.C = d&1;
	d >>= 1;
	setNZ(d);
}

void thumb4_asr(arm& cpu, u32 opc)
{
	const u32 sh = cpu.r[(opc>>3)&7] & 0xff;
	u32& d = cpu.r[opc&7];
	if( sh == 0 ) { setNZ(d); return; }
	if( sh >= 32 )
	{
		cpu.cpsr.b.C = d>>31; //todo: ??
		d = (d>>31)?0xffffFFFFu : 0;
		setNZ(d);
		return;
	}
	d = s32(d) >> (sh-1);
	cpu.cpsr.b.C = d&1;
	d = s32(d) >> 1;
	setNZ(d);
}

void thumb4_adc(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	u64 t = d;
	t += s;
	t += cpu.cpsr.b.C;
	cpu.cpsr.b.V = (((t^s)&(t^d)&BIT(31))?1:0);
	cpu.cpsr.b.C = t>>32;
	d = t;
	setNZ(d);
}

void thumb4_sbc(arm& cpu, u32 opc)
{
	const u32 s = ~cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	u64 t = d;
	t += s;//^0xffffFFFFu;
	t += cpu.cpsr.b.C;
	cpu.cpsr.b.V = (((t^s)&(t^d)&BIT(31))?1:0);
	cpu.cpsr.b.C = t>>32;
	d = t;
	setNZ(d);
}

void thumb4_ror(arm& cpu, u32 opc)
{
	const u32 sh = cpu.r[(opc>>3)&7] & 0xff;
	u32& d = cpu.r[opc&7];
	if( sh == 0 ) { setNZ(d); return; }
	d = std::rotr(d, sh);
	cpu.cpsr.b.C = d>>31;
	setNZ(d);
}

void thumb4_tst(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	const u32 d = s & cpu.r[opc&7];
	setNZ(d);
}

void thumb4_neg(arm& cpu, u32 opc)
{
	const u32 s = ~cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	u64 t = 0;
	t += s;//^0xffffFFFFu;
	t += 1;
	cpu.cpsr.b.V = (((t^s)&(t^0)&BIT(31))?1:0);
	cpu.cpsr.b.C = t>>32;
	d = t;
	setNZ(d);
}

void thumb4_cmp(arm& cpu, u32 opc)
{
	const u32 s = ~cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	u64 t = d;
	t += s;//^0xffffFFFFu;
	t += 1;
	cpu.cpsr.b.V = (((t^s)&(t^d)&BIT(31))?1:0);
	cpu.cpsr.b.C = t>>32;
	setNZ(u32(t));
}

void thumb4_cmn(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	u64 t = d;
	t += s;
	cpu.cpsr.b.V = (((t^s)&(t^d)&BIT(31))?1:0);
	cpu.cpsr.b.C = t>>32;
	setNZ(u32(t));
}

void thumb4_orr(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	d |= s;
	setNZ(d);
}

void thumb4_mul(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	d *= s;
	setNZ(d);
}

void thumb4_bic(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	d &= ~s;
	setNZ(d);
}

void thumb4_mvn(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&7];
	u32 &d = cpu.r[opc&7];
	d = ~s;
	setNZ(d);
}

void thumb5_add(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&15];
	u32 RD = ((opc>>4)&8)|(opc&7);
	cpu.r[RD] += s;
	if( RD == 15 ) cpu.flushp();
}

void thumb5_cmp(arm& cpu, u32 opc)
{
	const u32 s = ~cpu.r[(opc>>3)&15];
	const u32 d = cpu.r[((opc>>4)&8)|(opc&7)];
	u64 t = d;
	t += s;
	t += 1;
	cpu.cpsr.b.C = (t>>32)&1;
	cpu.cpsr.b.V = (((t^s) & (t^d) & BIT(31))?1:0);
	setNZ(u32(t));
}

void thumb5_mov(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&15];
	u32 RD = ((opc>>4)&8)|(opc&7);
	cpu.r[RD] = s;
	if( RD == 15 ) cpu.flushp();
}

void thumb5_bx(arm& cpu, u32 opc)
{
	const u32 s = cpu.r[(opc>>3)&15];
	u32 retaddr = (cpu.r[15]-2)|1;
	cpu.r[15] = s;
	if( !(s&1) )
	{
		cpu.cpsr.b.T = 0;
		cpu.r[15] &= ~2;
	}
	cpu.r[15] &= ~1;
	if( cpu.armV >= 5 ) { cpu.r[14] = retaddr; }
	cpu.flushp();
}

void thumb6_ldrpc(arm& cpu, u32 opc)
{
	cpu.r[(opc>>8)&7] = cpu.read((cpu.r[15]&~3) + ((opc&0xff)<<2), 32, ARM_CYCLE::N);
}

void thumb7_str(arm& cpu, u32 opc)
{
	const u32 Rb = cpu.r[(opc>>3)&7];
	const u32 Ro = cpu.r[(opc>>6)&7];
	const u32 ea = (Rb + Ro)&~3;
	cpu.write(ea, cpu.r[opc&7], 32, ARM_CYCLE::N);
}

void thumb7_strb(arm& cpu, u32 opc)
{
	const u32 Rb = cpu.r[(opc>>3)&7];
	const u32 Ro = cpu.r[(opc>>6)&7];
	const u32 ea = (Rb + Ro);
	cpu.write(ea, cpu.r[opc&7]&0xff, 8, ARM_CYCLE::N);
}

void thumb7_ldr(arm& cpu, u32 opc)
{
	const u32 Rb = cpu.r[(opc>>3)&7];
	const u32 Ro = cpu.r[(opc>>6)&7];
	const u32 ea = (Rb + Ro);
	cpu.r[opc&7] = std::rotr(cpu.read(ea&~3, 32, ARM_CYCLE::N), (ea&3)*8);
}

void thumb7_ldrb(arm& cpu, u32 opc)
{
	const u32 Rb = cpu.r[(opc>>3)&7];
	const u32 Ro = cpu.r[(opc>>6)&7];
	const u32 ea = (Rb + Ro);
	cpu.r[opc&7] = cpu.read(ea, 8, ARM_CYCLE::N);
}

void thumb8_strh(arm& cpu, u32 opc)
{
	const u32 Rb = cpu.r[(opc>>3)&7];
	const u32 Ro = cpu.r[(opc>>6)&7];
	const u32 ea = (Rb + Ro)&~1;
	cpu.write(ea, cpu.r[opc&7]&0xffff, 16, ARM_CYCLE::N);
}

void thumb8_ldsb(arm& cpu, u32 opc)
{
	const u32 Rb = cpu.r[(opc>>3)&7];
	const u32 Ro = cpu.r[(opc>>6)&7];
	const u32 ea = (Rb + Ro);
	cpu.r[opc&7] = (s8)cpu.read(ea, 8, ARM_CYCLE::N);
}

void thumb8_ldrh(arm& cpu, u32 opc)
{
	const u32 Rb = cpu.r[(opc>>3)&7];
	const u32 Ro = cpu.r[(opc>>6)&7];
	const u32 ea = (Rb + Ro);
	cpu.r[opc&7] = std::rotr(cpu.read(ea&~1, 16, ARM_CYCLE::N), (ea&1)*8);
}

void thumb8_ldsh(arm& cpu, u32 opc)
{
	const u32 Rb = cpu.r[(opc>>3)&7];
	const u32 Ro = cpu.r[(opc>>6)&7];
	const u32 ea = (Rb + Ro);
	if( ea & 1 )
	{
		cpu.r[opc&7] = (s8)cpu.read(ea, 8, ARM_CYCLE::N);
	} else {
		cpu.r[opc&7] = (s16)cpu.read(ea&~1, 16, ARM_CYCLE::N);
	}
}

void thumb9_str(arm& cpu, u32 opc)
{
	const u32 nn = ((opc>>6)&0x1f)<<2;
	const u32 ea = cpu.r[(opc>>3)&7] + nn;
	cpu.write(ea&~3, cpu.r[opc&7], 32, ARM_CYCLE::N);
}

void thumb9_ldr(arm& cpu, u32 opc)
{
	const u32 nn = ((opc>>6)&0x1f)<<2;
	const u32 ea = cpu.r[(opc>>3)&7] + nn;
	cpu.r[opc&7] = std::rotr(cpu.read(ea&~3, 32, ARM_CYCLE::N), (ea&3)*8);	
}

void thumb9_strb(arm& cpu, u32 opc)
{
	const u32 nn = ((opc>>6)&0x1f);
	const u32 ea = cpu.r[(opc>>3)&7] + nn;
	cpu.write(ea, cpu.r[opc&7]&0xff, 8, ARM_CYCLE::N);
}

void thumb9_ldrb(arm& cpu, u32 opc)
{
	const u32 nn = ((opc>>6)&0x1f);
	const u32 ea = cpu.r[(opc>>3)&7] + nn;
	cpu.r[opc&7] = cpu.read(ea, 8, ARM_CYCLE::N);
}

void thumb10_ldrh(arm& cpu, u32 opc)
{
	const u32 nn = ((opc>>6)&0x1f)<<1;
	const u32 ea = cpu.r[(opc>>3)&7] + nn;
	cpu.r[opc&7] = std::rotr(cpu.read(ea&~1, 16, ARM_CYCLE::N), (ea&1)*8);
}

void thumb10_strh(arm& cpu, u32 opc)
{
	const u32 nn = ((opc>>6)&0x1f)<<1;
	const u32 ea = cpu.r[(opc>>3)&7] + nn;
	cpu.write(ea&~1, cpu.r[opc&7]&0xffff, 16, ARM_CYCLE::N);
}

void thumb11_ldr(arm& cpu, u32 opc)
{
	const u32 ea = cpu.r[13] + ((opc&0xff)<<2);
	cpu.r[(opc>>8)&7] = std::rotr(cpu.read(ea&~3, 32, ARM_CYCLE::N), (ea&3)*8);
}

void thumb11_str(arm& cpu, u32 opc)
{
	const u32 ea = cpu.r[13] + ((opc&0xff)<<2);
	cpu.write(ea&~3, cpu.r[(opc>>8)&7], 32, ARM_CYCLE::N);
}

void thumb12_addsppc(arm& cpu, u32 opc)
{
	u32& d = cpu.r[(opc>>8)&7];
	if( opc & BIT(11) )
	{
		d = cpu.r[13] + ((opc&0xff)<<2);
	} else {
		d = (cpu.r[15]&~3) + ((opc&0xff)<<2);
	}
}

void thumb13_addspnn(arm& cpu, u32 opc)
{
	if( opc & BIT(7) )
	{
		cpu.r[13] -= (opc&0x7f)<<2;
	} else {
		cpu.r[13] += (opc&0x7f)<<2;
	}
}

void thumb14_pop(arm& cpu, u32 opc)
{
	ARM_CYCLE type = ARM_CYCLE::N;
	for(u32 i = 0; i < 8; ++i)
	{
		if( opc & BIT(i) )
		{
			cpu.r[i] = cpu.read(cpu.r[13]&~3, 32, type);
			cpu.r[13] += 4;
			type = ARM_CYCLE::S;
		}
	}
	if( opc & BIT(8) )
	{
		cpu.r[15] = cpu.read(cpu.r[13]&~3, 32, type);
		if( cpu.armV >= 5 && !(cpu.r[15]&1) ) cpu.cpsr.b.T = 0;
		cpu.r[13] += 4;
		cpu.flushp();
	}
}

void thumb14_push(arm& cpu, u32 opc)
{
	ARM_CYCLE type = ARM_CYCLE::N;
	if( opc & BIT(8) )
	{
		cpu.r[13] -= 4;
		cpu.write(cpu.r[13]&~3, cpu.r[14], 32, type);
		type = ARM_CYCLE::S;
	}
	for(u32 i = 0; i < 8; ++i)
	{
		if( opc & BIT(7-i) )
		{
			cpu.r[13] -= 4;
			cpu.write(cpu.r[13]&~3, cpu.r[7-i], 32, type);
			type = ARM_CYCLE::S;
		}
	}
}

void thumb15_ldmia(arm& cpu, u32 opc)
{
	u32 RB = (opc>>8)&7;
	u32 base = cpu.r[(opc>>8)&7];
	if( !(opc&0xff) )
	{
		if( cpu.armV < 5 )
		{
			cpu.r[15] = cpu.read(base&~3, 32, ARM_CYCLE::N);
			cpu.flushp();
		}
		cpu.r[RB] += 0x40; //??
		return;
	}
	ARM_CYCLE type = ARM_CYCLE::N;
	for(u32 i = 0; i < 8; ++i)
	{
		if( opc & BIT(i) )
		{
			cpu.r[i] = cpu.read(base&~3, 32, type);
			type = ARM_CYCLE::S;
			base += 4;
		}
	}
	if( opc & BIT(RB) ) return;
	cpu.r[RB] = base;
}

void thumb15_stmia(arm& cpu, u32 opc)
{
	int RB = (opc>>8)&7;
	u32 base = cpu.r[RB];
	u32 start = base;
	if( !(opc&0xff) )
	{
		if( cpu.armV < 5 ) cpu.write(base&~3, cpu.r[15]+2, 32, ARM_CYCLE::N);
		cpu.r[RB] += 0x40; //??
		return;
	}
	ARM_CYCLE type = ARM_CYCLE::N;
	bool store_new_base = (std::countr_zero(opc&0xff) != RB && cpu.armV<5);
	for(int i = 0; i < 8; ++i)
	{
		if( opc & BIT(i) )
		{
			cpu.write(base&~3, (i==RB && store_new_base)? start+std::popcount(opc&0xff)*4 : cpu.r[i], 32, type);
			type = ARM_CYCLE::S;
			base += 4;
		}
	}
	cpu.r[RB] = base;
}

void thumb16_bcc(arm& cpu, u32 opc)
{
	if( cpu.isCond((opc>>8)&15) )
	{
		cpu.r[15] += s8(opc)<<1;
		cpu.flushp();
	}
}

void thumb17_swi(arm& cpu, u32)
{
	cpu.spsr_svc = cpu.cpsr.v;
	cpu.switch_to_mode(ARM_MODE_SUPER);
	cpu.cpsr.b.M = ARM_MODE_SUPER;
	cpu.cpsr.b.I = 1;
	cpu.cpsr.b.T = 0;
	cpu.r[14] = cpu.r[15]-2;
	cpu.r[15] = 8;
	cpu.flushp();
	//std::println("Thumb SWI");
}

void thumb18_b(arm& cpu, u32 opc)
{
	s16 offset = (opc<<5);
	offset >>= 4;
	cpu.r[15] += offset;
	cpu.flushp();
}

void thumb19_bl(arm& cpu, u32 opc)
{
	if( opc & BIT(11) )
	{
		u32 temp = (cpu.r[15]-2)|1;
		cpu.r[15] = cpu.r[14] + ((opc&0x7ff)<<1);
		//std::println("thumb bl to ${:X}", cpu.r[15]);
		cpu.r[15] |= 1;
		cpu.r[14] = temp;
		cpu.flushp();
	} else {
		cpu.r[14] = cpu.r[15] + ((opc&0x7ff)<<12) + ((opc&0x400)?0xff800000u:0);
	}
}

arm7_instr arm7tdmi::decode_thumb(u16 opc)
{
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
		if( opc&BIT(12) ) return thumb19_bl;
		return thumb18_b;
	default: break;
	}
	std::println("thumb_decode error");
	exit(1);
	return nullptr;
}

void arm7tdmi::flushp() 
{
	if( cpsr.b.T )
	{
		r[15] &= ~1;
		decode = read(r[15], 16, ARM_CYCLE::N);
		r[15] += 2;
		fetch = read(r[15], 16, ARM_CYCLE::S);	
	} else {
		r[15] &= ~3;
		decode = read(r[15], 32, ARM_CYCLE::N);
		r[15] += 4;
		fetch = read(r[15], 32, ARM_CYCLE::S);
	}
}

void arm7tdmi::step()
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
	fetch = read(r[15]&(cpsr.b.T?~1:~3), (cpsr.b.T ? 16 : 32), ARM_CYCLE::X);
	u32 opc = execute;
	
	//if( r[15] > 0x08000740 ) std::println("${:X}:{:X}: opc = ${:X}", r[15] - (cpsr.b.T?4:8), u32(cpsr.b.T), opc);
	
	if( r[15] >= 0x10000000u )
	{
		std::println("${:X} too big, halting", (r[15]&(cpsr.b.T?~1:~3)) - (cpsr.b.T?4:8));
		exit(1);
	}
	
	if( cpsr.b.T )
	{
		decode_thumb(opc)(*this, opc);
	} else {
		if( isCond(opc>>28) ) 
		{
 			decode_arm(((opc>>16)&0xff0) | ((opc>>4)&15))(*this, opc);
		}
	}
	stamp+=1;
}

void arm7tdmi::reset()
{
	//todo: copied from my old emu, need to double check
	cpsr.b.M = ARM_MODE_USER;
	r[13] = 0x03007F00;
	r13_svc = 0x03007FE0; 
	r13_irq =  0x03007FA0;
	switch_to_mode(ARM_MODE_SUPER);
	spsr_svc = 0x9f;
	r[15] = 0;//0x08000000u;
	cpsr.b.T = 0;
	cpsr.b.I = 0;
	stamp = 0;
	flushp();
	irq_line = false;
}

bool arm7tdmi::isCond(u8 cc)
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
	return false; 
}

void arm7tdmi::switch_to_mode(u32 mode) 
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

void arm7tdmi::swi() { }

void arm::dump_regs()
{
	for(u32 i = 0; i < 16; ++i)
	{
		std::print("r{}=${:X} ", i, r[i]);
		if( i%4 == 0 && i ) std::println();
	}
	std::println();
}

arm7tdmi::arm7tdmi()
{
	armV = 4;
}










