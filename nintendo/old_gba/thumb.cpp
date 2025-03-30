#include <cstdio>
#include <cstring>
#include <cstdlib>
//#include <SDL.h>
#include <SFML/Graphics.hpp>
#include <bit>
#include "types.h"

extern bool debugger;
extern int debug_state;

u32 mem_read32(u32);
u16 mem_read16(u32);
u8 mem_read8(u32);
void mem_write32(u32, u32);
void mem_write16(u32, u16);
void mem_write8(u32, u8);
u32 code_read32(u32);
u16 code_read16(u32);
bool is_cond(const arm7&,u32);

static inline void setnz(arm7& cpu, u32 val)
{
	cpu.FLAG_N = ((val>>31)&1);
	cpu.FLAG_Z = (val==0);
	return;
}

u32 build_cpsr(arm7& cpu);
void set_cpsr(arm7& cpu, u32 val);
u32 getModeSpsr(arm7& cpu);
void set_spsr(arm7& cpu, u32 val);

void setreg(arm7& cpu, u32 reg, u32 val);
u32 getreg(arm7& cpu, u32 reg);

void thumb_swi(arm7& cpu, u32 opcode)
{	// thumb.17
	printf("THUMB SWI %X\n", (opcode&0xff));
	//if( (opcode&0xff)==0xb && ((cpu.regs[0]&0xf0000000) || (cpu.regs[1]&0xf0000000)) ) return; 
	// ^ cheap hack to fix Pokemon Ruby and FireRed. 
	// Ruby doesn't get thru the New Game prolog, afaik it never has.
	// FireRed startup intro fails. I don't know what I even changed recently that would make it fail
	cpu.spsr_svc = build_cpsr(cpu);
	cpu.cpsr &= ~0x3f;
	cpu.cpsr |= 0x80|MODE_SUPER;
	setreg(cpu, 14, cpu.regs[15]-2);
	setreg(cpu, 15, 8);
	return;
}

void thumb_uncond_branch(arm7& cpu, u16 opcode)
{	// thumb.18
	s16 offset = (opcode&0x7ff) << 5;
	offset >>= 4;
	setreg(cpu, 15, cpu.regs[15] + offset );
	return;
}

void thumb_long_branch(arm7& cpu, u16 opcode)
{	// thumb.19
	u32 PC = cpu.regs[15];
	u32 offs = (opcode&0x7ff);
	
	if( opcode & (1ul<<11) )
	{
		offs <<= 1;
		u32 LR = getreg(cpu, 14);
		LR += offs;
		setreg(cpu, 15, LR);  //TODO: clear/set bit0?
		setreg(cpu, 14, (PC-2)|1);
	} else {
		if( offs & (1ul<<10) )
		{
			offs |= 0xffffF800ul;
		}
		offs <<= 12;
		setreg(cpu, 14, PC + (s32)offs);
	}
	return;
}

void thumb_addsub(arm7& cpu, u16 opcode)
{	// thumb.2
	u32 Rd = opcode&7;
	u32 Rs = cpu.regs[(opcode>>3)&7];
	u32 Rn = (opcode>>6)&7;
	if( !(opcode & (1ul<<10)) ) Rn = cpu.regs[Rn];
	u64 res = Rs;
	
	if( opcode & (1ul<<9) )
	{
		Rn = ~Rn;
		res += Rn;
		res++;
	} else {
		res += Rn;
	}
	
	cpu.FLAG_C = (res>>32)&1;
	cpu.FLAG_V = (((res ^ Rs) & (res ^ Rn))>>31)&1;
	setnz(cpu, res);
	cpu.regs[Rd] = (u32)res;
	return;
}

