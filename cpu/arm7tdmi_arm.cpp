#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <bit>
#include "arm7tdmi.h"

#define SBIT(a) ((a)&BIT(20))
#define DPROC_IMM (opc&BIT(25))

std::vector<std::string> disasm;

static inline void setnz(arm& cpu, u32 v)
{
	cpu.cpsr.b.Z = ((v==0)?1:0);
	cpu.cpsr.b.N = ((v&BIT(31))?1:0);
}

u32 rotimm(arm& cpu, u32 opc)
{
	u32 imm = opc&0xff;
	u32 amt = (opc>>7)&0x1e;
	if( amt )
	{
		imm = std::rotr(imm, amt);
		if( SBIT(opc) ) cpu.cpsr.b.C = imm>>31;
	}
	return imm;
}

u32 barrelshift(arm& cpu, u32 opc)
{
	const u32 type = (opc>>5)&3;
	const u32 Rm = opc&15;
	const u32 amt = ((opc&BIT(4)) ? (cpu.r[(opc>>8)&15]&0xff) : ((opc>>7)&0x1f));
	if( (opc&BIT(4)) && amt==0 ) return cpu.r[Rm];
	
	if( type == 0 )
	{ // lsl
 		if( amt == 0 ) return cpu.r[Rm];
		u64 v = cpu.r[Rm];
		v <<= amt;
		if( SBIT(opc) ) cpu.cpsr.b.C = v>>32;
		return v;	
	}
	if( type == 1 )
	{ // lsr
		if( amt == 0 || amt == 32 ) { if( SBIT(opc) ) { cpu.cpsr.b.C = cpu.r[Rm]>>31; } return 0; }
		if( amt > 32 )
		{
			if( SBIT(opc) ) cpu.cpsr.b.C = 0;
			return 0;
		}
		if( SBIT(opc) ) cpu.cpsr.b.C = cpu.r[Rm]>>(amt-1);
		return cpu.r[Rm]>>amt;	
	}
	if( type == 2 )
	{ // asr
		if( amt == 0 || amt > 31 ) { if( SBIT(opc) ) { cpu.cpsr.b.C = cpu.r[Rm]>>31; } return (cpu.r[Rm]&BIT(31))?0xffffFFFFu:0; }
		if( SBIT(opc) ) 
		{
			cpu.cpsr.b.C = cpu.r[Rm]>>(amt-1);
		}
		return s32(cpu.r[Rm])>>amt;
	}
	// ror
	if( amt == 0 )
	{ // rrx
		u32 oldC = cpu.cpsr.b.C;
		if( SBIT(opc) ) cpu.cpsr.b.C = cpu.r[Rm]&1;
		return (oldC<<31)|(cpu.r[Rm]>>1);
	}
	if( amt == 32 )
	{
		if( SBIT(opc) ) cpu.cpsr.b.C = cpu.r[Rm]>>31;
		return cpu.r[Rm];
	}
	u32 v = std::rotr(cpu.r[Rm], amt);
	if( SBIT(opc) ) cpu.cpsr.b.C = v>>31;
	return v;
}

void arm_bx(arm& cpu, u32 opc)
{
	u32 Rn = cpu.r[opc&15];
	cpu.r[15] = Rn&~1;
	cpu.cpsr.b.T = Rn&1;
	cpu.flushp();
}

void arm_block_data(arm& cpu, u32 opcode)
{
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
			u32 val = cpu.read(addr, 32, ARM_CYCLE::N);
			//printf("multi-read! r%i = (%X)=%X\n", i, addr, val);
			cpu.r[i] = val;
			if( ! (opcode & (1ul<<24)) ) addr += 4;
		}
		if( (opcode & (1ul<<21)) && !(opcode & (1ul<<Rn)) )
		{
			addr = base + offset*(std::popcount(rlist));
			//printf("Final base = %X\n", addr);
			cpu.r[Rn] = addr;
		}
		if( (rlist&0x8000) && (opcode & (1ul<<22)) )
		{
			//set_cpsr(cpu, getModeSpsr(cpu));				
		}
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
			cpu.write(addr, val, 32, ARM_CYCLE::N);
			if( ! (opcode & (1ul<<24)) ) addr += 4;
		}
		if( (opcode & (1ul<<21)) )
		{
			addr = base + offset*(std::popcount(rlist));
			//printf("Final base = %X\n", addr);
			cpu.r[Rn] = addr;
		}
	}
	
	return;
}

void arm_branch(arm& cpu, u32 opc)
{
	s32 offset = opc<<8;
	offset >>= 6;
	if( opc & BIT(24) ) cpu.r[14] = cpu.r[15] - 4;
	cpu.r[15] += offset;
	//printf("ARM7: Uncond branch to $%X\n", cpu.r[15]);
	cpu.flushp();
}

