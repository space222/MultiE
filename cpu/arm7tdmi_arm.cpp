#include <cstdlib>
#include <print>
#include "arm7tdmi.h"

#define SBIT (opc&BIT(20))
#define setNZ(a) cpu.cpsr.b.Z = ((a)==0?1:0); cpu.cpsr.b.N = ((a)>>31)&1

static auto undef_instr = [](arm& cpu, u32 opc) { std::println("${:X}: unimpl opc ${:X}", cpu.r[15]-(cpu.cpsr.b.T?4:8), opc); exit(1); };

void arm7_mul(arm& cpu, u32 opc)
{
	const u32 Rs = cpu.r[(opc>>8)&15];
	const u32 Rn = cpu.r[(opc>>12)&15];
	const u32 Rm = cpu.r[opc&15];
	u32& Rd = cpu.r[(opc>>16)&15];
	Rd = Rs * Rm;
	if( opc & BIT(21) ) Rd += Rn;
	if( SBIT ) { setNZ(Rd); }
	if( ((opc>>16)&15) == 15 ) cpu.flushp();
}

void arm7_mul_long(arm& cpu, u32 opc)
{
	const bool U = !(opc & BIT(22));
	const bool A = opc & BIT(21);
	const u32 RdLo = (opc>>12)&15;
	const u32 RdHi = (opc>>16)&15;
	const u32 S = cpu.r[(opc>>8)&15];
	const u32 M = cpu.r[opc&15];
	if( U )
	{
		u64 temp = S;
		temp *= M;
		if( A )
		{
			u64 val = cpu.r[RdHi];
			val <<= 32;
			val |= cpu.r[RdLo];
			temp += val;
		}
		cpu.r[RdHi] = temp>>32;
		cpu.r[RdLo] = temp;
		if( SBIT )
		{
			cpu.cpsr.b.N = temp>>63;
			cpu.cpsr.b.Z = ((temp==0)?1:0);
		}
	} else {
		s64 temp = s32(S);
		temp *= s32(M);
		if( A )
		{
			u64 val = cpu.r[RdHi];
			val <<= 32;
			val |= cpu.r[RdLo];
			temp += val;
		}
		cpu.r[RdHi] = temp>>32;
		cpu.r[RdLo] = temp;
		if( SBIT )
		{
			cpu.cpsr.b.N = (temp>>63)&1;
			cpu.cpsr.b.Z = ((temp==0)?1:0);
		}
	}
}

void arm7_single_swap(arm& cpu, u32 opc)
{
	const u32 Rm = cpu.r[opc&15];
	const u32 base = cpu.r[(opc>>16)&15];
	u32 &Rd = cpu.r[(opc>>12)&15];
	
	if( opc & BIT(22) )
	{
		u32 t = cpu.read(base, 8, ARM_CYCLE::N);
		cpu.write(base, Rm&0xff, 8, ARM_CYCLE::N);
		Rd = t;
	} else {
		u32 t = std::rotr(cpu.read(base&~3, 32, ARM_CYCLE::N), (base&3)*8);
		cpu.write(base&~3, Rm, 32, ARM_CYCLE::N);
		Rd = t;
	}
	if( ((opc>>12)&0x1f) == 15 ) { cpu.flushp(); }
	cpu.stamp += 1;
}

void arm7_xfer_half(arm& cpu, u32 opc)
{
	u32 offs = (opc&BIT(22)) ? (((opc>>4)&0xf0)|(opc&15)) : cpu.r[opc&15];
	u32 n = (opc>>16)&15;
	u32 d = (opc>>12)&15;
	u32 base = cpu.r[n];
	const bool up = opc & BIT(23);
	const bool pre = opc & BIT(24);
	const bool wback = !pre || (opc & BIT(21));
	
	if( pre )
	{
		base += up ? offs : -offs;
	}
	
	if( opc & BIT(20) )
	{
		cpu.r[d] = std::rotr(cpu.read(base&~1, 16, ARM_CYCLE::N), (base&1)*8);
	} else {
		cpu.write(base&~1, cpu.r[d]&0xffff, 16, ARM_CYCLE::N);
	}
	
	if( !pre )
	{
		base += up ? offs : -offs;
	}
	
	if( wback && (n != d || !(opc&BIT(20))) ) cpu.r[n] = base;	
}