void thumb_move_shifted(arm7& cpu, u16 opcode)
{	// thumb.1
	u32 type = (opcode>>11)&3;
	u32 Rs = (opcode>>3) & 7;
	u32 Rd = opcode&7;
	u32 amt = (opcode>>6)&0x1f;
	u32 val = cpu.regs[Rs];

	if( type == 0 )
	{ // LSL
		cpu.regs[Rd] = val << amt;
		if( amt ) cpu.FLAG_C = (val>>(32-amt))&1;	
		setnz(cpu, cpu.regs[Rd]);
	} else if( type == 1 ) { // LSR
		if( amt == 0 )
		{
			cpu.regs[Rd] = 0;
			cpu.FLAG_C = (val>>31)&1;
		} else {
			cpu.regs[Rd] = val >> amt;
			cpu.FLAG_C = (val>>(amt-1))&1;
		}
		setnz(cpu, cpu.regs[Rd]);
	} else { // ASR
		if( amt == 0 )
		{
			cpu.regs[Rd] = (val&(1ul<<31)) ? 0xffffFFFF : 0;
			cpu.FLAG_C = (val>>31)&1;
		} else {
			cpu.regs[Rd] = ((s32)val) >> amt;
			cpu.FLAG_C = (val>>(amt-1))&1;
		}
		setnz(cpu, cpu.regs[Rd]);
	}
	
	return;
}

void thumb_ldst_signed_bh(arm7& cpu, u16 opcode)
{	// thumb.8
	u32 subop = (opcode>>10)&3;
	u32 Rd = (opcode&7);
	u32 Ro = (opcode>>6)&7;
	u32 Rb = (opcode>>3)&7;
	u32 addr = cpu.regs[Rb] + cpu.regs[Ro];
	switch( subop )
	{
	case 0: // strh
		mem_write16(addr, cpu.regs[Rd]);
		break;
	case 1: // ldsb
		cpu.regs[Rd] = (s32)(s8)mem_read8(addr);
		break;
	case 2: // ldrh
		cpu.regs[Rd] = mem_read16(addr);
		if( addr & 1 ) cpu.regs[Rd] <<= 16;
		break;
	case 3: // ldrsh
		if( addr & 1 )
			cpu.regs[Rd] = (s32)(s8)mem_read8(addr);
		else
			cpu.regs[Rd] = (s32)(s16)mem_read16(addr);
		break;
	}	
	return;
}

void thumb_hireg_or_b(arm7& cpu, u16 opcode)
{	// thumb.5
	u32 subop = (opcode>>8)&3;
	u32 Rd = ((opcode>>4)&8) | (opcode&7);
	u32 Rs = (opcode>>3)&0xf;
	
	switch( subop )
	{
	case 0: // ADD
		setreg(cpu, Rd, getreg(cpu, Rd) + getreg(cpu, Rs));
		break;
	case 1:{ // CMP
		u32 VRd = getreg(cpu, Rd);
		u64 res = VRd;
		u32 val = ~getreg(cpu, Rs);
		res += val;
		res++;
		setnz(cpu, res);
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ VRd) & (res ^ val))>>31)&1;
		}break;
	case 2:{ // MOV
		u32 val = getreg(cpu, Rs);
		setreg(cpu, Rd, val);
		if( Rd < 8 && Rs < 8 )
		{
			setnz(cpu, val);
			//cpu.FLAG_C = cpu.FLAG_V = 0;
		}	
		}break;
	case 3:{ // BX
		u32 val = getreg(cpu, Rs);
		cpu.cpsr &= ~0x20;
		cpu.cpsr |= ((val&1)<<5);
		setreg(cpu, 15, val&~1);
		}break;
	default:
		printf("Unimpl. hireg_or_b subop = %i\n", subop);
		exit(1);
		break;
	}
	return;
}