void arm_swi(arm& cpu, u32 opc)
{
	printf("arm7: swi unimpl.\n");
	exit(1);
}

void arm_undef(arm& cpu, u32 opc)
{
	printf("arm7: undef unimpl.\n");
	exit(1);
}

void arm_single_data(arm& cpu, u32 opc)
{
	s32 offset = ((opc&BIT(25)) ? barrelshift(cpu,(opc&~(BIT(20)|BIT(4)))) : (opc&0xfff));
	if( !(opc&BIT(23)) ) offset = -offset;
	const u32 Rn = (opc>>16)&15;
	const u32 Rd = (opc>>12)&15;
	u32 base = cpu.r[Rn];
	if( opc & BIT(24) ) base += offset;
		
	if( opc & BIT(20) )
	{ // load
		cpu.r[Rd] = cpu.read(base, (opc&BIT(22))?8:32, ARM_CYCLE::N);
		if( !(opc & BIT(24)) ) base += offset;
		if( Rd!=Rn && ((opc&BIT(21))||!(opc&BIT(24))) ) cpu.r[Rn] = base;
		if( Rd == 15 ) cpu.flushp();
	} else {
		cpu.write(base, cpu.r[Rd], (opc&BIT(22))?8:32, ARM_CYCLE::N);
		if( !(opc & BIT(24)) ) base += offset;
		if( ((opc&BIT(21))||!(opc&BIT(24))) ) cpu.r[Rn] = base;	
	}
}

void arm_swap(arm& cpu, u32 opc)
{
	cpu.icycles += 1;
	u32 Rm = opc&15;
	u32 Rn = (opc>>16)&15;
	u32 Rd = (opc>>12)&15;
	u32 VRm = cpu.r[Rm];
	u32 temp = cpu.read(cpu.r[Rn], (opc&BIT(22))?8:32, ARM_CYCLE::N);
	cpu.write(cpu.r[Rn], VRm, (opc&BIT(22))?8:32, ARM_CYCLE::N);
	cpu.r[Rd] = temp;
}

void arm_mult(arm& cpu, u32 opc)
{
	cpu.icycles += ((opc&BIT(21))?1:0);
	u32& Rd = cpu.r[(opc>>16)&15];
	u32 Rn = cpu.r[(opc>>12)&15];
	u32 Rm = cpu.r[opc&15];
	u32 Rs = cpu.r[(opc>>8)&15];
	
	Rd = (Rs * Rm) + ((opc&BIT(21)) ? Rn : 0);
	if( SBIT(opc) ) setnz(cpu, Rd);
	
	if( Rs >>= 8; Rs == 0 || Rs == 0xffFFff ) cpu.icycles += 1;
	else if( Rs >>= 8; Rs == 0 || Rs == 0xffFF ) cpu.icycles += 2;
	else if( Rs >>= 8; Rs == 0 || Rs == 0xff ) cpu.icycles += 3;
	else cpu.icycles += 4;
}

void arm_mult_long(arm& cpu, u32 opc)
{
	cpu.icycles += ((opc&BIT(21))?2:1);
	u32& RdHI = cpu.r[(opc>>16)&15];
	u32& RdLO = cpu.r[(opc>>12)&15];
	u64 accum = ((opc&BIT(21)) ? ((u64(RdHI)<<32)|RdLO) : 0);
	u32 Rs = cpu.r[(opc>>8)&15];
	u32 Rm = cpu.r[opc&15];
	if( opc & BIT(22) )
	{ // signed
		s64 t = s64(s32(Rs));
		t *= s64(s32(Rm));
		t += accum;
		if( SBIT(opc) )
		{
			cpu.cpsr.b.Z = ((t==0)?1:0);
			cpu.cpsr.b.N = ((t&BITL(63))?1:0);
		}
		RdHI = t>>32;
		RdLO = t;
		if( Rs >>= 8; Rs == 0 || Rs == 0xffFFff ) cpu.icycles += 1;
		else if( Rs >>= 8; Rs == 0 || Rs == 0xffFF ) cpu.icycles += 2;
		else if( Rs >>= 8; Rs == 0 || Rs == 0xff ) cpu.icycles += 3;
		else cpu.icycles += 4;
	} else {
		u64 t = Rs;
		t *= Rm;
		t += accum;
		if( SBIT(opc) )
		{
			cpu.cpsr.b.Z = ((t==0)?1:0);
			cpu.cpsr.b.N = ((t&BITL(63))?1:0);
		}
		RdHI = t>>32;
		RdLO = t;
		if( Rs >>= 8; Rs == 0 ) cpu.icycles += 1;
		else if( Rs >>= 8; Rs == 0 ) cpu.icycles += 2;
		else if( Rs >>= 8; Rs == 0 ) cpu.icycles += 3;
		else cpu.icycles += 4;
	}
}

