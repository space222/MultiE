#include <cstdio>
#include <cstring>
#include <cstdlib>
//#include <SDL.h>
#include <SFML/Graphics.hpp>
#include <bit>
#include "types.h"

#define START_ADDR 0
//0x08000000

extern u16 IME;
extern u16 IE;
extern u16 IF;

u32 mem_read32(u32);
u16 mem_read16(u32);
u8 mem_read8(u32);
void mem_write32(u32, u32);
void mem_write16(u32, u16);
void mem_write8(u32, u8);
u32 code_read32(u32);
u16 code_read16(u32);
bool is_cond(const arm7&,u32);
void arm7_step_thumb(arm7&);
void arm7_swi(arm7&, u32);
void arm7_block_transfer(arm7&, u32);
void arm7_data_proc(arm7&, u32);
void arm7_mult(arm7&, u32);
void arm7_mult_long(arm7&, u32);
void arm7_single_swap(arm7&, u32);
void arm7_single_data(arm7&, u32);
void arm7_dumpregs(arm7&);

inline void setnz(arm7& cpu, u32 val)
{
	cpu.FLAG_N = ((val>>31)&1);
	cpu.FLAG_Z = (val==0);
	return;
}

u32 build_cpsr(arm7& cpu)
{
	cpu.cpsr &= 0x0fffFFFF;
	if( cpu.FLAG_N ) cpu.cpsr |= (1ul<<31);
	if( cpu.FLAG_Z ) cpu.cpsr |= (1ul<<30);
	if( cpu.FLAG_C ) cpu.cpsr |= (1ul<<29);
	if( cpu.FLAG_V ) cpu.cpsr |= (1ul<<28);
	return cpu.cpsr;
}

void set_cpsr(arm7& cpu, u32 val)
{
	cpu.FLAG_N = cpu.FLAG_Z = cpu.FLAG_V = cpu.FLAG_C = 0;
	cpu.cpsr = val;
	if( cpu.cpsr & (1ul<<31) ) cpu.FLAG_N = 1;
	if( cpu.cpsr & (1ul<<30) ) cpu.FLAG_Z = 1;
	if( cpu.cpsr & (1ul<<29) ) cpu.FLAG_C = 1;
	if( cpu.cpsr & (1ul<<28) ) cpu.FLAG_V = 1;
	return;
}

u32 getModeSpsr(arm7& cpu)
{
	u32 mode = cpu.cpsr&0x1f;
	
	switch( mode )
	{
	case MODE_SUPER: return cpu.spsr_svc;
	case MODE_IRQ: return cpu.spsr_irq;
	case MODE_FIQ: return cpu.spsr_fiq;
	default: return cpu.cpsr;
		//printf("getModeSpsr in unimpl. mode\n");
		//exit(1);
	}

	return cpu.cpsr;
}

void set_spsr(arm7& cpu, u32 val)
{
	u32 mode = cpu.cpsr&0x1f;
	
	switch( mode )
	{
	case MODE_SUPER: cpu.spsr_svc = val; return;
	case MODE_IRQ: cpu.spsr_irq = val; return;
	case MODE_FIQ: cpu.spsr_fiq = val; return;
	default:
		//printf("Set SPSR of unsupported mode = %X\n", mode);
		//exit(1);
		break;
	}

	return;
}

void setreg(arm7& cpu, u32 reg, u32 val)
{
	u32 mode = cpu.cpsr&0x1f;
	if( reg == 15 )
	{
		cpu.regs[15] = val;
		cpu.preset = true;
		return;
	}
	
	switch( mode )
	{
	case MODE_FIQ:
		if( reg < 8 ) cpu.regs[reg] = val;
		else cpu.fiq_regs[reg] = val;
		return;
	case MODE_SUPER:
		if( reg == 14 ) cpu.r14_svc = val;
		else if( reg == 13 ) cpu.r13_svc = val;
		else cpu.regs[reg] = val;
		return;
	case MODE_IRQ:
		if( reg == 14 ) cpu.r14_irq = val;
		else if( reg == 13 ) cpu.r13_irq = val;
		else cpu.regs[reg] = val;
		return;		
	case MODE_SYSTEM:
	case MODE_USER:
		cpu.regs[reg] = val;
		return;
	default:
		printf("Error: in invalid cpu mode = %X, cpsr = %X, @$%X\n", mode, cpu.cpsr, cpu.regs[15]);
		exit(1);
		return;
	}
	
	return;
}