void arm7_xfer_signed(arm& cpu, u32 opc)
{
	u32 offs = (opc&BIT(22)) ? (((opc>>4)&0xf0)|(opc&15)) : cpu.r[opc&15];
	u32 n = (opc>>16)&15;
	u32 d = (opc>>12)&15;
	u32 base = cpu.r[n];
	const bool up = opc & BIT(23);
	const bool pre = opc & BIT(24);
	const bool wback = !pre || (opc & BIT(21));
	
	if( pre )
	{
		base += up ? offs : -offs;
		if( wback && n != d ) cpu.r[n] = base;
	}
	
	if( (base&1) || (opc & BIT(5))==0 )
	{
		cpu.r[d] = (s8)cpu.read(base, 8, ARM_CYCLE::N);
	} else {
		cpu.r[d] = (s16)cpu.read(base, 16, ARM_CYCLE::N);
	}
	
	if( !pre )
	{
		base += up ? offs : -offs;
		if( wback && n != d ) cpu.r[n] = base;
	}
}

void arm7_bx(arm& cpu, u32 opc)
{
	u32 nia = cpu.r[opc&15];
	cpu.cpsr.b.T = nia&1;
	cpu.r[15] = nia & ((nia&1) ? ~1 : ~3);
	cpu.flushp();
}

void arm7_mrs(arm& cpu, u32 opc)
{
	u32& reg = cpu.r[(opc>>12)&15];
	if( opc & BIT(22) )
	{
		reg = cpu.getSPSR();
	} else {
		reg = cpu.cpsr.v;
	}
}

void arm7_msr_reg(arm& cpu, u32 opc)
{
	u32 val = cpu.r[opc&15];
	u32 curpsr = ((opc&BIT(22)) ? cpu.getSPSR() : cpu.cpsr.v);
	u32 mask = 0;
	if( opc & BIT(19) ) mask |= 0xff000000u;
	if( cpu.cpsr.b.M != ARM_MODE_USER && (opc&BIT(16)) ) mask |= 0xffu;
	curpsr = (curpsr&~mask) | (val&mask);
	if( opc & BIT(22) )
	{
		cpu.setSPSR(curpsr);
	} else {
		if( (curpsr & 0x1f) != cpu.cpsr.b.M )
		{
			cpu.switch_to_mode(curpsr&0x1f);
		}
		cpu.cpsr.v = curpsr;
	}
}

void arm7_msr_imm(arm& cpu, u32 opc)
{
	u32 val = std::rotr(opc&0xff, ((opc>>8)&15)<<1);
	u32 curpsr = ((opc&BIT(22)) ? cpu.getSPSR() : cpu.cpsr.v);
	u32 mask = 0;
	if( opc & BIT(19) ) mask |= 0xff000000u;
	if( cpu.cpsr.b.M != ARM_MODE_USER && (opc&BIT(16)) ) mask |= 0xffu;
	curpsr = (curpsr&~mask) | (val&mask);
	if( opc & BIT(22) )
	{
		cpu.setSPSR(curpsr);
	} else {
		if( (curpsr & 0x1f) != cpu.cpsr.b.M )
		{
			cpu.switch_to_mode(curpsr&0x1f);
		}
		cpu.cpsr.v = curpsr;
	}
}

