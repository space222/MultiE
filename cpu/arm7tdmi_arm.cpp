#include <cstdio>
#include <cstdlib>
#include <bit>
#include "arm7tdmi.h"

u32 arm::barrelshifter(u32 opc, bool setc)
{
	return 0;
}

u32 arm::rotate_imm(u32 opc, bool setc)
{	//todo: this can set carry for some reason
	u32 imm = opc&0xff;
	return std::rotr(imm, ((opc>>8)&0xf)*2);
}

void arm_swi(arm& cpu, u32 opc)
{
}

void arm_branch(arm& cpu, u32 opc)
{
	if( opc & BIT(24) )
	{
		cpu.r[14] = cpu.r[15] - 4;
	}
	if( opc & BIT(23) ) 
	{
		opc |= 0xff00'0000;
	} else {
		opc &=~0xff00'0000;
	}
	cpu.r[15] += s32(opc<<2);
	//printf("ARM branch to $%X\n", cpu.r[15]);
	cpu.flushp();
}

void arm_block_data(arm& cpu, u32 opc)
{
}

void arm_undef(arm&, u32)
{
	printf("ARM7: explicit undef\n");
	exit(1);
}

void arm_single_data(arm& cpu, u32 opc)
{
	cpu.icycles += 1;
	u32 offs = ( (opc&BIT(25)) ? cpu.barrelshifter(opc&0xfff, false) : (opc&0xfff) );
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	u32 base = cpu.r[Rn];
	if( opc & BIT(24) ) 
	{
		base += (opc&BIT(23)) ? offs : -offs;
		if( opc & BIT(21) ) cpu.r[Rn] = base;
	} else {
		cpu.r[Rn] = base + ((opc&BIT(23)) ? offs : -offs);
	}
	
	if( opc & BIT(20) )
	{ // load
		cpu.r[Rd] = cpu.read(base, ((opc&BIT(22))?8:32), ARM_CYCLE::N);
		if( Rd == 15 ) cpu.flushp();
	} else {
		cpu.write(base, cpu.r[Rd] + ((Rd==15)?4:0), 32, ARM_CYCLE::N);
		cpu.next_cycle_type = ARM_CYCLE::N;
	}
}

void arm_bx(arm& cpu, u32 opc)
{
	cpu.r[15] = cpu.r[opc&15]&~1;
	printf("$%X: ARM BX to $%X\n", opc, cpu.r[opc&15]);
	cpu.cpsr.b.T = cpu.r[opc&15]&1;
	cpu.flushp();
}

void arm_mul(arm& cpu, u32 opc)
{
}

void arm_mul_long(arm& cpu, u32 opc)
{
}

void arm_single_swap(arm& cpu, u32 opc)
{
	cpu.icycles += 1;
	cpu.next_cycle_type = ARM_CYCLE::N;
	u32& Rm = cpu.r[opc&15];
	u32& Rd = cpu.r[(opc>>12)&15];
	u32& Rn = cpu.r[(opc>>16)&15];
	u32 size = ( (opc&BIT(22)) ? 8 : 32 );

	u32 temp = cpu.read(Rn, size, ARM_CYCLE::N);
	cpu.write(Rn, Rm, size, ARM_CYCLE::N);
	Rd = temp;
}

void arm_tranhw_reg(arm& cpu, u32 opc)
{
	cpu.icycles += 1;
	u32 offs = cpu.r[opc&15];
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	u32 base = cpu.r[Rn];
	
	u32 type = (opc>>5)&3;	
	
	if( opc & BIT(24) ) 
	{
		base += (opc&BIT(23)) ? offs : -offs;
		if( opc & BIT(21) ) cpu.r[Rn] = base;
	} else {
		cpu.r[Rn] = base + ((opc&BIT(23)) ? offs : -offs);
	}
	
	if( opc & BIT(20) )
	{ // load
		cpu.r[Rd] = (u16)cpu.read(base, ((type==2)?8:16), ARM_CYCLE::N);
		if( type == 2 ) cpu.r[Rd] = (s32)(s8)cpu.r[Rd];
		else if( type == 3 ) cpu.r[Rd] = (s32)(s16)cpu.r[Rd];
		if( Rd == 15 ) cpu.flushp();
	} else {
		cpu.write(base, cpu.r[Rd] + ((Rd==15)?4:0), ((type==2)?8:16), ARM_CYCLE::N);
		cpu.next_cycle_type = ARM_CYCLE::N;
	}
}

void arm_tranhw_imm(arm& cpu, u32 opc)
{
	cpu.icycles += 1;
	u32 offs = ((opc>>4)&0xf0)|(opc&15);
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	u32 base = cpu.r[Rn];
	
	u32 type = (opc>>5)&3;	
	
	if( opc & BIT(24) ) 
	{
		base += (opc&BIT(23)) ? offs : -offs;
		if( opc & BIT(21) ) cpu.r[Rn] = base;
	} else {
		cpu.r[Rn] = base + ((opc&BIT(23)) ? offs : -offs);
	}
	
	if( opc & BIT(20) )
	{ // load
		cpu.r[Rd] = (u16)cpu.read(base, ((type==2)?8:16), ARM_CYCLE::N);
		if( type == 2 ) cpu.r[Rd] = (s32)(s8)cpu.r[Rd];
		else if( type == 3 ) cpu.r[Rd] = (s32)(s16)cpu.r[Rd];
		if( Rd == 15 ) cpu.flushp();
	} else {
		cpu.write(base, cpu.r[Rd] + ((Rd==15)?4:0), ((type==2)?8:16), ARM_CYCLE::N);
		cpu.next_cycle_type = ARM_CYCLE::N;
	}
}