u32 getreg(arm7& cpu, u32 reg)
{
	u32 mode = cpu.cpsr&0x1f;
	
	switch( mode )
	{
	case MODE_FIQ:
		if( reg < 8 || reg == 15 ) return cpu.regs[reg];
		return cpu.fiq_regs[reg];
	case MODE_SUPER:
		if( reg == 14 ) return cpu.r14_svc;
		else if( reg == 13 ) return cpu.r13_svc;
		return cpu.regs[reg];
	case MODE_IRQ:
		if( reg == 14 ) return cpu.r14_irq;
		else if( reg == 13 ) return cpu.r13_irq;
		return cpu.regs[reg];
	case MODE_SYSTEM:
	case MODE_USER:
		return cpu.regs[reg];
	default:
		printf("Error2: in invalid cpu mode = %X, cpsr=%X, @%X\n", mode, cpu.cpsr, cpu.regs[15]);
		exit(1);
		return 0;
	}
	
	return 0xcafebabe;
}

void arm7_halfword_regimm(arm7& cpu, u32 opcode)
{
	s32 offset = 0;
	if( opcode & (1ul<<22) )
	{
		offset = (s32)(((opcode>>4)&0xf0) | (opcode&0xf));
	} else {
		offset = (s32)getreg(cpu, (opcode&0xf) );
	}
	if( ! (opcode & (1ul<<23)) ) offset = -offset;
	
	u32 Rd = (opcode>>12)&0xf;
	u32 Rn = (opcode>>16)&0xf;
	u32 addr = getreg(cpu, Rn);
	
	if( opcode & (1ul<<24) )
	{
		addr += offset;
		if( opcode & (1ul<<21) ) setreg(cpu, Rn, addr);
	}
	
	u32 subop = (opcode>>5)&3;
	
	if( opcode & (1ul<<20) )
	{ //load
		switch( subop )
		{
		case 1: // ldrh
			setreg(cpu, Rd, mem_read16(addr));
			break;
		case 2: // ldrsb
			setreg(cpu, Rd, (s32)(s8)mem_read8(addr));
			break;
		case 3: // ldrsh
			setreg(cpu, Rd, (s32)(s16)mem_read16(addr));
			break;	
		default:
			printf("shouldn't ever happen in halfword transf.\n");
			exit(1);
			break;
		}	
	} else {
		//store
		u32 val = getreg(cpu, Rd);
		switch( subop )
		{
		case 1:
			mem_write16(addr, (Rd == 15) ? (val+4) : val);
			break;
		default:
			printf("shouldn't ever happen in halfword transf.\n");
			exit(1);
			break;		
		}	
	}
	
	if( ! (opcode & (1ul<<24)) ) setreg(cpu, Rn, addr+offset);
	
	return;
}