u32 dataproc_op2(arm& cpu, u32 opc)
{
	if( opc & BIT(25) )
	{
		u32 rot = ((opc>>8)&15)<<1;
		u32 val = opc&0xff;
		if( rot )
		{
			val = std::rotr(val, rot);
			if( SBIT ) cpu.cpsr.b.C = val>>31;
		}
		return val;
	}

	const u32 Rm = opc&15;
	const bool imm = !(opc&BIT(4));
	const u32 Rs = ( !imm ? ((opc>>8)&15):((opc>>7)&0x1f));
	const u32 type = ((opc>>5)&3);

	u32 val = cpu.r[Rm] + ((Rm==15 && !imm) ? 4 : 0);
	if( !imm && !(cpu.r[Rs]&0xff) ) return val; // reg shifts by zero just return val
	if( imm && type==0 && !Rs ) return val; // lsl by imm 0 just returns val
	if( imm && type==3 && !Rs )
	{ // ror by imm 0 is RRX#1
		u32 C = cpu.cpsr.b.C;
		if( SBIT ) cpu.cpsr.b.C = val&1;
		return (C<<31)|(val>>1);	
	}
	
	const u32 sh = !imm ? (cpu.r[Rs]&0xff) : (Rs ? Rs : 32);
	
	if( type == 0 )
	{ // LSL
		if( sh == 32 )
		{
			if( SBIT ) cpu.cpsr.b.C = val&1;
			return 0;
		}
		if( sh > 32 )
		{
			if( SBIT ) cpu.cpsr.b.C = 0;
			return 0;
		}
		val <<= sh-1;
		if( SBIT ) cpu.cpsr.b.C = val>>31;
		return val << 1;
	}
	
	if( type == 1 )
	{ // LSR
		if( sh == 32 )
		{
			if( SBIT ) cpu.cpsr.b.C = val>>31;
			return 0;
		}
		if( sh > 32 )
		{
			if( SBIT ) cpu.cpsr.b.C = 0;
			return 0;
		}
		val >>= sh-1;
		if( SBIT ) cpu.cpsr.b.C = val&1;
		return val>>1;
	}

	if( type == 2 )
	{ // ASR
		if( sh >= 32 )
		{
			if( SBIT ) cpu.cpsr.b.C = val>>31;
			return (val&BIT(31))?0xffffFFFFu:0;
		}
		val = s32(val) >> (sh-1);
		if( SBIT ) cpu.cpsr.b.C = val&1;
		return s32(val) >> 1;
	}
	
	// ROR
	if( sh == 32 )
	{
		if( SBIT ) cpu.cpsr.b.C = val>>31;
		return val;
	}
	val = std::rotr(val, sh);
	if( SBIT ) cpu.cpsr.b.C = val>>31;
	return val;
}

void arm7_and(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = cpu.r[(opc>>16)&15] & op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(cpu.r[Rd]);
		}	
	}
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_eor(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = cpu.r[(opc>>16)&15] ^ op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(cpu.r[Rd]);
		}	
	}
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_sub(arm& cpu, u32 opc)
{
	u32 op2 = ~dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 op1 = cpu.r[(opc>>16)&15];
	u64 res = op1;
	res += op2;
	res += 1;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(u32(res));
			cpu.cpsr.b.V = ((res^op1)&(res^op2)&BIT(31))?1:0;
			cpu.cpsr.b.C = res>>32;
		}	
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_rsb(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 op1 = ~cpu.r[(opc>>16)&15];
	u64 res = op1;
	res += op2;
	res += 1;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(u32(res));
			cpu.cpsr.b.V = ((res^op1)&(res^op2)&BIT(31))?1:0;
			cpu.cpsr.b.C = res>>32;
		}	
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_add(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 op1 = cpu.r[(opc>>16)&15];
	u64 res = op1;
	res += op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(u32(res));
			cpu.cpsr.b.V = ((res^op1)&(res^op2)&BIT(31))?1:0;
			cpu.cpsr.b.C = res>>32;
		}	
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_adc(arm& cpu, u32 opc)
{
	u32 C = cpu.cpsr.b.C;
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 op1 = cpu.r[(opc>>16)&15];
	u64 res = op1;
	res += op2;
	res += C;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(u32(res));
			cpu.cpsr.b.V = ((res^op1)&(res^op2)&BIT(31))?1:0;
			cpu.cpsr.b.C = res>>32;
		}	
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_sbc(arm& cpu, u32 opc)
{
	u32 C = cpu.cpsr.b.C;
	u32 op2 = ~dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 op1 = cpu.r[(opc>>16)&15];
	u64 res = op1;
	res += op2;
	res += C;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(u32(res));
			cpu.cpsr.b.V = ((res^op1)&(res^op2)&BIT(31))?1:0;
			cpu.cpsr.b.C = res>>32;
		}	
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_rsc(arm& cpu, u32 opc)
{
	u32 C = cpu.cpsr.b.C;
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 op1 = ~cpu.r[(opc>>16)&15];
	u64 res = op1;
	res += op2;
	res += C;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(u32(res));
			cpu.cpsr.b.V = ((res^op1)&(res^op2)&BIT(31))?1:0;
			cpu.cpsr.b.C = res>>32;
		}	
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_tst(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 res = cpu.r[(opc>>16)&15] & op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(res);
		}	
	}
}

void arm7_teq(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 res = cpu.r[(opc>>16)&15] ^ op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(res);
		}	
	}
}