void thumb_ldst_regoff(arm7& cpu, u16 opcode)
{	// thumb.7
	u32 Rd = opcode&7;
	u32 Rb = (opcode>>3)&7;
	u32 Ro = (opcode>>6)&7;
	u32 addr = cpu.regs[Rb] + cpu.regs[Ro];
	
	if( opcode & (1ul<<11) )
	{  // load
		if( opcode & (1ul<<10) )
		{ //byte
			cpu.regs[Rd] = mem_read8(addr);	
		} else { //word
			cpu.regs[Rd] = mem_read32(addr);
		}
	} else {
		// store
		if( opcode & (1ul<<10) )
		{ //byte
			mem_write8(addr, cpu.regs[Rd]);
		} else { //word
			mem_write32(addr, cpu.regs[Rd]);
		}
	}
	
	return;
}

void thumb_ldst_offs5(arm7& cpu, u16 opcode)
{	// thumb.9
	u32 Rd = opcode&7;
	u32 Rb = (opcode>>3)&7;
	u32 offs5 = (opcode>>6)&0x1f;
	u32 addr = cpu.regs[Rb];
	if( !(opcode&(1ul<<12)) )
	{
		offs5 <<= 2;
	}
	addr += offs5;
	
	if( opcode & (1ul<<11) )
	{  // load
		if( opcode & (1ul<<12) )
		{ //byte
			cpu.regs[Rd] = mem_read8(addr);
		} else { //word
			cpu.regs[Rd] = mem_read32(addr);
		}
	} else {
		// store
		if( opcode & (1ul<<12) )
		{ //byte
			mem_write8(addr, cpu.regs[Rd]);
		} else { //word
			mem_write32(addr, cpu.regs[Rd]);
		}
	}
	
	return;
}

void thumb_pcrel_load(arm7& cpu, u16 opcode)
{	// thumb.6
	u32 offset = (opcode&0xff);
	offset <<= 2;
	u32 Rd = (opcode>>8)&7;
	u32 addr = (cpu.regs[15]&~3) + offset;
	setreg(cpu, Rd, mem_read32(addr));
	return;
}