void arm_half_data(arm& cpu, u32 opc)
{
	u32 type = (opc>>5)&3;
	s32 offset = ((opc&BIT(22)) ? (((opc>>4)&0xf0)|(opc&15)) : cpu.r[opc&15]);
	if( !(opc & BIT(23)) ) offset = -offset;
	const u32 Rn = (opc>>16)&15;
	const u32 Rd = (opc>>12)&15;
	u32 base = cpu.r[Rn];
	if( opc & BIT(24) ) base += offset;
	
	if( opc & BIT(20) )
	{ // load
		cpu.icycles += 1;
		cpu.r[Rd] = cpu.read(base, (type==2)?8:16, ARM_CYCLE::N);
		if( type == 2 ) cpu.r[Rd] = s32((s8)cpu.r[Rd]);
		else if( type == 3 ) cpu.r[Rd] = s32((s16)cpu.r[Rd]);
		else cpu.r[Rd] = u16(cpu.r[Rd]);
		
		if( !(opc & BIT(24)) ) base += offset;
		
		if( Rd!=Rn && ((opc&BIT(21))||!(opc&BIT(24))) ) cpu.r[Rn] = base;
		if( Rd == 15 ) cpu.flushp();
	} else {
		cpu.write(base, cpu.r[Rd], (type==2)?8:16, ARM_CYCLE::N);
		if( !(opc & BIT(24)) ) base += offset;
		if( ((opc&BIT(21))||!(opc&BIT(24))) ) cpu.r[Rn] = base;	
	}
}

void arm_psr(arm& cpu, u32 opc)
{
	printf("arm7: psr unimpl.\n");
	//exit(1);
}