void arm7_cmp(arm& cpu, u32 opc)
{
	u32 op2 = ~dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 op1 = cpu.r[(opc>>16)&15];
	u64 res = op1;
	res += op2;
	res += 1;
	//if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(u32(res));
			cpu.cpsr.b.V = ((res^op1)&(res^op2)&BIT(31))?1:0;
			cpu.cpsr.b.C = res>>32;
		}	
	}
}

void arm7_cmn(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	u32 op1 = cpu.r[(opc>>16)&15];
	u64 res = op1;
	res += op2;
	//if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(u32(res));
			cpu.cpsr.b.V = ((res^op1)&(res^op2)&BIT(31))?1:0;
			cpu.cpsr.b.C = res>>32;
		}	
	}
}

void arm7_orr(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = cpu.r[(opc>>16)&15] | op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(cpu.r[Rd]);
		}	
	}
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_mov(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(cpu.r[Rd]);
		}	
	}
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_bic(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = cpu.r[(opc>>16)&15] & ~op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(cpu.r[Rd]);
		}	
	}
	if( Rd == 15 ) { cpu.flushp(); }
}

void arm7_mvn(arm& cpu, u32 opc)
{
	u32 op2 = dataproc_op2(cpu, opc);
	const u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = ~op2;
	if( SBIT )
	{
		if( Rd == 15 )
		{
			u32 v = cpu.getSPSR();
			cpu.switch_to_mode(v&0x1f);
			cpu.cpsr.v = v;
		} else {
			setNZ(cpu.r[Rd]);
		}	
	}
	if( Rd == 15 ) { cpu.flushp(); }
}

u32 arm7_ldst_shift(arm& cpu, u32 opc)
{
	const u32 sh = (opc>>7) & 0x1f;
	const u32 type = (opc>>5) & 3;
	u32 val = cpu.r[opc&15];
	
	if( type == 0 )
	{ // lsl
		if( sh == 0 ) return val;
		return val << sh;
	}
	
	if( type == 1 )
	{ // lsr
		if( sh == 0 ) return 0;
		return val >> sh;
	}
	
	if( type == 2 )
	{ // asr
		if( sh == 0 ) return (val&BIT(31)) ? 0xffffFFFFu : 0;
		return s32(val) >> sh;
	}
	
	// ror
	if( sh == 0 )
	{ // rrx#1
		u32 C = cpu.cpsr.b.C;
		//ldr/str barrel shifter doesn't update C
		return (C<<31)|(val>>1);
	}
	return std::rotr(val, sh);
}

void arm7_ldst_reg(arm& cpu, u32 opc)
{
	const bool pre = opc & BIT(24);
	const bool wback = !pre || (opc&BIT(21));
	const bool up = opc & BIT(23);
	const bool byte = opc & BIT(22);
	const bool user = !pre && (opc&BIT(21));
	const bool load = opc & BIT(20);
	u32 offs = arm7_ldst_shift(cpu, opc);
	const u32 Rn = (opc>>16)&15;
	u32 base = cpu.r[Rn];
	const u32 Rd = (opc>>12)&15;
	
	if( pre )
	{
		base += up ? offs : -offs;
	}
	
	if( load )
	{
		u32 val = byte ? cpu.read(base, 8, ARM_CYCLE::N) : std::rotr(cpu.read(base&~3, 32, ARM_CYCLE::N), (base&3)*8);
		if( user )
		{
			cpu.setUserReg(Rd, val);
		} else {
			cpu.r[Rd] = val;
		}
		if( Rd == 15 ) cpu.flushp();
	} else {
		cpu.write(base&(byte?~0:~3), ((Rd==15)?4:0) + (user?cpu.getUserReg(Rd):cpu.r[Rd]), byte?8:32, ARM_CYCLE::N);
	}
	
	if( !pre )
	{
		base += up ? offs : -offs;
	}

	if( wback && (!load || Rd != Rn) )
	{
		cpu.r[Rn] = base;
	}
}

