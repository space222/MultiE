#include <cstdio>
#include <cstdlib>
#include <bit>
#include "arm7tdmi.h"

#define SBIT(a) ((a)&BIT(20))

u32 arm::barrelshifter(u32 opc, bool setc)
{
	const u32 opcode = opc;
	u64 Rm = r[opc&15];
	u32 VRm = Rm;
	if( (opc&0x10) && (opc&15) == 15 ) Rm += 4;
	const u32 shift_type = (opc>>5)&3;
	const u32 type = shift_type;
	const u32 shift_amount = ( (opc&BIT(4)) ? (r[(opc>>8)&15]&0xff) : ((opc>>7)&0x1f) );
	const u32 amt = shift_amount;
	if( (opc&0x10) && (shift_amount == 0) ) return Rm;
	
	u32 oldC = cpsr.b.C;
	u32 newC = oldC;
	
	if( type == 0 )
	{	// logical left
		if( amt == 0 ) return VRm;
		if( amt > 32 ) 
		{ 
			if( SBIT(opcode) ) cpsr.b.C = 0; 
			return 0; 
		}
		if( amt == 32 ) 
		{ 
			if( SBIT(opcode) ) cpsr.b.C = (VRm&1); 
			return 0; 
		}
		if( SBIT(opcode) ) cpsr.b.C = (VRm>>(32-amt))&1;
		return (VRm<<amt);
	}
	if( type == 1 )
	{	// logical right
		if( amt > 32 ) 
		{ 
			if( SBIT(opcode) ) cpsr.b.C = 0;
			return 0; 
		}
		if( amt == 32 || amt == 0 ) 
		{ 
			if( SBIT(opcode) ) cpsr.b.C = (VRm>>31)&1; 
			return 0; 
		}
		if( SBIT(opcode) ) cpsr.b.C = (VRm>>(amt-1))&1;
		return (VRm>>amt);
	}
	if( type == 2 )
	{	// arithmetic right
		if( amt == 0 || amt >= 32 ) 
		{ 
			if( SBIT(opcode) ) cpsr.b.C = (VRm>>31)&1; 
			return (VRm&(1ul<<31)) ? 0xffffFFFFul : 0; 
		}
		if( SBIT(opcode) ) cpsr.b.C = (VRm>>(amt-1))&1;
		return ( ((s32)VRm) >> amt );	
	}
	
	if( amt == 0 )
	{  // ROR #0 == RRX
		u32 temp = (cpsr.b.C ? 1 : 0);
		if( SBIT(opcode) ) cpsr.b.C = (VRm & 1);
		return (VRm>>1) | (temp<<31);	
	}	
	if( amt == 32 )
	{
		if( SBIT(opcode) ) cpsr.b.C = (VRm>>31)&1;
		return VRm;
	}
	u32 temp = std::rotr(VRm, amt);
	if( SBIT(opcode) ) cpsr.b.C = (temp>>31)&1;
	return temp;
	
	//if( setc && (opc & BIT(20)) ) cpsr.b.C = newC;
	//return Rm;
}

u32 arm::rotate_imm(u32 opc, bool setc)
{
	u32 R = (opc & 0x00000F00) >> 7;
	u32 val = (opc&0xff);
	if( R )
	{
		val = std::rotr(val, R);
		if( opc&BIT(20) ) cpsr.b.C = (val>>31)&1;
	}
	return val;
}