void arm_dproc_and(arm& cpu, u32 opc)
{
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	cpu.r[Rd] = cpu.r[Rn] & op2;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			setnz(cpu, cpu.r[Rd]);
		}
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_eor(arm& cpu, u32 opc)
{
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	cpu.r[Rd] = cpu.r[Rn] ^ op2;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			setnz(cpu, cpu.r[Rd]);
		}
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_sub(arm& cpu, u32 opc)
{
	u32 op2 = ~(DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	u64 res = cpu.r[Rn];
	res += op2;
	res += 1;
	cpu.r[Rd] = res;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			cpu.cpsr.b.V = (((res^cpu.r[Rn])&(res^op2)&BIT(31))?1:0);
			cpu.cpsr.b.C = res>>32;
			setnz(cpu, res);
		}	
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_rsb(arm& cpu, u32 opc)
{
	printf("arm7: rsb unimpl.\n");
	exit(1);
}

void arm_dproc_add(arm& cpu, u32 opc)
{
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	u64 res = cpu.r[Rn];
	res += op2;
	cpu.r[Rd] = res;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			cpu.cpsr.b.V = (((res^cpu.r[Rn])&(res^op2)&BIT(31))?1:0);
			cpu.cpsr.b.C = res>>32;
			setnz(cpu, res);
		}	
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_adc(arm& cpu, u32 opc)
{
	u64 res = cpu.cpsr.b.C;
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	res += op2;
	res += cpu.r[Rn];
	cpu.r[Rd] = res;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			cpu.cpsr.b.V = (((res^cpu.r[Rn])&(res^op2)&BIT(31))?1:0);
			cpu.cpsr.b.C = res>>32;
			setnz(cpu, res);
		}	
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_sbc(arm& cpu, u32 opc)
{
	u64 res = cpu.cpsr.b.C;
	u32 op2 = ~(DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	res += op2;
	res += cpu.r[Rn];
	cpu.r[Rd] = res;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			cpu.cpsr.b.V = (((res^cpu.r[Rn])&(res^op2)&BIT(31))?1:0);
			cpu.cpsr.b.C = res>>32;
			setnz(cpu, res);
		}	
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_rsc(arm& cpu, u32 opc)
{
	u64 res = cpu.cpsr.b.C;
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	res += op2;
	res += ~cpu.r[Rn];
	cpu.r[Rd] = res;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			cpu.cpsr.b.V = (((res^~cpu.r[Rn])&(res^op2)&BIT(31))?1:0);
			cpu.cpsr.b.C = res>>32;
			setnz(cpu, res);
		}	
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_tst(arm& cpu, u32 opc)
{
	u32 op2 = ~(DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rn = (opc>>16)&15;
	u32 res = cpu.r[Rn] & op2;
	setnz(cpu, res);
}

void arm_dproc_teq(arm& cpu, u32 opc)
{
	u32 op2 = ~(DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rn = (opc>>16)&15;
	u32 res = cpu.r[Rn] ^ op2;
	setnz(cpu, res);
}

void arm_dproc_cmp(arm& cpu, u32 opc)
{
	u32 op2 = ~(DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rn = (opc>>16)&15;
	u64 res = cpu.r[Rn];
	res += op2;
	res += 1;
	cpu.cpsr.b.V = (((res^cpu.r[Rn])&(res^op2)&BIT(31))?1:0);
	cpu.cpsr.b.C = res>>32;
	setnz(cpu, res);
}

void arm_dproc_cmn(arm& cpu, u32 opc)
{
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rn = (opc>>16)&15;
	u64 res = cpu.r[Rn];
	res += op2;
	cpu.cpsr.b.V = (((res^cpu.r[Rn])&(res^op2)&BIT(31))?1:0);
	cpu.cpsr.b.C = res>>32;
	setnz(cpu, res);
}

void arm_dproc_orr(arm& cpu, u32 opc)
{
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	cpu.r[Rd] = cpu.r[Rn] | op2;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			setnz(cpu, cpu.r[Rd]);
		}
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_mov(arm& cpu, u32 opc)
{
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 rd = (opc>>12)&15;
	cpu.r[rd] = op2;
	if( SBIT(opc) )
	{
		if( rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			setnz(cpu, op2);
		}
	}
	if( rd == 15 ) cpu.flushp();
}

void arm_dproc_bic(arm& cpu, u32 opc)
{
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	u32 Rd = (opc>>12)&15;
	u32 Rn = (opc>>16)&15;
	cpu.r[Rd] = cpu.r[Rn] & ~op2;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			setnz(cpu, cpu.r[Rd]);
		}
	}
	if( Rd == 15 ) cpu.flushp();
}

void arm_dproc_mvn(arm& cpu, u32 opc)
{
	u32 op2 = (DPROC_IMM ? rotimm(cpu, opc) : barrelshift(cpu, opc));
	const u32 Rd = (opc>>12)&15;
	cpu.r[Rd] = ~op2;
	if( SBIT(opc) )
	{
		if( Rd == 15 )
		{
			cpu.setCPSR(cpu.getSPSR());
		} else {
			setnz(cpu, cpu.r[Rd]);
		}
	}
	if( Rd == 15 ) cpu.flushp();
}

arm7_instr arm7tdmi::decode_arm(u32 opc)
{
	u32 cc = ((opc>>16)&0xff0)|((opc>>4)&15);
	if( (cc & 0xfff) == 0x121 ) return arm_bx;
	if( (cc & 0xe00) == 0x800 ) return arm_block_data;
	if( (cc & 0xe00) == 0xa00 ) return arm_branch;
	if( (cc & 0xf00) == 0xf00 ) return arm_swi;
	if( (cc & 0xe01) == 0x601 ) return arm_undef;
	if( (cc & 0xc00) == 0x400 ) return arm_single_data;
	if( (cc & 0xfbf) == 0x109 ) return arm_swap;
	if( (cc & 0xfcf) == 0x009 ) return arm_mult;
	if( (cc & 0xf8f) == 0x089 ) return arm_mult_long;
	if( (cc & 0xe09) == 0x009 ) return arm_half_data;
	if( (cc & 0xD90) == 0x100 ) return arm_psr;
	
	switch( (opc >> 21) & 15 )
	{
	case 0: return arm_dproc_and;
	case 1:	return arm_dproc_eor;
	case 2: return arm_dproc_sub;
	case 3: return arm_dproc_rsb;
	case 4: return arm_dproc_add;
	case 5: return arm_dproc_adc;
	case 6: return arm_dproc_sbc;
	case 7:	return arm_dproc_rsc;
	case 8:	return arm_dproc_tst;
	case 9: return arm_dproc_teq;
	case 10:return arm_dproc_cmp;
	case 11:return arm_dproc_cmn;
	case 12:return arm_dproc_orr;
	case 13:return arm_dproc_mov;
	case 14:return arm_dproc_bic;
	case 15:return arm_dproc_mvn;
	}
	
	return nullptr;
}

void arm::setCPSR(u32 v)
{
}

u32 arm::getSPSR()
{
	switch( cpsr.b.M )
	{
	case ARM_MODE_SUPER: return spsr_svc;
	case ARM_MODE_IRQ: return spsr_irq;
	case ARM_MODE_FIQ: return spsr_fiq;
	case ARM_MODE_ABORT: return spsr_abt;	
	}
	return cpsr.v;
}


void arm::dump_regs()
{
	for(u32 i = 0; i < 16; ++i) printf("\tr[%i] = $%08X\n", i, r[i]);
	if( cpsr.b.T ) printf("Thumb\n"); else printf("Arm\n");
}