void arm_dproc_and(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = (opc>>16)&15;
	u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = cpu.r[Rn] & oper2;
	if( Rd == 15 ) cpu.flushp();
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_eor(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = (opc>>16)&15;
	u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = cpu.r[Rn] ^ oper2;
	if( Rd == 15 ) cpu.flushp();
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_orr(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = (opc>>16)&15;
	u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = cpu.r[Rn] | oper2;
	if( Rd == 15 ) cpu.flushp();
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_mov(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = oper2;
	if( Rd == 15 ) cpu.flushp();
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_mvn(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = ~oper2;
	if( Rd == 15 ) cpu.flushp();
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_add(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = cpu.r[(opc>>16)&15];
	u32 Rd = (opc>>12)&15;
	u64 res = Rn;
	res += oper2;
	cpu.r[Rd] = res;
	if( Rd == 15 ) cpu.flushp();
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.C = (res>>32);
		cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_adc(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = cpu.r[(opc>>16)&15];
	u32 Rd = (opc>>12)&15;
	u64 res = Rn;
	res += oper2;
	res += cpu.cpsr.b.C;
	cpu.r[Rd] = res;
	if( Rd == 15 ) cpu.flushp();
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.C = (res>>32);
		cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_sbc(arm& cpu, u32 opc)
{
}

void arm_dproc_rsc(arm& cpu, u32 opc)
{
}

void arm_dproc_cmp(arm& cpu, u32 opc)
{
	u32 oper2 = ~((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = cpu.r[(opc>>16)&15];
	u32 Rd = (opc>>12)&15;
	u64 res = Rn;
	res += oper2;
	res += 1;
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.C = (res>>32);
		cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
		cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
		cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	}
}

void arm_dproc_cmn(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = cpu.r[(opc>>16)&15];
	u32 Rd = (opc>>12)&15;
	u64 res = Rn;
	res += oper2;
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.C = (res>>32);
		cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
		cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
		cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	}
}

void arm_dproc_bic(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = (opc>>16)&15;
	u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = cpu.r[Rn] & ~oper2;
	if( Rd == 15 ) cpu.flushp();
	
	if( opc & BIT(22) )
	{
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_tst(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = (opc>>16)&15;
	u32 Rd = (opc>>12)&15;
	u32 res = cpu.r[Rn] | oper2;
	
	cpu.cpsr.b.Z = ((res==0)?1:0);
	cpu.cpsr.b.N = ((res&BIT(31))?1:0);
}

void arm_dproc_sub(arm& cpu, u32 opc)
{
}

void arm_dproc_rsb(arm& cpu, u32 opc)
{
}

void arm_dproc_teq(arm& cpu, u32 opc)
{
}

void arm_psr_imm(arm& cpu, u32 opc)
{
}

void arm_psr_reg(arm& cpu, u32 opc)
{
}

arm7_instr arm7tdmi::decode_arm(u32 opc)
{
	if( (opc & 0x0f00'0000) == 0x0f00'0000 ) return arm_swi;
	if( (opc & 0x0e00'0000) == 0x0a00'0000 ) return arm_branch;
	if( (opc & 0x0e00'0000) == 0x0800'0000 ) return arm_block_data;
	if( (opc & 0x0e00'0010) == 0x0600'0010 ) return arm_undef;
	if( (opc & 0x0c00'0000) == 0x0400'0000 ) return arm_single_data;
	if( (opc & 0x0fff'fff0) == 0x012f'ff10 ) return arm_bx;
	if( (opc & 0x0fc0'00f0) == 0x0000'0090 ) return arm_mul;
	if( (opc & 0x0f80'00f0) == 0x0080'0090 ) return arm_mul_long;
	if( (opc & 0x0fb0'00f0) == 0x0100'0090 ) return arm_single_swap;
	if( (opc & 0x0e40'0f90) == 0x0000'0090 ) return arm_tranhw_reg;
	if( (opc & 0x0e40'0090) == 0x0040'0090 ) return arm_tranhw_imm;
	
	if( (opc & 0x0fb0'0000) == 0x0320'0000 ) return arm_psr_imm;
	if( (opc & 0x0f90'0ff0) == 0x0100'0000 ) return arm_psr_reg;
	
	switch( (opc>>21) & 15 )
	{
	case 0: return arm_dproc_and;
	case 1: return arm_dproc_eor;
	case 2: return arm_dproc_sub;
	case 3: return arm_dproc_rsb;
	case 4: return arm_dproc_add;
	case 5: return arm_dproc_adc;
	case 6: return arm_dproc_sbc;
	case 7: return arm_dproc_rsc;
	case 8: return arm_dproc_tst;
	case 9: return arm_dproc_teq;
	case 10:return arm_dproc_cmp;
	case 11:return arm_dproc_cmn;
	case 12:return arm_dproc_orr;
	case 13:return arm_dproc_mov;
	case 14:return arm_dproc_bic;
	case 15:return arm_dproc_mvn;
	default: break;	
	}
	printf("$%X: arm decode shouldn't get here opc = $%X\n", r[15]-8, opc);
	exit(1);
	return nullptr;
}