void arm7_step(arm7& cpu)
{
	if( !(cpu.cpsr&CPSR_IRQ) && (IME&1) && (IF&IE) )
	{
		cpu.spsr_irq = build_cpsr(cpu);
		if( cpu.cpsr & 0x20 )
		{
			cpu.r14_irq = cpu.regs[15] + (cpu.preset ? 4 : 2);
		} else {
			cpu.r14_irq = cpu.regs[15] + (cpu.preset ? 4 : 0);
		}
		cpu.cpsr &= ~0x3f;
		cpu.cpsr |= 0x80|MODE_IRQ;
		cpu.regs[15] = 0x18;
		cpu.preset = true;
	}
	
	if( cpu.cpsr & 0x20 )
	{
		arm7_step_thumb(cpu);
		return;
	}

	if( cpu.preset )
	{
		cpu.preset = false;
		cpu.prefetch = code_read32(cpu.regs[15]);
		cpu.regs[15] += 4;
	}
		
	u32 opcode = cpu.prefetch;
	cpu.prefetch = code_read32(cpu.regs[15]); 
	cpu.regs[15] += 4;

	u32 cond = (opcode>>28)&0xf;
	if( !is_cond(cpu, cond) )
	{
		return;
	}

	u32 top = (opcode>>25)&7;
	
	switch( top )
	{
	case 0: // so much			
		if( (opcode & 0x0fffFFF0) == 0x012fff10 )
		{	// branch and exchange (BX)
			u32 T = getreg(cpu, opcode&0xf);
			cpu.cpsr &= ~0x20;
			cpu.cpsr |= ((T&1)<<5);
			cpu.regs[15] = T&~1;
			cpu.preset = true;
			break;
		}
		if( (opcode & 0x90) != 0x90 )
		{
			arm7_data_proc(cpu, opcode);
			break;
		}
		if( opcode & 0x60 )
		{
			arm7_halfword_regimm(cpu, opcode);
			break;
		}
		switch( (opcode>>23) & 3 )
		{
		case 0:
			arm7_mult(cpu, opcode);
			break;
		case 1:
			arm7_mult_long(cpu, opcode);
			break;
		case 2:
			arm7_single_swap(cpu, opcode);
			break;
		case 3:
			printf("Error: no instruction there\n");
			exit(1);
			break;
		}
		break;
	case 1: // data proc
		arm7_data_proc(cpu, opcode);
		break;
	case 2: // single transfer
		arm7_single_data(cpu, opcode);
		break;
	case 3: // undef or single transfer
		if( (opcode & 0x10) )
		{
			//TODO: undef
			printf("ARM Undefined Opcode, PC=%X\n", cpu.regs[15]-8);
			exit(1);
			return;		
		}
		arm7_single_data(cpu, opcode);
		break;
	case 4: // block transfer
		arm7_block_transfer(cpu, opcode);
		break;
	case 5: // branch
		if( opcode & (1<<24) )
		{
			setreg(cpu, 14, cpu.regs[15] - 4);
		}
		opcode &= 0xffffff;
		if( opcode & (1<<23) ) opcode |= (0xff<<24);
		opcode <<= 2;
		setreg(cpu, 15, cpu.regs[15] + (s32)opcode);
		//printf("Branch to %X\n", cpu.regs[15]);
		break;	
	case 6: // unused coprocessor
		printf("Error2: no coprocessor\n");
		exit(1);
		break;
	case 7: // swi
		if( ! (opcode & (1<<24)) )
		{
			printf("Error1: no coprocessor\n");
			exit(1);
		}
		arm7_swi(cpu, opcode);
		break;
	default:
		printf("Unimpl. opcode category = %X\n", top);
		exit(1);
	}

	return;
}

void arm7_swi(arm7& cpu, u32 /* opcode */)
{
	//u32 swinum = (opcode>>16)&0xff;
	//printf("%X: ARM SWI %X\n", cpu.regs[15]-8, swinum);
	cpu.spsr_svc = build_cpsr(cpu);
	cpu.cpsr &= ~0x3f;
	cpu.cpsr |= 0x80|MODE_SUPER;
	setreg(cpu, 14, cpu.regs[15]-4);
	setreg(cpu, 15, 8);
	return;
}