void thumb_alu(arm7& cpu, u16 opcode)
{	// thumb.4
	u32 subop = (opcode>>6)&0xf;
	u32 Rd = opcode&7;
	u32 VRd = cpu.regs[Rd];
	u32 VRs = cpu.regs[(opcode>>3)&7];

	switch( subop )
	{
	case 0: // AND
		cpu.regs[Rd] = VRd & VRs;
		setnz(cpu, cpu.regs[Rd]);
		break;
	case 1: // EOR
		cpu.regs[Rd] = VRd ^ VRs;
		setnz(cpu, cpu.regs[Rd]);
		break;
	case 2: // LSL
		VRs &= 0xFF;
		if( VRs ) 
		{
			if (VRs < 32) {
				cpu.FLAG_C = (cpu.regs[Rd] >> (32 - VRs)) & 1;
				cpu.regs[Rd] <<= VRs;
			} else {
				if (VRs > 32) {
					cpu.FLAG_C = 0;
				} else {
					cpu.FLAG_C = cpu.regs[Rd] & 1;
				}
				cpu.regs[Rd] = 0;
			}
		}
		setnz(cpu, cpu.regs[Rd]);
		break;
	case 3:{ // LSR
		VRs &= 0xFF;
		if (VRs) 
		{
			if (VRs < 32) {
				cpu.FLAG_C = (cpu.regs[Rd] >> (VRs - 1)) & 1;
				cpu.regs[Rd] = (uint32_t) cpu.regs[Rd] >> VRs;
			} else {
				if (VRs > 32) {
					cpu.FLAG_C = 0;
				} else {
					cpu.FLAG_C = (cpu.regs[Rd]>>31)&1;
				}
				cpu.regs[Rd] = 0;
			}
		}
		setnz(cpu, cpu.regs[Rd]);
		}break;
	case 4: // ASR
		VRs &= 0xff;
		if( VRs )
		{
			if( VRs < 32 )
			{
				cpu.FLAG_C = (cpu.regs[Rd] >> (VRs - 1))&1;
				cpu.regs[Rd] = ((s32)cpu.regs[Rd]) >> VRs;
			} else {
				cpu.FLAG_C = ((cpu.regs[Rd]>>31)&1);
				if( cpu.FLAG_C ) cpu.regs[Rd] = 0xffffFFFF;
				else cpu.regs[Rd] = 0;			
			}
		}
		setnz(cpu, cpu.regs[Rd]);
		break;	
	case 5:{ // ADC
		u32 A = VRd;
		u32 B = VRs;
		u64 res = A;
		res += B;
		res += (cpu.FLAG_C ? 1 : 0);
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ A) & (res ^ B))>>31)&1;
		cpu.regs[Rd] = res;
		setnz(cpu, cpu.regs[Rd]);
		}break;
	case 6:{ // SBC
		u32 A = VRd;
		u32 B = ~VRs;
		u64 res = A;
		res += B;
		res += (cpu.FLAG_C ? 1 : 0);
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ A) & (res ^ B))>>31)&1;
		cpu.regs[Rd] = res;
		setnz(cpu, cpu.regs[Rd]);
		}break;
	case 7:{ //ROR
		VRs &= 0xFF;
		if( VRs ) 
		{
			VRs &= 0x1F;
			if(VRs > 0) {
				cpu.FLAG_C = (cpu.regs[Rd] >> (VRs - 1)) & 1;
				cpu.regs[Rd] = std::rotr(cpu.regs[Rd], VRs);
			} else {
				cpu.FLAG_C = (cpu.regs[Rd]>>31)&1;
			}
		}
		setnz(cpu, cpu.regs[Rd]);
		}break;
	case 8: // TST
		setnz(cpu, VRd & VRs);
		break;
	case 9:{ // NEG
		u32 A = 0;
		u32 B = ~VRs;
		u64 res = A;
		res += B;
		res++;
		setnz(cpu, res);
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ A) & (res ^ B))>>31)&1;	
		cpu.regs[Rd] = res;
		}break;
	case 0xA:{ // CMP
		u64 A = VRd;
		u64 B = ~VRs;
		u64 res = A;
		res += B;
		res++;
		setnz(cpu, res);
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ A) & (res ^ B))>>31)&1;	
		}break;
	case 0xB:{ // CMN
		u32 A = VRd;
		u32 B = VRs;
		u64 res = A;
		res += B;
		setnz(cpu, res);
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ A) & (res ^ B))>>31)&1;	
		}break;
	case 0xC: // ORR
		cpu.regs[Rd] = VRd | VRs;
		setnz(cpu, cpu.regs[Rd]);
		break;
	case 0xD:{ // MUL
		u64 res = ((u64)VRd) * ((u64)VRs);
		cpu.regs[Rd] = res;
		setnz(cpu, cpu.regs[Rd]);
		}break;		
	case 0xE: // BIC
		cpu.regs[Rd] &= ~VRs;
		setnz(cpu, cpu.regs[Rd]);
		break;
	case 0xF: // MVN
		cpu.regs[Rd] = ~VRs;
		setnz(cpu, cpu.regs[Rd]);
		break;
	default:
		printf("Unimpl. cat 2 alu subop = %X\n", subop);
		exit(1);
	}
	return;
}

void thumb_loadrel_offs(arm7& cpu, u16 opcode)
{  	// thumb.12
	// this is a LEA-type instruction, it does NOT read memory
	u32 offset = (opcode&0xff)<<2;
	u32 Rd = (opcode>>8)&7;
	u32 temp = 0;
	if( opcode & (1ul<<11) )
		temp = getreg(cpu, 13);
	else
		temp = getreg(cpu, 15) & ~3;
	cpu.regs[Rd] = (temp + offset);
	return;
}

void thumb_ldst_half(arm7& cpu, u16 opcode)
{	// thumb.10
	u32 offs = (opcode>>6)&0x1f;
	u32 Rd = opcode&7;
	u32 addr = (offs<<1) + cpu.regs[(opcode>>3)&7];

	if( opcode & (1ul<<11) )
	{ //load
		cpu.regs[Rd] = mem_read16(addr);
		if( addr & 1 ) cpu.regs[Rd] <<= 16;	
	} else {
		//store
		mem_write16(addr, cpu.regs[Rd]);
	}
	return;
}