void arm7_ldst_imm(arm& cpu, u32 opc)
{
	const bool pre = opc & BIT(24);
	const bool wback = !pre || (opc&BIT(21));
	const bool up = opc & BIT(23);
	const bool byte = opc & BIT(22);
	const bool user = !pre && (opc&BIT(21));
	const bool load = opc & BIT(20);
	u32 offs = opc & 0xfff;
	const u32 Rn = (opc>>16)&15;
	u32 base = cpu.r[Rn];
	const u32 Rd = (opc>>12)&15;
	
	if( pre )
	{
		base += up ? offs : -offs;
	}
	
	if( load )
	{
		u32 val = byte ? cpu.read(base, 8, ARM_CYCLE::N) : std::rotr(cpu.read(base&~3, 32, ARM_CYCLE::N), (base&3)*8);
		if( user )
		{
			cpu.setUserReg(Rd, val);
		} else {
			cpu.r[Rd] = val;
		}
		if( Rd == 15 ) cpu.flushp();
	} else {
		cpu.write(base&(byte?~0:~3), ((Rd==15)?4:0) + (user?cpu.getUserReg(Rd):cpu.r[Rd]), byte?8:32, ARM_CYCLE::N);
	}
	
	if( !pre )
	{
		base += up ? offs : -offs;
	}

	if( wback && (!load || Rd != Rn) )
	{
		cpu.r[Rn] = base;
	}
}

void arm7_ldst_m(arm& cpu, u32 opc)
{
	const bool U = opc & BIT(23);
	const bool Pre = ((opc>>24)&1) ^ ((opc>>23)&1);
	const bool S = opc & BIT(22);
	const bool W = opc & BIT(21);
	const bool L = opc & BIT(20);
	const int Rn = (opc>>16)&15;
	const u32 rlist = opc&0xffff;
	
	u32 base = cpu.r[Rn] - (U ? 0 : std::popcount(rlist)*4);
	u32 start = base;
	ARM_CYCLE ctype = ARM_CYCLE::N;
	
	if( !rlist )
	{
		if( L )
		{
			cpu.r[15] = cpu.read(base&~3, 32, ARM_CYCLE::N);
			cpu.flushp();
		} else {
			cpu.write(base&~3, cpu.r[15], 32, ARM_CYCLE::N);
		}
		cpu.r[Rn] += (U ? 0x40 : -0x40);
		return;
	}
	
	for(u32 i = 0; i < 16; ++i)
	{
		if( !(rlist & BIT(i)) ) continue;

		if( Pre ) base += 4;	
		if( L )
		{
			u32 val = cpu.read(base&~3, 32, ctype);
			if( U )
			{
				cpu.setUserReg(i, val);
			} else {
				cpu.r[i] = val;
				if( i == 15 )
				{
					if( S ) { val = cpu.getSPSR(); cpu.switch_to_mode(val); cpu.cpsr.v = val; }
					cpu.r[15] &= ~3;
					cpu.flushp();
				}
			}
		} else {
			cpu.write(base&~3, (U ? cpu.getUserReg(i) : cpu.r[i]), 32, ctype);
		}
		ctype = ARM_CYCLE::S;
		if( !Pre ) base += 4;	
	}
	
	if( W )
	{
		if( L && (rlist & BIT(Rn)) ) return;
		cpu.r[Rn] = (U ? base : start);
	}
}

arm7_instr arm7tdmi::decode_arm(u32 cc)
{
	if( (cc&0xfcf) == 9 ) { return arm7_mul; }
	if( (cc&0xf8f) == 0x89 ) { return arm7_mul_long; }
	if( (cc&0xfbf) == 0x109) { return arm7_single_swap; }

	if( (cc&0xe0f) == 0xB ) { return arm7_xfer_half; }
	if( (cc&0xe1d) == 0x1D ){ return arm7_xfer_signed; }
	
	if( (cc&0xfbf) == 0x100 ) { return arm7_mrs; }
	if( (cc&0xfbf) == 0x120 ) { return arm7_msr_reg; }
	if( (cc&0xfb0) == 0x320 ) { return arm7_msr_imm; }

	if( (cc&0xfff) == 0x121 ) { return arm7_bx; }

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

	if( (cc&0xf00) == 0xf00 )
	{   // SWI
		return [](arm& cpu, u32) {
			cpu.r14_svc = cpu.r[15];
			cpu.spsr_svc = cpu.cpsr.v;
			cpu.switch_to_mode(ARM_MODE_SUPER);
			cpu.cpsr.b.I = 1;
			cpu.r[15] = 8;
			cpu.flushp();
		};
	}

	return undef_instr;
}