void arm7_single_swap(arm7& cpu, u32 opcode)
{
	u32 Rm = (opcode & 0xF);
	u32 Rd = (opcode>>12)&0xf;
	u32 Rn = (opcode>>16)&0xf;
	u32 base = getreg(cpu, Rn);

	if( opcode & (1ul<<22) )
	{ // byte
		u32 val = mem_read8(base);
		mem_write8(base, getreg(cpu, Rm));	
		setreg(cpu, Rd, val);
	} else {
		// word
		u32 val = mem_read32(base);
		mem_write32(base, getreg(cpu, Rm));
		setreg(cpu, Rd, val);
	}
	return;
}

u32 getShift(arm7& cpu, u32 opcode)
{
	u32 type = (opcode>>5)&3;
	u32 Rm = (opcode&0xf);
	u32 val = getreg(cpu, Rm);
	if( (opcode&0x10) && Rm == 15 ) val += 4;
	u32 amt = ( opcode&0x10 ) ? (getreg(cpu, (opcode>>8)&0xf) & 0xff) : ((opcode>>7)&0x1f);
	if( (opcode&0x10) && (amt == 0) ) return val;
	u32 VRm = val;
/*
	u32 type = (opcode>>5)&3;
	u32 VRm = getreg(cpu, (opcode&0xf) );
	u32 amt = (opcode&0x10) ? (getreg(cpu, (opcode>>8)&0xf)&0xff) : ((opcode>>7)&0x1f);
	if( (opcode&0xf) == 0xf && (opcode&0x10) ) VRm += 4;
	if( (opcode&0x10) && amt == 0 ) return VRm;
	// suite.gba doens't like this adding 4 for PC?
*/
	if( type == 0 )
	{	// logical left
		if( amt == 0 ) return VRm;
		if( amt > 32 ) 
		{ 
			if( SBIT(opcode) ) cpu.FLAG_C = 0; 
			return 0; 
		}
		if( amt == 32 ) 
		{ 
			if( SBIT(opcode) ) cpu.FLAG_C = (VRm&1); 
			return 0; 
		}
		if( SBIT(opcode) ) cpu.FLAG_C = (VRm>>(32-amt))&1;
		return (VRm<<amt);
	}
	if( type == 1 )
	{	// logical right
		if( amt > 32 ) 
		{ 
			if( SBIT(opcode) ) cpu.FLAG_C = 0;
			return 0; 
		}
		if( amt == 32 || amt == 0 ) 
		{ 
			if( SBIT(opcode) ) cpu.FLAG_C = (VRm>>31)&1; 
			return 0; 
		}
		if( SBIT(opcode) ) cpu.FLAG_C = (VRm>>(amt-1))&1;
		return (VRm>>amt);
	}
	if( type == 2 )
	{	// arithmetic right
		if( amt == 0 || amt >= 32 ) 
		{ 
			if( SBIT(opcode) ) cpu.FLAG_C = (VRm>>31)&1; 
			return (VRm&(1ul<<31)) ? 0xffffFFFFul : 0; 
		}
		if( SBIT(opcode) ) cpu.FLAG_C = (VRm>>(amt-1))&1;
		return ( ((s32)VRm) >> amt );	
	}
	
	if( amt == 0 )
	{  // ROR #0 == RRX
		u32 temp = (cpu.FLAG_C ? 1 : 0);
		if( SBIT(opcode) ) cpu.FLAG_C = (VRm & 1);
		return (VRm>>1) | (temp<<31);	
	}	
	if( amt == 32 )
	{
		if( SBIT(opcode) ) cpu.FLAG_C = (VRm>>31)&1;
		return VRm;
	}
	u32 temp = std::rotr(VRm, amt);
	if( SBIT(opcode) ) cpu.FLAG_C = (temp>>31)&1;
	return temp;

}