void thumb_ldst_sprel(arm7& cpu, u16 opcode)
{	// thumb.11
	u32 addr = getreg(cpu, 13) + ((opcode&0xff)<<2);
	u32 Rd = (opcode>>8)&7;

	if( opcode & (1ul<<11) )
	{ //load
		cpu.regs[Rd] = mem_read32(addr);
	} else {
		//store
		mem_write32(addr, cpu.regs[Rd]);
	}
	
	return;
}

void thumb_mcas(arm7& cpu, u16 opcode)
{	// thumb.3
	u32 subop = (opcode>>11)&3;
	u32 val = (opcode&0xff);
	u32 Rd = (opcode>>8)&7;
	
	if( subop == 0 )
	{ // mov
		cpu.regs[Rd] = val;
		setnz(cpu, cpu.regs[Rd]);
		//cpu.FLAG_C = cpu.FLAG_V = 0;
	} else if( subop == 1 ) { // cmp
		u64 A = cpu.regs[Rd];
		u64 B = ~val;
		u64 res = A;
		res += B;
		res++;
		setnz(cpu, res);
		cpu.FLAG_Z = ((u32)res)==0;
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ A) & (res ^ B))>>31)&1;
	} else if( subop == 2 ) { // ADD
		u32 A = cpu.regs[Rd];
		u32 B = val;
		u64 res = A;
		res += B;
		setnz(cpu, res);
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ A) & (res ^ B))>>31)&1;
		cpu.regs[Rd] = res;
	} else { // SUB
		u32 A = cpu.regs[Rd];
		u32 B = ~val;
		u64 res = A;
		res += B;
		res++;
		setnz(cpu, res);
		cpu.FLAG_C = (res>>32)&1;
		cpu.FLAG_V = (((res ^ A) & (res ^ B))>>31)&1;
		cpu.regs[Rd] = res;
	}

	return;
}

void thumb_ldst_mult(arm7& cpu, u16 opcode)
{	//thumb.15
	u32 Rn = (opcode>>8)&7;
	u32 base = cpu.regs[Rn];
	//printf("ldst mult, base(r%i) = %X\n", Rn, base);
	if( opcode & (1ul<<11) )
	{ // load
		if( !(opcode&0xff) )
		{ // empty rlist
			setreg(cpu, 15, mem_read32(base & ~3));
			setreg(cpu, Rn, base+0x40);	
			return;
		}
		for(u32 i = 0; i < 8; ++i)
		{
			if( opcode & (1ul<<i) )
			{
				cpu.regs[i] = mem_read32(base & ~3);
				base += 4;
			}		
		}
		if( !(opcode & (1ul<<Rn) ) ) setreg(cpu, Rn, base & ~3);
	} else { 
		//store
		if( !(opcode&0xff) )
		{ // empty rlist
			mem_write32(base & ~3, cpu.regs[15]+2);
			setreg(cpu, Rn, base+0x40);	
			return;
		}
		for(u32 i = 0; i < 8; ++i)
		{
			if( opcode & (1ul<<i) )
			{
				mem_write32(base & ~3, cpu.regs[i]);
				base += 4;
			}		
		}	
		cpu.regs[Rn] = base;
	}
	return;
}

void thumb_cond_branch(arm7& cpu, u16 opcode)
{	// thumb.16
	if( ! is_cond(cpu, (opcode>>8)&0xf) ) 
	{
		return;
	}
	s32 offset = (s32)(s8)(opcode&0xff);
	offset <<= 1;
	setreg(cpu, 15, getreg(cpu, 15) + (s32)offset);	
	return;
}