void arm_swi(arm& cpu, u32 opc)
{
	printf("ARM SWI\n");
	exit(1);
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

void arm_block_data(arm& cpu, u32 opcode)
{
printf("$%X: Arm block data opc\n", cpu.r[15]-8);
	cpu.dump_regs();
	cpu.icycles += 1; //always at least 1I
	ARM_CYCLE ct = ARM_CYCLE::N;
	u16 rlist = opcode;
	u32 Rn = (opcode>>16)&0xf;
	u32 base = cpu.r[Rn];
	u32 addr = base & ~3;
	s32 offset = (opcode & (1ul<<23)) ? 4 : -4;  // offset only used in final write-back calculation
	//bool use_base = (Rn == (u32)std::countr_zero(rlist));
	//printf("base = r%i = %X\n", Rn, base);
	if( !(opcode & (1ul<<23)) )
	{
		addr -= std::popcount(rlist)<<2;
		opcode ^= (1ul<<24);
	}
	//printf("corrected to %X, rlist=%X\n", addr, rlist);
	
	if( opcode & (1ul<<20) )
	{ //load
		for(u32 i = 0; i < 16; ++i)
		{
			if( !(rlist & (1u<<i)) ) continue;
			if( opcode & (1ul<<24) ) addr += 4;
			u32 val = cpu.read(addr, 32, ct);
			ct = ARM_CYCLE::S;
			cpu.r[i] = val;
			if( ! (opcode & (1ul<<24)) ) addr += 4;
		}
		if( (opcode & (1ul<<21)) && !(opcode & (1ul<<Rn)) )
		{
			addr = base + offset*(std::popcount(rlist));
			//printf("Final base = %X\n", addr);
			cpu.r[Rn] = addr;
		}
		if( rlist&0x8000 ) cpu.flushp();
	} else {
		//store
		for(u32 i = 0; i < 16; ++i)
		{
			if( !(rlist & (1u<<i)) ) continue;
			if( opcode & (1ul<<24) ) addr += 4;
			u32 val = cpu.r[i];
			if( i == 15 ) val += 4;
			//if( i == Rn && !use_base ) val = addr;
			//printf("multi-write! r(%i):", i);
			cpu.write(addr, val, 32, ct);
			ct = ARM_CYCLE::N;
			if( ! (opcode & (1ul<<24)) ) addr += 4;
		}
		if( (opcode & (1ul<<21)) )
		{
			addr = base + offset*(std::popcount(rlist));
			//printf("Final base = %X\n", addr);
			cpu.r[Rn] = addr;
		}
	}
	
	cpu.next_cycle_type = ARM_CYCLE::N;
	
}

void arm_undef(arm&, u32)
{
	printf("ARM7: explicit undef\n");
	exit(1);
}

void arm_single_data(arm& cpu, u32 opcode)
{
	cpu.icycles += 1;
	u32 Rd = (opcode>>12) & 0xf;
	u32 Rn = (opcode>>16) & 0xf;
	u32 offset = 0;
	if( (opcode & (1ul<<25)) )
	{
		u32 temp = cpu.cpsr.b.C;
		offset = cpu.barrelshifter(opcode&~0x10);
		cpu.cpsr.b.C = temp;
	} else {
		offset = (opcode & 0xfff);
	}
	u32 addr = cpu.r[Rn];
	
	if( opcode & (1ul<<20) )
	{	//load
		if( opcode & (1ul<<24) )
		{
			addr = (opcode&(1ul<<23)) ? (addr+offset) : (addr-offset);
			if( opcode & (1ul<<21) ) cpu.r[Rn] = addr;
		}

		if( opcode & (1ul<<22) )
		{ // byte
			cpu.r[Rd] = cpu.read(addr, 8, ARM_CYCLE::N);
		} else {
			// word
			u32 val = cpu.read(addr, 32, ARM_CYCLE::N);
			cpu.r[Rd] = val;
		}
		if( Rd == 15 ) cpu.flushp();
		
		if( Rn != Rd && !(opcode & (1ul<<24)) )
		{
			addr = (opcode&(1ul<<23)) ? (addr+offset) : (addr-offset);
			cpu.r[Rn] = addr;
		}
	} else {
		//store
		u32 val = cpu.r[Rd];
		if( Rd == 15 ) val += 4;
		
		if( opcode & (1ul<<24) )
		{
			addr = (opcode&(1ul<<23)) ? (addr+offset) : (addr-offset);
			if( opcode & (1ul<<21) ) 
			{
				cpu.r[Rn] = addr;
			}
		}
		
		if( opcode & (1ul<<22) )
		{ // byte
			cpu.write(addr, val, 8, ARM_CYCLE::N);
		} else {
			// word
			cpu.write(addr, val, 32, ARM_CYCLE::N);
		}
		
		if( !(opcode & (1ul<<24)) )
		{
			addr = (opcode&(1ul<<23)) ? (addr+offset) : (addr-offset);
			cpu.r[Rn] = addr;
		}
	}
}

void arm_bx(arm& cpu, u32 opc)
{
	cpu.r[15] = cpu.r[opc&15]&~1;
	//printf("$%X: ARM BX to $%X\n", opc, cpu.r[opc&15]);
	cpu.cpsr.b.T = cpu.r[opc&15]&1;
	cpu.flushp();
}

void arm_mul(arm& cpu, u32 opc)
{
	u32 &Rd = cpu.r[(opc>>16)&15];
	u32 Rm = cpu.r[opc&15];
	u32 Rs = cpu.r[(opc>>8)&15];
	u32 Rn = cpu.r[(opc>>12)&15];
	Rd = Rm * Rs;
	if( opc & BIT(21) ) 
	{
		Rd += Rn;
		cpu.icycles += 1;
	}
	if( opc & BIT(20) )
	{
		cpu.cpsr.b.Z = ((Rd==0)?1:0);
		cpu.cpsr.b.N = ((Rd&BIT(31))?1:0);	
	}
	if( (Rs&0xffFFff00) == 0 || (Rs&0xffFFff00) == 0xffFFff00 )
	{
		cpu.icycles += 1;
	} else if( (Rs&0xffFF0000) == 0 || (Rs&0xffFF0000) == 0xffFF0000 ) {
		cpu.icycles += 2;
	} else if( (Rs&0xff000000) == 0 || (Rs&0xff000000) == 0xff000000 ) {
		cpu.icycles += 3;
	} else {
		cpu.icycles += 4;
	}
}

void arm_mul_long(arm& cpu, u32 opc)
{
	u32 RdLo = (opc>>12)&15;
	u32 RdHi = (opc>>16)&15;
	u32 Rs = (opc>>8)&15;
	u32 Rm = opc&15;
	u64 res = 0;
	
	if( opc & BIT(22) )
	{ // signed
		s64 a = s64(s32(cpu.r[Rm]));
		a *= s64(s32(cpu.r[Rs]));
		if( opc & BIT(21) )
		{
			s64 b = (u64(cpu.r[RdHi])<<32)|u64(cpu.r[RdLo]);
			a += b;
		}
		res = a;	
	} else {
		u64 a =  cpu.r[Rm];
		a *= cpu.r[Rs];
		if( opc & BIT(21) )
		{
			u64 b = (u64(cpu.r[RdHi])<<32)|u64(cpu.r[RdLo]);
			a += b;
		}
		res = a;
	}
	
	cpu.r[RdHi] = res>>32;
	cpu.r[RdLo] = res;
			
	if( opc & BIT(20) )
	{
		cpu.cpsr.b.Z = ((res==0)?1:0);
		cpu.cpsr.b.N = ((res&BITL(63))?1:0);
	}
	
	cpu.icycles += 4; //todo: better timing
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
	u32 val = cpu.r[Rd] + ((Rd==15)?4:0);
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
		cpu.write(base, val, ((type==2)?8:16), ARM_CYCLE::N);
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
	u32 val = cpu.r[Rd] + ((Rd==15)?4:0);
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
		cpu.write(base, val, ((type==2)?8:16), ARM_CYCLE::N);
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
	
	if( opc & BIT(20) )
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
	
	if( opc & BIT(20) )
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
	
	if( opc & BIT(20) )
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
	if( Rd == 15 ) 
	{
		cpu.flushp();
	} else if( opc & BIT(20) ) {
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
	
	if( opc & BIT(20) )
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
	
	if( opc & BIT(20) )
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
	
	if( opc & BIT(20) )
	{
		cpu.cpsr.b.C = (res>>32);
		cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_sbc(arm& cpu, u32 opc)
{
	u32 oper2 = ~((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = cpu.r[(opc>>16)&15];
	u32 Rd = (opc>>12)&15;
	u64 res = Rn;
	res += oper2;
	res += cpu.cpsr.b.C;
	
	if( opc & BIT(20) )
	{
		cpu.cpsr.b.C = (res>>32);
		cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
		cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
		cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_rsc(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = ~cpu.r[(opc>>16)&15];
	u32 Rd = (opc>>12)&15;
	u64 res = Rn;
	res += oper2;
	res += cpu.cpsr.b.C;
	
	if( opc & BIT(20) )
	{
		cpu.cpsr.b.C = (res>>32);
		cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
		cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
		cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_cmp(arm& cpu, u32 opc)
{
	u32 oper2 = ~((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = cpu.r[(opc>>16)&15];
	//u32 Rd = (opc>>12)&15;
	u64 res = Rn;
	res += oper2;
	res += 1;
	
	
	
	cpu.cpsr.b.C = (res>>32);
	cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
	cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
	cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
}

void arm_dproc_cmn(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = cpu.r[(opc>>16)&15];
	u64 res = Rn;
	res += oper2;
	
	if( opc & BIT(20) )
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
	
	if( opc & BIT(20) )
	{
		cpu.cpsr.b.Z = ((cpu.r[Rd]==0)?1:0);
		cpu.cpsr.b.N = ((cpu.r[Rd]&BIT(31))?1:0);
	}
}

void arm_dproc_tst(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = (opc>>16)&15;
	//u32 Rd = (opc>>12)&15;
	u32 res = cpu.r[Rn] & oper2;
	
	cpu.cpsr.b.Z = ((res==0)?1:0);
	cpu.cpsr.b.N = ((res&BIT(31))?1:0);
}

void arm_dproc_sub(arm& cpu, u32 opc)
{
	u32 oper2 = ~((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = cpu.r[(opc>>16)&15];
	u32 Rd = (opc>>12)&15;
	u64 res = Rn;
	res += oper2;
	res += 1;
	
	if( opc & BIT(20) )
	{
		cpu.cpsr.b.C = (res>>32);
		cpu.cpsr.b.V = (((res^Rn)&(res^oper2)&BIT(31))?1:0);
		cpu.cpsr.b.Z = ((u32(res)==0)?1:0);
		cpu.cpsr.b.N = ((u32(res)&BIT(31))?1:0);
	}
	cpu.r[Rd] = res;
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_rsb(arm& cpu, u32 opc)
{
	printf("arm dproc RSB!\n");
	exit(1);
}

void arm_dproc_teq(arm& cpu, u32 opc)
{
	u32 oper2 = ((opc&BIT(25)) ? cpu.rotate_imm(opc) : cpu.barrelshifter(opc) );
	u32 Rn = (opc>>16)&15;
	u32 res = cpu.r[Rn] ^ oper2;
	
	cpu.cpsr.b.Z = ((res==0)?1:0);
	cpu.cpsr.b.N = ((res&BIT(31))?1:0);
}

void arm_psr(arm& cpu, u32 opc, u32 val)
{
	if( opc & BIT(21) )
	{  // psr = val
		if( opc & BIT(22) )
		{
			u32 S = cpu.getSPSR();
			if( opc & BIT(19) ) S = (S & 0xfffFFFF) | (val&0xf0000000u);
			if( opc & BIT(16) )
			{
				S &= ~0xff;
				S |= val&0xff;
			}
			cpu.setSPSR(S);		
		} else {
			if( opc & BIT(19) ) cpu.cpsr.v = (cpu.cpsr.v & 0xfffFFFF) | (val&0xf0000000u);
			if( opc & BIT(16) )
			{
				printf("would switch to mode $%X\n", val&0x1f);
				cpu.switch_to_mode(val&0x1f);
				//cpu.cpsr.b.T = cpu.cpsr.b.F = 0;
				//cpu.cpsr.v = val&0xe0;
			}	
		}	
	} else {
		// Rd = psr
		u32& Rd = cpu.r[(opc>>12)&15];
		Rd = ((opc&BIT(22)) ? cpu.getSPSR() : cpu.cpsr.v);	
	}
}

void arm_psr_imm(arm& cpu, u32 opc)
{
	arm_psr(cpu, opc, cpu.rotate_imm(opc));
}

void arm_psr_reg(arm& cpu, u32 opc)
{
	arm_psr(cpu, opc, cpu.r[opc&15]);
}

u32 arm::getSPSR()
{
	switch( cpsr.b.M )
	{
	case ARM_MODE_USER: return cpsr.v;
	case ARM_MODE_SYSTEM: return cpsr.v;
	case ARM_MODE_FIQ: return spsr_fiq;
	case ARM_MODE_ABORT: return spsr_abt;
	case ARM_MODE_IRQ: return spsr_irq;
	case ARM_MODE_SUPER: return spsr_svc;
	}
	printf("shouldn't happen getSPSR()\n");
	exit(1);
	return cpsr.v;
}

void arm::setSPSR(u32 v)
{
	switch( cpsr.b.M )
	{
	case ARM_MODE_USER: cpsr.v = v; return;
	case ARM_MODE_SYSTEM: cpsr.v = v; return;
	case ARM_MODE_FIQ: spsr_fiq = v; return; 
	case ARM_MODE_ABORT: spsr_abt = v; return;
	case ARM_MODE_IRQ: spsr_irq = v; return;
	case ARM_MODE_SUPER: spsr_svc = v; return;
	}
	printf("shouldn't happen setSPSR()\n");
	exit(1);
}

arm7_instr arm7tdmi::decode_arm(u32 opc)
{
	if( (opc & 0x0fff'fff0) == 0x012f'ff10 ) return arm_bx;
	if( (opc & 0x0f00'0000) == 0x0f00'0000 ) return arm_swi;
	if( (opc & 0x0e00'0000) == 0x0a00'0000 ) return arm_branch;
	if( (opc & 0x0e00'0000) == 0x0800'0000 ) return arm_block_data;
	if( (opc & 0x0e00'0010) == 0x0600'0010 ) return arm_undef;
	if( (opc & 0x0c00'0000) == 0x0400'0000 ) return arm_single_data;
	if( (opc & 0x0fc0'00f0) == 0x0000'0090 ) return arm_mul;
	if( (opc & 0x0f80'00f0) == 0x0080'0090 ) return arm_mul_long;
	if( (opc & 0x0fb0'00f0) == 0x0100'0090 ) return arm_single_swap;
	if( (opc & 0x0e40'0f90) == 0x0000'0090 ) return arm_tranhw_reg;
	if( (opc & 0x0e40'0090) == 0x0040'0090 ) return arm_tranhw_imm;
	
	//if( (opc & 0x0fb0'0000) == 0x0320'0000 ) return arm_psr_imm;
	//if( (opc & 0x0f90'0ff0) == 0x0100'0000 ) return arm_psr_reg;
	
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
	case 8: 
		if( !SBIT(opc) )  
		{
			return (opc&BIT(25)) ? arm_psr_imm : arm_psr_reg;		
		}
		return arm_dproc_tst;
	case 9: 
		if( !SBIT(opc) )  
		{
			return (opc&BIT(25)) ? arm_psr_imm : arm_psr_reg;		
		}
		return arm_dproc_teq;
	case 10:
		if( !SBIT(opc) )  
		{
			return (opc&BIT(25)) ? arm_psr_imm : arm_psr_reg;		
		}
		return arm_dproc_cmp;
	case 11:
		if( !SBIT(opc) )  
		{
			return (opc&BIT(25)) ? arm_psr_imm : arm_psr_reg;		
		}
		return arm_dproc_cmn;
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

void arm::dump_regs()
{
	for(u32 i = 0; i < 16; ++i) printf("\tr[%i] = $%08X\n", i, r[i]);
}