void arm7_single_data(arm7& cpu, u32 opcode)
{
	u32 Rd = (opcode>>12) & 0xf;
	u32 Rn = (opcode>>16) & 0xf;
	u32 offset = 0;
	if( (opcode & (1ul<<25)) )
	{
		u32 temp = cpu.FLAG_C;
		offset = getShift(cpu, opcode&~0x10);
		cpu.FLAG_C = temp;
	} else {
		offset = (opcode & 0xfff);
	}
	u32 addr = getreg(cpu, Rn);
	
	if( opcode & (1ul<<20) )
	{	//load
		if( opcode & (1ul<<24) )
		{
			addr = (opcode&(1ul<<23)) ? (addr+offset) : (addr-offset);
			if( opcode & (1ul<<21) ) setreg(cpu, Rn, addr);
		}

		if( opcode & (1ul<<22) )
		{ // byte
			setreg(cpu, Rd, mem_read8(addr));
		} else {
			// word
			u32 val = mem_read32(addr);
			//printf("r%i = (%X)=$%X\n", Rd, addr, val);
			setreg(cpu, Rd, val);
		}
		
		if( Rn != Rd && !(opcode & (1ul<<24)) )
		{
			addr = (opcode&(1ul<<23)) ? (addr+offset) : (addr-offset);
			setreg(cpu, Rn, addr);	
		}
	} else {
		//store
		//bool delaywb = false;
		if( opcode & (1ul<<24) )
		{
			addr = (opcode&(1ul<<23)) ? (addr+offset) : (addr-offset);
			if( opcode & (1ul<<21) ) 
			{
			//	if( Rd == Rn ) delaywb = true;
			//} else {
				setreg(cpu, Rn, addr);
			}
		}
		
		u32 val = getreg(cpu, Rd);
		if( Rd == 15 ) val += 4;
			
		if( opcode & (1ul<<22) )
		{ // byte
			mem_write8(addr, val);
		} else {
			// word
			mem_write32(addr, val);
		}
		
		if( !(opcode & (1ul<<24)) ) //|| delaywb )
		{
			addr = (opcode&(1ul<<23)) ? (addr+offset) : (addr-offset);
			setreg(cpu, Rn, addr);	
		}
	}
	
	return;
}

void arm7_block_transfer(arm7& cpu, u32 opcode)
{
	u16 rlist = opcode;
	u32 Rn = (opcode>>16)&0xf;
	u32 base = getreg(cpu, Rn);
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
			u32 val = mem_read32(addr);
			//printf("multi-read! r%i = (%X)=%X\n", i, addr, val);
			if( (opcode&(1u<<22)) && !(rlist&0x8000) ) cpu.regs[i] = val;
			else setreg(cpu, i, val);
			if( ! (opcode & (1ul<<24)) ) addr += 4;
		}
		if( (opcode & (1ul<<21)) && !(opcode & (1ul<<Rn)) )
		{
			addr = base + offset*(std::popcount(rlist));
			//printf("Final base = %X\n", addr);
			setreg(cpu, Rn, addr);
		}
		if( (rlist&0x8000) && (opcode & (1ul<<22)) )
		{
			set_cpsr(cpu, getModeSpsr(cpu));				
		}
	} else {
		//store
		for(u32 i = 0; i < 16; ++i)
		{
			if( !(rlist & (1u<<i)) ) continue;
			if( opcode & (1ul<<24) ) addr += 4;
			u32 val = (opcode&(1u<<22)) ? cpu.regs[i] : getreg(cpu, i); 
			if( i == 15 ) val += 4;
			//if( i == Rn && !use_base ) val = addr;
			//printf("multi-write! r(%i):", i);
			mem_write32(addr, val);
			if( ! (opcode & (1ul<<24)) ) addr += 4;
		}
		if( (opcode & (1ul<<21)) )
		{
			addr = base + offset*(std::popcount(rlist));
			//printf("Final base = %X\n", addr);
			setreg(cpu, Rn, addr);
		}
	}
	
	return;
}