void thumb_add_sp(arm7& cpu, u16 opcode)
{	// thumb.13
	u32 val = (opcode&0x7f)<<2;
	u32 SP = getreg(cpu, 13);
	if( opcode & (1ul<<7) )
	{
		setreg(cpu, 13, SP-val);
	} else {
		setreg(cpu, 13, SP+val);
	}
	return;
}

void thumb_pushpop(arm7& cpu, u16 opcode)
{ 	// thumb.14	//TODO: making assumptions about order here
	u32 SP = getreg(cpu, 13);
	if( opcode & (1ul<<11) )
	{  // pop
		for(u32 i = 0; i < 8; ++i)
		{
			if( opcode & (1ul<<i) )
			{
				cpu.regs[i] = mem_read32(SP&~3);
				SP += 4;
			}
		}
		if( opcode & 0x100 )
		{
			setreg(cpu, 15, mem_read32(SP&~3) & ~1);
			SP += 4;
		}
	} else { // push
		if( opcode & 0x100 )
		{
			SP -= 4;
			mem_write32(SP&~3, getreg(cpu, 14));
		}
		for(u32 i = 0; i < 8; ++i)
		{
			if( opcode & (1ul<<(7-i)) )
			{
				SP -= 4;
				mem_write32(SP&~3, cpu.regs[7-i]);			
			}		
		}
	}
	setreg(cpu, 13, SP);
	return;
}

void arm7_step_thumb(arm7& cpu)
{
	u16 opcode = 0;
	cpu.regs[15] &= ~1;
	if( cpu.preset )
	{
		cpu.preset = false;
		cpu.prefetch = code_read16(cpu.regs[15]);
		cpu.regs[15] += 2;
	}
	
	opcode = cpu.prefetch;
	//if( debugger && debug_state != DEBUG_UNTIL ) 
	//	printf("%X Topcode = %X\n", cpu.regs[15]-2, opcode);
	cpu.prefetch = code_read16(cpu.regs[15]);
	cpu.regs[15] += 2;
	
	u32 top = (opcode>>13);
	if( top == 0 )
	{
		if( ((opcode>>11)&3) == 3 )
		{
			thumb_addsub(cpu, opcode);
		} else {
			thumb_move_shifted(cpu, opcode);
		}
		return;
	}
	
	if( top == 1 )
	{
		thumb_mcas(cpu, opcode);
		return;	
	}
	
	top = (opcode>>12);
	switch( top )
	{
	case 4:
		if( opcode & (1ul<<11) )
		{
			thumb_pcrel_load(cpu, opcode);		
			break;
		}
		if( opcode & (1ul<<10) )
		{
			thumb_hireg_or_b(cpu, opcode);
			break;
		}
		
		thumb_alu(cpu, opcode);
		break;
	case 5:
		if( opcode & (1ul<<9) )
		{
			thumb_ldst_signed_bh(cpu, opcode);			
		} else {
			thumb_ldst_regoff(cpu, opcode);
		}
		break;	
	case 6:
	case 7:
		thumb_ldst_offs5(cpu, opcode);
		break;
	case 8:
		thumb_ldst_half(cpu, opcode);
		break;
	case 9:
		thumb_ldst_sprel(cpu, opcode);
		break;
	case 0xA:
		thumb_loadrel_offs(cpu, opcode);
		break;
	case 0xB:
		if( opcode & (1ul<<10) )
		{
			thumb_pushpop(cpu, opcode);			
		} else {
			thumb_add_sp(cpu, opcode);
		}
		break;
	case 0xC:
		thumb_ldst_mult(cpu, opcode);
		break;
	case 0xD:
		if( (opcode&0x0f00) == 0x0f00 )
		{
			thumb_swi(cpu, opcode);
		} else {
			thumb_cond_branch(cpu, opcode);
		}
		break;
	case 0xE:
		thumb_uncond_branch(cpu, opcode);
		break;
	case 0xF:
		thumb_long_branch(cpu, opcode);
		break;
	}
	
	return;
}