u32 getOp2(arm7& cpu, u32 opcode)
{
	if( opcode & (1ul<<25) )
	{
		u32 R = (opcode & 0x00000F00) >> 7;
		u32 val = (opcode&0xff);
		if( R )
		{
			val = std::rotr(val, R);
			if( opcode&(1u<<20) ) cpu.FLAG_C = (val>>31)&1;
		}
		return val;
	}

	return getShift(cpu, opcode);
}

void arm7_data_proc(arm7& cpu, u32 opcode)
{
	u32 C = (cpu.FLAG_C ? 1 : 0);
	u32 Rd = (opcode>>12)&0xf;
	u32 op1 = getreg(cpu , (opcode>>16)&0xf);
	if( ((opcode>>16)&0xf)==0xf && !(opcode&(1u<<25)) && (opcode&0x10) ) op1 += 4;
	u32 subop = (opcode>>21) & 0xf;
	
	if( subop > 7 && subop < 12 && !(opcode&(1ul<<20)) )
	{ 	// msr / mrs
		if( !(opcode&(1ul<<21)) )
		{
			u32 val = (opcode&(1ul<<22)) ? getModeSpsr(cpu) : build_cpsr(cpu);
			setreg(cpu, Rd, val);
		} else {
			u32 val = 0;
			if( (opcode&(1ul<<25)) )
			{
				val = std::rotr(opcode&0xff, ((opcode>>8)&0xf)*2);
			} else {
				val = getreg(cpu, opcode&0xf);		
			}
			u32 psr = (opcode&(1ul<<22)) ? getModeSpsr(cpu) : build_cpsr(cpu);
			if( (opcode&(1ul<<16)) )
			{
				psr &= ~0xff;
				psr |= (val&0xff);
				if( (psr & 0x1f) == 1 )
				{
					printf("Error: change to mode 1\n");
					arm7_dumpregs(cpu);
					exit(1);
				}				
			}
			if( (opcode&(1ul<<19)) )
			{
				psr &= 0xfffFFFFul;
				psr |= (val&0xF0000000ul);
			}
			if( (opcode&(1ul<<22)) ) set_spsr(cpu, psr);
			else set_cpsr(cpu, psr);
		}
		return;
	}
		
	switch( subop )
	{
	case 0:{ // and
		u32 op2 = op1 & getOp2(cpu, opcode);
		setreg(cpu, Rd, op2);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, op2);
			}
		}
		}break;
	case 1:{ // eor
		u32 op2 = op1 ^ getOp2(cpu, opcode);
		setreg(cpu, Rd, op2);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, op2);
			}
		}
		}break;
	case 0x2:{ // sub
		u64 op2 = ~getOp2(cpu, opcode);
		u64 res = op1;
		res += op2;
		res++;
		setreg(cpu, Rd, res);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, res);
				cpu.FLAG_C = (res>>32)&1;
				cpu.FLAG_V = (((res ^ op1) & (res ^ op2))>>31)&1;
			}
		}
		}break;
	case 0x3:{ // rsb
		op1 = ~op1; //u64 op1 = ~getreg(cpu , (opcode>>16)&0xf);
		u64 op2 = getOp2(cpu, opcode);
		u64 res = op2;
		res += op1;
		res++;
		setreg(cpu, Rd, res);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, res);
				cpu.FLAG_C = (res>>32)&1;
				cpu.FLAG_V = (((res ^ op1) & (res ^ op2))>>31)&1;
			}
		}
		}break;
	case 0x4:{ // add
		u64 op2 = getOp2(cpu, opcode);
		u64 res = op1;
		res += op2;
		setreg(cpu, Rd, res);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, res);
				cpu.FLAG_C = (res>>32)&1;
				cpu.FLAG_V = (((res ^ op1) & (res ^ op2))>>31)&1;
			}
		}
		}break;
	case 0x5:{ // adc
		u64 op2 = getOp2(cpu, opcode);
		u64 res = op1;
		res += op2;
		res += (C ? 1 : 0);
		setreg(cpu, Rd, res);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, res);
				cpu.FLAG_C = (res>>32)&1;
				cpu.FLAG_V = (((res ^ op1) & (res ^ op2))>>31)&1;
			}
		}
		}break;
	case 0x6:{ // sbc
		u64 op2 = ~getOp2(cpu, opcode);
		u64 res = op1;
		res += op2;
		res += C;
		setreg(cpu, Rd, res);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, res);
				cpu.FLAG_C = (res>>32)&1;
				cpu.FLAG_V = ((res ^ op1) & (res ^ op2) & (1ull<<31))>>31;
			}
		}
		}break;
	case 0x7:{ // rsc
		op1 = ~op1;
		u64 op2 = getOp2(cpu, opcode);
		u64 res = op2;
		res += op1;
		res += C;
		setreg(cpu, Rd, res);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, res);
				cpu.FLAG_C = (res>>32)&1;
				cpu.FLAG_V = ((res ^ op1) & (res ^ op2) & (1ull<<31))>>31;
			}
		}
		}break;
	case 8:{ // tst
		u32 op2 = op1 & getOp2(cpu, opcode);
		if( Rd == 15 )
		{
			set_cpsr(cpu, getModeSpsr(cpu));
		} else {
			setnz(cpu, op2);
		}
		}break;
	case 9:{ // teq
		u32 op2 = op1 ^ getOp2(cpu, opcode);
		if( Rd == 15 )
		{
			set_cpsr(cpu, getModeSpsr(cpu));
		} else {
			setnz(cpu, op2);
		}
		}break;
	case 0xA:{ // cmp
		u64 op2 = ~getOp2(cpu, opcode);
		u64 res = op1;
		res += op2;
		res++;
		if( Rd == 15 )
		{
			set_cpsr(cpu, getModeSpsr(cpu));
		} else {
			setnz(cpu, res);
			cpu.FLAG_C = (res>>32)&1;
			cpu.FLAG_V = ((res ^ op1) & (res ^ op2) & (1ull<<31))>>31;
		}
		}break;
	case 0xB:{ // cmn
		u64 op2 = getOp2(cpu, opcode);
		u64 res = op1;
		res += op2;
		if( Rd == 15 )
		{
			set_cpsr(cpu, getModeSpsr(cpu));
		} else {
			setnz(cpu, res);
			cpu.FLAG_C = (res>>32)&1;
			cpu.FLAG_V = ((res ^ op1) & (res ^ op2) & (1ull<<31))>>31;
		}
		}break;
	case 0xC:{ // orr
		u32 op2 = op1 | getOp2(cpu, opcode);
		setreg(cpu, Rd, op2);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, op2);
			}
		}
		}break;
	case 0xD:{ // mov
		u32 op2 = getOp2(cpu, opcode);
		setreg(cpu, Rd, op2);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, op2);
			}
		}
		}break;
	case 0xE:{ // bic
		u32 op2 = op1 & ~getOp2(cpu, opcode);
		setreg(cpu, Rd, op2);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, op2);
			}
		}
		}break;
	case 0xF:{ // mvn
		u32 op2 = ~getOp2(cpu, opcode);
		setreg(cpu, Rd, op2);
		if( SBIT(opcode) )
		{
			if( Rd == 15 )
			{
				set_cpsr(cpu, getModeSpsr(cpu));
			} else {
				setnz(cpu, op2);
			}
		}
		}break;			
	default:
		printf("Unimpl. data proc opc=%X\n", (opcode>>21) & 0xf);
		exit(1);
		return;
	}
	return;
}

void arm7_mult(arm7& cpu, u32 opcode)
{
	u32 Rd = (opcode>>16)&0xf;
	u32 Rs = (opcode>>8)&0xf;
	u32 Rm = (opcode & 0xf);
	u64 res = getreg(cpu, Rm);
	res *= (u64)getreg(cpu, Rs);
	if( opcode & (1ul<<21) )
	{
		res += (u64)getreg(cpu, (opcode>>12)&0xf);
	}
	setreg(cpu, Rd, (u32)res);
	if( opcode & (1ul<<20) )
	{
		setnz(cpu, (u32)res);
	}
	return;
}

void arm7_mult_long(arm7& cpu, u32 opcode)
{
	u32 Rm = opcode &0xf;
	u32 Rs = (opcode>>8)&0xf;
	u32 RdHi = (opcode>>16)&0xf;
	u32 RdLo = (opcode>>12)&0xf;
	u64 res = 0;
	u64 acc = 0;
	if( opcode & (1ul<<21) )
	{
		acc = getreg(cpu, RdHi);
		acc <<= 32;
		acc |= (u64)getreg(cpu, RdLo);
	}
	
	if( (opcode&(1ul<<22)) )
	{
		s64 temp = (s64)(s32)getreg(cpu, Rm);
		temp *= (s64)(s32)getreg(cpu, Rs);
		res = temp + (s64)acc;
	} else {
		res = (u64)(u32)getreg(cpu, Rm);
		res *= (u64)(u32)getreg(cpu, Rs);
		res += acc;
	}	
	
	setreg(cpu, RdHi, (u32)(res>>32));
	setreg(cpu, RdLo, (u32)res);
	if( opcode & (1ul<<20) )
	{
		cpu.FLAG_N = (res>>63)&1;
		cpu.FLAG_Z = (res==0);
	}	
	return;
}

void arm7_reset(arm7& cpu)
{
	for(int i = 0; i < 16; ++i) cpu.regs[i] = 0;
	cpu.regs[15] = START_ADDR;
	cpu.cpsr = 0x80|MODE_SUPER;
	cpu.preset = true;
	cpu.regs[13] = 0x03007F00;
	cpu.r13_svc = 0x03007FE0; 
	cpu.r13_irq =  0x03007FA0;
	cpu.spsr_svc = 0x9f;
	return;
}

bool is_cond(const arm7& cpu, u32 cond)
{
	switch(cond)
	{
	case 0: return cpu.FLAG_Z != 0;
	case 1: return cpu.FLAG_Z == 0;
	case 2: return cpu.FLAG_C != 0;
	case 3: return cpu.FLAG_C == 0;
	case 4: return cpu.FLAG_N != 0;
	case 5: return cpu.FLAG_N == 0;
	case 6: return cpu.FLAG_V != 0;
	case 7: return cpu.FLAG_V == 0;
	case 8: return cpu.FLAG_C != 0 && cpu.FLAG_Z == 0;
	case 9: return cpu.FLAG_C == 0 || cpu.FLAG_Z != 0;
	case 0xA: return (cpu.FLAG_N == 0 && cpu.FLAG_V == 0) || (cpu.FLAG_N != 0 && cpu.FLAG_V != 0);
	case 0xB: return (cpu.FLAG_N != 0 && cpu.FLAG_V == 0) || (cpu.FLAG_N == 0 && cpu.FLAG_V != 0);
	case 0xC: return cpu.FLAG_Z == 0 && ((cpu.FLAG_N == 0 && cpu.FLAG_V == 0) || (cpu.FLAG_N != 0 && cpu.FLAG_V != 0));
	case 0xD: return cpu.FLAG_Z != 0 || ((cpu.FLAG_N != 0 && cpu.FLAG_V == 0) || (cpu.FLAG_N == 0 && cpu.FLAG_V != 0));
	case 0xE: return true;
	case 0xF: return false;
	default:
		printf("Unimpl. opcode cond = %X\n", cond);
		exit(1);	
	}
	return true;
}

void arm7_dumpregs(arm7& cpu)
{
	for(int i = 0; i < 16; ++i)
	{
		printf("r%i = %X\t", i, getreg(cpu, i));
		if( (i+1)%4 == 0 ) printf("\n");	
	}
	printf("CPSR = %X\n", build_cpsr(cpu));
	return;
}






