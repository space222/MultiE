#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include "itypes.h"
#include "68k.h"

#define se8to32(a) ((s32)(s8)(a))
#define se16to32(a) ((s32)(s16)(a))

#define EXC_ILLEGAL 0x10
#define EXC_DIVZERO 0x14
#define EXC_CHK     0x18
#define EXC_TRAPV   0x1C
#define EXC_PRIV    0x20
#define EXC_SPURIOUS  0x60
#define EXC_LEVEL_1   0x64
#define EXC_LEVEL_4   0x70
#define EXC_LEVEL_6   0x78

u32 debug_exec_count = 0;
int testno = 0;


/*
void mem_write8(u32, u8);
void mem_write16(u32, u16);
void mem_write32(u32, u32);
u8 mem_read8(u32);
u16 mem_read16(u32);
u32 mem_read32(u32);
*/

//u16 cpu.read_code16(u32);

typedef void (*m68k_instr)(m68k&, u16);

struct opcode
{
	u16 mask, res;
	m68k_instr i;
};

u32 size_mask[] = { 0xff, 0xff, 0xffff, 0xffff, 0xffffFFFF };

struct M
{
	u32 EA;
	u8 m, r, s;
	bool isReg, isImm;
	m68k& cpu;
	
	u32 value()
	{
		if( isImm ) return EA & size_mask[s];
		if( isReg ) return cpu.r[r] & size_mask[s];
		switch( s )
		{
		case 1: return cpu.mem_read8(EA);
		case 2: return cpu.mem_read16(EA);
		case 4: return cpu.mem_read32(EA);
		default: break;
		}
		printf("Error in 68k:value(), size = %i\n", s); 
		exit(1);
		return 0xbadf00d;
	}
	
	void set(u32 v)
	{
		if( isImm ) return;
		if( isReg )
		{
			if( r > 7 )
			{
				switch( s )
				{
				case 1: cpu.r[r] = se8to32(v); break;
				case 2: cpu.r[r] = se16to32(v); break;
				case 4: cpu.r[r] = v; break;
				}
				return;
			}
			switch( s )
			{
			case 1: cpu.regs[r].B = v; break;
			case 2: cpu.regs[r].W = v; break;
			case 4: cpu.regs[r].L = v; break;
			}
			return;
		}
		switch( s )
		{
		case 1: cpu.mem_write8(EA, v); return;
		case 2: cpu.mem_write16(EA, v); return;
		case 4: cpu.mem_write32(EA, v); return;
		default: break;		
		}
		printf("Error in 68k:set()\n"); 
		exit(1);
	}

	M(m68k& cp_u, u8 a_M, u8 Xn, int size = 4) : EA(0), m(0), r(0), s(4), isReg(false), isImm(false), cpu(cp_u)
	{
		m = a_M;
		s = size;
		if( m == 0 )
		{
			r = Xn;
			isReg = true;
			//printf("r = %i\n", Xn);
			return;
		}
		r = Xn + 8; 
		if( r == 15 ) r += (cpu.sr.b.S^1);
		//printf("m = %i\n", m);
		if( m == 1 )
		{
			isReg = true;
			return;
		}
		if( m == 2 )
		{
			EA = cpu.r[r];
			cpu.icycles += (s==4)?4:0;
			return;
		}
		if( m == 3 )
		{
			EA = cpu.r[r];
			cpu.r[r] += (( r >= 15 && size == 1 ) ? 2 : size);
			cpu.icycles += (s==4)?4:0;
			return;
		}
		if( m == 4 )
		{
			cpu.r[r] -= (( r >= 15 && size == 1 ) ? 2 : size);		
			EA = cpu.r[r];
			cpu.icycles += (s==4)?6:2;
			return;
		}
		if( m == 5 )
		{
			EA = cpu.r[r] + se16to32(cpu.read_code16(cpu.pc));
			cpu.pc += 2;
			cpu.icycles += (s==4)?8:4;
			return;
		}
		if( m == 6 )
		{
			u16 bew = cpu.read_code16(cpu.pc);
			cpu.pc += 2;
			EA = cpu.r[r] + se8to32((bew&0xff));
			u8 wl = (bew>>11)&1;
			bew >>= 12;
			if( bew == 15 ) bew += (cpu.sr.b.S^1);
			EA += (wl ? cpu.r[bew] : se16to32(cpu.r[bew]));
			cpu.icycles += (s==4)?12:8; // (s==4)?14:10;
			return;
		}
		// if we get here, m==7 and operation depends on Xn
		if( Xn == 0 )
		{
			EA = se16to32(cpu.read_code16(cpu.pc));
			cpu.pc += 2;
			cpu.icycles += (s==4)?8:4;
			return;
		}
		if( Xn == 1 )
		{
			EA = cpu.read_code16(cpu.pc)<<16;
			cpu.pc += 2;
			EA += cpu.read_code16(cpu.pc);
			cpu.pc += 2;
			cpu.icycles += (s==4)?12:8;
			return;
		}
		if( Xn == 2 )
		{
			EA = se16to32(cpu.read_code16(cpu.pc));
			EA += cpu.pc;
			cpu.pc += 2;
			cpu.icycles += (s==4)?8:4;
			return;		
		}
		if( Xn == 3 )
		{
			u16 bew = cpu.read_code16(cpu.pc);
			EA = cpu.pc + se8to32(bew);
			cpu.pc += 2;
			u8 wl = (bew>>11)&1;
			bew >>= 12;
			if( bew == 15 ) bew += (cpu.sr.b.S^1);
			EA += (wl ? cpu.r[bew] : se16to32(cpu.r[bew]));
			cpu.icycles += (s==4)?12:10;
			return;
		}
		if( Xn == 4 )
		{
			if( size == 4 )
			{
				EA = cpu.read_code16(cpu.pc)<<16;
				cpu.pc+=2;
				EA += cpu.read_code16(cpu.pc);
			} else {
				EA = cpu.read_code16(cpu.pc);
				if( size == 1 ) EA &= 0xff;
			}
			cpu.icycles += (s==4)?4:0;
			cpu.pc+=2;
			isImm = true;
			return;
		}
	}
};

bool isCond(m68k& cpu, u8 c)
{
	switch( c )
	{
	case 0: return true;	
	case 1: return false;
	case 2: return !cpu.sr.b.C && !cpu.sr.b.Z;
	case 3: return cpu.sr.b.C || cpu.sr.b.Z;
	case 4: return !cpu.sr.b.C;
	case 5: return cpu.sr.b.C;
	case 6: return !cpu.sr.b.Z;	
	case 7: return cpu.sr.b.Z;
	case 8: return !cpu.sr.b.V;	
	case 9: return cpu.sr.b.V;
	case 0xA: return !cpu.sr.b.N;	
	case 0xB: return cpu.sr.b.N;
	case 0xC: return (cpu.sr.b.N&&cpu.sr.b.V)||(!cpu.sr.b.N&&!cpu.sr.b.V);
	case 0xD: return (cpu.sr.b.N&&!cpu.sr.b.V)||(!cpu.sr.b.N&&cpu.sr.b.V);
	case 0xE: return !cpu.sr.b.Z && ((cpu.sr.b.N&&cpu.sr.b.V)||(!cpu.sr.b.N&&!cpu.sr.b.V));
	case 0xF: return cpu.sr.b.Z || ((cpu.sr.b.N&&!cpu.sr.b.V)||(!cpu.sr.b.N&&cpu.sr.b.V));	
	}
	printf("error in isCond, shouldn't get here, cond was $%X\n", c);
	exit(1);
}

u32 pop32(m68k& cpu)
{
	u32 res = cpu.mem_read32(cpu.r[15 + (cpu.sr.b.S^1)]);
	cpu.r[15 + (cpu.sr.b.S^1)] += 4;
	return res;
}

u16 pop16(m68k& cpu)
{
	u32 res = cpu.mem_read16(cpu.r[15 + (cpu.sr.b.S^1)]);
	cpu.r[15 + (cpu.sr.b.S^1)] += 2;
	return res;
}

void push32(m68k& cpu, u32 v)
{
	//printf("pushing $%X, onto stack at $%X\n", v, cpu.r[15 + (cpu.sr.b.S^1)]);
	cpu.r[15 + (cpu.sr.b.S^1)] -= 4;
	cpu.mem_write32(cpu.r[15 + (cpu.sr.b.S^1)], v);
}

void push16(m68k& cpu, u16 v)
{
	cpu.r[15 + (cpu.sr.b.S^1)] -= 2;
	cpu.mem_write16(cpu.r[15 + (cpu.sr.b.S^1)], v);
}

void m68k_trap(m68k& cpu, u32 vector, bool subtwo)
{
	u16 old_sr = cpu.sr.raw;
	cpu.sr.b.S = 1;
	cpu.sr.b.T = 0;
	push32(cpu, cpu.pc - (subtwo?2:0));
	push16(cpu, old_sr);
	cpu.pc = cpu.mem_read32(vector);
}

void ori_to_sr(m68k& cpu, u16) 
{
	if( ! cpu.sr.b.S )
	{
		m68k_trap(cpu, EXC_PRIV, true);
		cpu.icycles = 34;
		return;
	}
	cpu.icycles = 20;
	cpu.sr.raw |= cpu.read_code16(cpu.pc);
	cpu.pc += 2;
	cpu.sr.b.pad = 0;
	cpu.sr.b.pad2 = 0;
	cpu.sr.b.pad3 &= 2;
}

void ori_to_ccr(m68k& cpu, u16)
{
	cpu.icycles = 20;
	u8 val = cpu.read_code16(cpu.pc);
	cpu.pc += 2;
	cpu.sr.raw = (cpu.sr.raw&0xff00) | ((cpu.sr.raw|val)&0x1f);
}

int s1_to_size[] = { 1, 2, 4, 0 };
inline u32 imm_from_size(m68k& cpu, u8 s)
{
	u32 res = 0;
	switch( s )
	{
	case 0:
		res = cpu.read_code16(cpu.pc) & 0xff;
		cpu.pc += 2;
		return res;
	case 1:
		res = cpu.read_code16(cpu.pc);
		cpu.pc += 2;
		return res;
	case 2:
		res = cpu.read_code16(cpu.pc)<<16; cpu.pc += 2;
		res |=cpu.read_code16(cpu.pc); cpu.pc += 2;
		return res;
	default: break;
	}
	printf("Error in imm_from_size(), cpu.pc=%X, s=%i\n", cpu.pc-2, s);
	exit(1);
}

#define target() M t(cpu, (opc>>3)&7, opc&7, s1_to_size[S1(opc)] )
#define S1(a) (((a)>>6)&3)
#define imm() u32 imm = imm_from_size(cpu, S1(opc))

void ori(m68k& cpu, u16 opc)
{
	imm();
	target();
	imm |= t.value();
	cpu.sr.b.V = cpu.sr.b.C = 0;
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.Z = (imm&0xff)==0;
		cpu.sr.b.N = (imm&0x80)?1:0;
		break;
	case 1:
		cpu.sr.b.Z = (imm&0xffff)==0;
		cpu.sr.b.N = (imm&0x8000)?1:0;		
		break;
	default:
		cpu.sr.b.Z = imm==0;
		cpu.sr.b.N = (imm&(1u<<31))?1:0;
		break;
	}
	t.set(imm);
	return;
}

void eori_to_ccr(m68k& cpu, u16)
{
	cpu.icycles = 20;
	u8 val = cpu.read_code16(cpu.pc);
	cpu.pc += 2;
	cpu.sr.raw = (cpu.sr.raw&0xff00) | ((cpu.sr.raw^val)&0x1f);
}

void eori_to_sr(m68k& cpu, u16)
{
	if( ! cpu.sr.b.S )
	{
		m68k_trap(cpu, EXC_PRIV, true);
		cpu.icycles = 34;
		return;
	}
	cpu.icycles = 20;
	cpu.sr.raw ^= cpu.read_code16(cpu.pc);
	cpu.pc += 2;
	cpu.sr.b.pad = 0;
	cpu.sr.b.pad2 = 0;
	cpu.sr.b.pad3 &= 2;
}

void eori(m68k& cpu, u16 opc)
{
	imm();
	target();
	imm ^= t.value();
	cpu.sr.b.V = cpu.sr.b.C = 0;
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.Z = (imm&0xff)==0;
		cpu.sr.b.N = (imm&0x80)?1:0;
		break;
	case 1:
		cpu.sr.b.Z = (imm&0xffff)==0;
		cpu.sr.b.N = (imm&0x8000)?1:0;		
		break;
	default:
		cpu.sr.b.Z = imm==0;
		cpu.sr.b.N = (imm&(1u<<31))?1:0;
		break;
	}
	t.set(imm);
	return;
}

void cmpi(m68k& cpu, u16 opc)
{
	imm();
	target();
	u32 mask = size_mask[s1_to_size[S1(opc)]];
	u64 v = t.value() & mask;
	u64 R = (~imm) & mask;
	u64 res = v + R + 1;
		
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.C = ((res>>8)&1)^1;
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x80) ? 1 : 0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		break;
	case 1:
		cpu.sr.b.C = ((res>>16)&1)^1;
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x8000) ? 1 : 0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		break;
	default:
		cpu.sr.b.C = ((res>>32)&1)^1;
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & (1ul<<31)) ? 1 : 0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		break;
	}
}

void btst(m68k& cpu, u16 opc)
{
	u16 imm = cpu.read_code16(cpu.pc); cpu.pc += 2;
	//printf("btst #%i\n", imm);
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.m == 0 )
	{
		cpu.sr.b.Z = (cpu.r[t.r]&(1<<(imm&0x1F)))?0:1;
	} else {
		u8 val = t.value();
		cpu.sr.b.Z = (val&(1<<(imm&7)))?0:1;
	}
}

void btst2(m68k& cpu, u16 opc)
{
	u16 imm = cpu.r[(opc>>9)&7];
	//printf("btst #%i\n", imm);
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.m == 0 )
	{
		cpu.sr.b.Z = (cpu.r[t.r]&(1<<(imm&0x1F)))?0:1;
	} else {
		u8 val = t.value();
		cpu.sr.b.Z = (val&(1<<(imm&7)))?0:1;
	}
}

void bchg(m68k& cpu, u16 opc)
{
	u16 imm = cpu.read_code16(cpu.pc); cpu.pc += 2;
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.m == 0 )
	{
		cpu.sr.b.Z = (cpu.r[t.r]&(1<<(imm&0x1F)))?0:1;
		cpu.r[t.r] ^= (1<<(imm&0x1F));
	} else {
		u8 val = t.value();
		cpu.sr.b.Z = (val&(1<<(imm&7)))?0:1;
		val ^= (1<<(imm&7));
		t.set(val);	
	}	
}

void bchg2(m68k& cpu, u16 opc)
{
	u16 imm = cpu.r[(opc>>9)&7];
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.m == 0 )
	{
		cpu.sr.b.Z = (cpu.r[t.r]&(1<<(imm&0x1F)))?0:1;
		cpu.r[t.r] ^= (1<<(imm&0x1F));
	} else {
		u8 val = t.value();
		cpu.sr.b.Z = (val&(1<<(imm&7)))?0:1;
		val ^= (1<<(imm&7));
		t.set(val);	
	}
}

void bclr(m68k& cpu, u16 opc)
{
	u16 imm = cpu.read_code16(cpu.pc); cpu.pc += 2;
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.m == 0 )
	{
		cpu.sr.b.Z = (cpu.r[t.r]&(1<<(imm&0x1F)))?0:1;
		cpu.r[t.r] &= ~(1<<(imm&0x1F));
	} else {
		u8 val = t.value();
		cpu.sr.b.Z = (val&(1<<(imm&7)))?0:1;
		val &= ~(1<<(imm&7));
		t.set(val);	
	}
}

void bclr2(m68k& cpu, u16 opc)
{
	u16 imm = cpu.r[(opc>>9)&7];
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.m == 0 )
	{
		cpu.sr.b.Z = (cpu.r[t.r]&(1<<(imm&0x1F)))?0:1;
		cpu.r[t.r] &= ~(1<<(imm&0x1F));
	} else {
		u8 val = t.value();
		cpu.sr.b.Z = (val&(1<<(imm&7)))?0:1;
		val &= ~(1<<(imm&7));
		t.set(val);	
	}
}

void bset(m68k& cpu, u16 opc)
{
	u16 imm = cpu.read_code16(cpu.pc); cpu.pc += 2;
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.m == 0 )
	{
		cpu.sr.b.Z = (cpu.r[t.r]&(1<<(imm&0x1F)))?0:1;
		cpu.r[t.r] |= (1<<(imm&0x1F));
	} else {
		u8 val = t.value();
		cpu.sr.b.Z = (val&(1<<(imm&7)))?0:1;
		val |= (1<<(imm&7));
		t.set(val);	
	}
}

void bset2(m68k& cpu, u16 opc)
{
	u16 imm = cpu.r[(opc>>9)&7];
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.m == 0 )
	{
		cpu.sr.b.Z = (cpu.r[t.r]&(1<<(imm&0x1F)))?0:1;
		cpu.r[t.r] |= (1<<(imm&0x1F));
	} else {
		u8 val = t.value();
		cpu.sr.b.Z = (val&(1<<(imm&7)))?0:1;
		val |= (1<<(imm&7));
		t.set(val);	
	}
}

void addi(m68k& cpu, u16 opc)
{
	imm();
	target();
	u64 v = t.value() & size_mask[s1_to_size[S1(opc)]];
	u64 R = imm;
	u64 res = v + R;
	//printf("v($%lX) + R($%lX) = $%lX\n", v, R, res);
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.X = cpu.sr.b.C = (res>>8)&1;
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x80) ? 1 : 0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		break;
	case 1:
		cpu.sr.b.X = cpu.sr.b.C = (res>>16)&1;
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x8000) ? 1 : 0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		break;
	default:
		cpu.sr.b.X = cpu.sr.b.C = (res>>32)&1;
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & (1ul<<31)) ? 1 : 0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		break;
	}
	t.set(res);
}

void subi(m68k& cpu, u16 opc)
{
	imm();
	target();
	u64 v = t.value() & size_mask[s1_to_size[S1(opc)]];
	u64 R = (~imm) & size_mask[s1_to_size[S1(opc)]];
	u64 res = v + R + 1;
	//printf("v($%lX) + R($%lX) = $%lX\n", v, R, res);
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.X = cpu.sr.b.C = ((res>>8)&1)^1;
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x80) ? 1 : 0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		break;
	case 1:
		cpu.sr.b.X = cpu.sr.b.C = ((res>>16)&1)^1;
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x8000) ? 1 : 0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		break;
	default:
		cpu.sr.b.X = cpu.sr.b.C = ((res>>32)&1)^1;
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & (1ul<<31)) ? 1 : 0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		break;
	}
	t.set(res);
}

void andi(m68k& cpu, u16 opc)
{
	imm();
	target();
	cpu.sr.b.V = cpu.sr.b.C = 0;
	switch( (opc>>6)&3 )
	{
	case 0:
		imm = (imm&~0xff) | (imm&t.value()&0xff);
		cpu.sr.b.Z = (imm&0xff)==0;
		cpu.sr.b.N = (imm&0x80)?1:0;
		break;
	case 1:
		imm = (imm&~0xffff) | (imm&t.value()&0xffff);
		cpu.sr.b.Z = (imm&0xffff)==0;
		cpu.sr.b.N = (imm&0x8000)?1:0;		
		break;
	default:
		imm &= t.value();
		cpu.sr.b.Z = imm==0;
		cpu.sr.b.N = (imm&(1u<<31))?1:0;
		break;
	}
	t.set(imm);
	return;
}

void andi_to_ccr(m68k& cpu, u16)
{
	cpu.icycles = 20;
	u8 val = cpu.read_code16(cpu.pc);
	cpu.pc += 2;
	cpu.sr.raw = (cpu.sr.raw&0xff00) | (cpu.sr.raw&val&0xff);
}

void andi_to_sr(m68k& cpu, u16)
{
	if( ! cpu.sr.b.S )
	{
		m68k_trap(cpu, EXC_PRIV, true);
		cpu.sr.b.T = 0; //???
		cpu.icycles = 34;
		return;
	}
	cpu.icycles = 20;
	u16 imm = cpu.read_code16(cpu.pc); 
	cpu.pc += 2;
	cpu.sr.raw &= imm;
	cpu.sr.b.pad = 0;
	cpu.sr.b.pad2 = 0;
	cpu.sr.b.pad3 = 0;
}

void movep(m68k& cpu, u16 opc)
{
	cpu.icycles = 16;
	u32 An = 8 + (opc&7);
	if( An == 15 ) An += cpu.sr.b.S^1;
	An = cpu.r[An] + se16to32(cpu.read_code16(cpu.pc));
	cpu.pc += 2;
	u8 Dn = (opc>>9)&7;
	u8 mode = S1(opc);
	
	if( mode == 0 )
	{
		u8 hi = cpu.mem_read8(An);
		An += 2;
		u8 lo = cpu.mem_read8(An);
		cpu.regs[Dn].W = (hi<<8)|lo;
	} else if( mode == 1 ) {
		u8 bytes[4];
		cpu.icycles += 8;
		bytes[3] = cpu.mem_read8(An); An += 2;
		bytes[2] = cpu.mem_read8(An); An += 2;
		bytes[1] = cpu.mem_read8(An); An += 2;
		bytes[0] = cpu.mem_read8(An);
		memcpy(&cpu.r[Dn], &bytes, 4);
	} else if( mode == 2 ) {
		cpu.mem_write8(An, cpu.regs[Dn].W>>8); 
		An += 2;
		cpu.mem_write8(An, cpu.regs[Dn].W);
	} else {
		cpu.icycles += 8;
		cpu.mem_write8(An, cpu.r[Dn]>>24); An += 2;
		cpu.mem_write8(An, cpu.r[Dn]>>16); An += 2;
		cpu.mem_write8(An, cpu.r[Dn]>>8); An += 2;
		cpu.mem_write8(An, cpu.r[Dn]);
	}
}

void movea(m68k& cpu, u16 opc)
{
	u8 sz = (opc&(1<<12)) ? 2 : 4;
	M t(cpu, (opc>>3)&7, opc&7, sz);
	
	u8 r = 8 + ((opc>>9)&7);
	if( r == 15 ) r += cpu.sr.b.S^1;
	
	cpu.r[r] = (sz==2) ? (s32)(s16)t.value() : t.value();
}

void move(m68k& cpu, u16 opc)
{
	u8 sz = (opc>>12)&3;
	if( sz == 2 ) sz = 4;
	else if( sz == 3 ) sz = 2;
	
	M src(cpu, (opc>>3)&7, opc&7, sz);
	u32 v = src.value();
	
	M dest(cpu, (opc>>6)&7, (opc>>9)&7, sz);
	//printf("opc = $%X, sz = %i, dest = $%X(%i)\n", opc, sz, dest.EA&0xffFFff, dest.EA&0xffFFff);
	dest.set(v);
	cpu.sr.b.C = cpu.sr.b.V = 0;
	switch( sz )
	{
	case 1:
		cpu.sr.b.Z = ((v&0xff)==0);
		cpu.sr.b.N = (v&0x80)?1:0;
		break;
	case 2:
		cpu.sr.b.Z = (v&0xffff)==0;
		cpu.sr.b.N = (v&0x8000)?1:0;
		break;
	default:
		cpu.sr.b.Z = v==0;
		cpu.sr.b.N = (v&(1u<<31))?1:0;
		break;
	}
}

void move_from_sr(m68k& cpu, u16 opc)
{
	// not priviledged?
	M t(cpu, (opc>>3)&7, opc&7, 2);
	cpu.sr.b.pad = 0;
	cpu.sr.b.pad2 = 0;
	cpu.sr.b.pad3 = 0;
	t.set(cpu.sr.raw);
}

void move_to_ccr(m68k& cpu, u16 opc)
{
	cpu.icycles += 8;
	M t(cpu, (opc>>3)&7, opc&7, 2);
	cpu.sr.raw &= 0xff00;
	cpu.sr.raw |= t.value()&0x1F;
}

void move_to_sr(m68k& cpu, u16 opc)
{
	if( ! cpu.sr.b.S )
	{
		m68k_trap(cpu, EXC_PRIV, true);
		cpu.icycles = 34;
		return;
	}
	cpu.icycles += 8;	
	M t(cpu, (opc>>3)&7, opc&7, 2);
	cpu.sr.raw = t.value();
	//printf("SR = $%X\n", cpu.sr.raw);
	cpu.sr.b.pad = 0;
	cpu.sr.b.pad2 = 0;
	cpu.sr.b.pad3 &= 2;
}

void clr(m68k& cpu, u16 opc)
{
	target();
	t.value(); // original 68k issues a read before clearing
	t.set(0);
	cpu.sr.b.Z = 1;
	cpu.sr.b.C = cpu.sr.b.N = cpu.sr.b.V = 0;
}

void op_not(m68k& cpu, u16 opc)
{
	target();
	u32 v = ~t.value();
	cpu.sr.b.V = cpu.sr.b.C = 0;
	if( !t.isReg ) cpu.icycles += 8;
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.N = (v&0x80) ? 1 : 0;
		cpu.sr.b.Z = (v&0xff)==0;
		break;
	case 1:
		cpu.sr.b.N = (v&0x8000) ? 1 : 0;
		cpu.sr.b.Z = (v&0xffff)==0;
		break;
	case 2:
		cpu.icycles += 2;
		cpu.sr.b.N = (v&(1u<<31)) ? 1 : 0;
		cpu.sr.b.Z = v==0;
		break;	
	}
	t.set(v);
}

void neg(m68k& cpu, u16 opc)
{
	target();
	u32 v = t.value();
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.V = ((v&0xff)==0x80);
		v = (v&~0xff) | ((0-(v&0xff))&0xff);
		cpu.sr.b.N = (v&0x80) ? 1 : 0;
		cpu.sr.b.Z = (v&0xff)==0;
		cpu.sr.b.C = cpu.sr.b.X = (cpu.sr.b.Z^1);
		break;
	case 1:
		cpu.sr.b.V = ((v&0xffff)==0x8000);
		v = (v&~0xffff) | ((0-(v&0xffff))&0xffff);
		cpu.sr.b.N = (v&0x8000) ? 1 : 0;
		cpu.sr.b.Z = (v&0xffff)==0;
		cpu.sr.b.C = cpu.sr.b.X = (cpu.sr.b.Z^1);
		break;
	case 2:
		cpu.sr.b.V = (v==(1u<<31));
		v = 0-v;
		cpu.sr.b.N = (v&(1u<<31)) ? 1 : 0;
		cpu.sr.b.Z = v==0;
		cpu.sr.b.C = cpu.sr.b.X = (cpu.sr.b.Z^1);
		break;	
	}
	t.set(v);
}

void nbcd(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 1);
	if( t.isReg ) cpu.icycles += 2;
	else cpu.icycles += 8;
	
	// copied from: https://gendev.spritesmind.net/forum/viewtopic.php?t=1964
	u8 src = t.value();
	uint8_t dd = - src - cpu.sr.b.X;
	// Normal carry computation for subtraction:
	// (sm & ~dm) | (rm & ~dm) | (sm & rm)
	// but simplified because dm = 0 and ~dm = 1 for 0:
	uint8_t bc = (src | dd) & 0x88;
	uint8_t corf = bc - (bc >> 2);
	uint8_t rr = dd - corf;
	// Compute flags.
	// Carry has two parts: normal carry for subtraction
	// (computed above) OR'ed with normal carry for
	// subtraction with corf:
	// (sm & ~dm) | (rm & ~dm) | (sm & rm)
	// but simplified because sm = 0 and ~sm = 1 for corf:
	cpu.sr.b.X = cpu.sr.b.C = (bc | (~dd & rr)) >> 7;
	// Normal overflow computation for subtraction with corf:
	// (~sm & dm & ~rm) | (sm & ~dm & rm)
	// but simplified because sm = 0 and ~sm = 1 for corf:
	cpu.sr.b.V = (dd & ~rr) >> 7;
	// Accumulate zero flag:
	cpu.sr.b.Z = cpu.sr.b.Z & (rr == 0);
	cpu.sr.b.N = rr >> 7;
	//return rr;
	t.set(rr);
}

void pea(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 4);
	push32(cpu, t.EA);
	cpu.icycles += 4;
	//printf("PEA $%X\n", t.EA);
}

void swap(m68k& cpu, u16 opc)
{
	opc &= 7;
	cpu.r[opc] = (cpu.r[opc]<<16)|(cpu.r[opc]>>16);
	cpu.sr.b.C = cpu.sr.b.V = 0;
	cpu.sr.b.Z = (cpu.r[opc] == 0);
	cpu.sr.b.N = (cpu.r[opc]&(1u<<31)) ? 1 : 0;
}

void ext(m68k& cpu, u16 opc)
{
	u8 mode = (opc>>6)&7;
	u8 r = opc&7;
	cpu.sr.b.C = cpu.sr.b.V = 0;
	
	if( mode == 0b010 )
	{
		cpu.regs[r].W = (s16)(s8)cpu.regs[r].B;
		cpu.sr.b.Z = (cpu.regs[r].W==0);
		cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
	} else if( mode == 0b011 ) {
		cpu.regs[r].L = (s32)(s16)cpu.regs[r].W;
		cpu.sr.b.Z = (cpu.regs[r].L==0);
		cpu.sr.b.N = (cpu.regs[r].L&(1u<<31))?1:0;
	}
}

void illegal(m68k& cpu, u16)
{
	m68k_trap(cpu, EXC_ILLEGAL, true);
}

void reset(m68k& cpu, u16)
{
	if( ! cpu.sr.b.S )
	{
		cpu.icycles = 34;
		m68k_trap(cpu, EXC_PRIV, true);
		return;
	}
	cpu.icycles = 132;
	//todo: doing nothing passes the test, but ???
	//cpu.pc = cpu.mem_read32(4);
	//cpu.r[15] = cpu.mem_read32(0);
}

void nop(m68k&, u16) {}

void stop(m68k& cpu, u16)
{
	if( ! cpu.sr.b.S )
	{
		m68k_trap(cpu, EXC_PRIV, true);
		return;
	}
	cpu.halted = true;
	cpu.sr.raw = cpu.read_code16(cpu.pc);
	cpu.sr.b.pad = 0;
	cpu.sr.b.pad2 = 0;
	cpu.sr.b.pad3 = 0;
	cpu.pc += 2;
	printf("halted\n");
	exit(1);
}

void move_usp(m68k& cpu, u16 opc)
{
	if( ! cpu.sr.b.S )
	{
		cpu.icycles = 34;
		m68k_trap(cpu, EXC_PRIV, true);
		return;
	}
	u8 r = 8 + (opc&7);
	if( opc & 8 )
	{
		cpu.r[r] = cpu.r[16];
	} else {
		cpu.r[16] = cpu.r[r];
	}
}

void link(m68k& cpu, u16 opc)
{
	// seems to be some debate on whether LINK A7, # results in A7 being
	// pushed before||after being decremented. Harte's tests pass with -4
	// https://github.com/dirkwhoffmann/vAmiga/issues/266 suggests differently
	// Newer: The MAME microcode-based tests require NOT doing the -4
	u8 r = 8 + (opc&7);
	if( r == 15 ) r += cpu.sr.b.S^1;
	u32 val = cpu.r[r];
	//if( r >= 15 ) val -= 4;
	push32(cpu, val);
	cpu.r[r] = cpu.r[15+(cpu.sr.b.S^1)];
	cpu.r[15+(cpu.sr.b.S^1)] += se16to32(cpu.read_code16(cpu.pc));
	cpu.pc += 2;
	cpu.icycles = 16;
}

void unlk(m68k& cpu, u16 opc)
{
	cpu.icycles = 12;
	u8 r = 8 + (opc&7);
	if( r == 15 ) r += cpu.sr.b.S^1;
	cpu.r[15+(cpu.sr.b.S^1)] = cpu.r[r];
	cpu.r[r] = pop32(cpu);
}

void tas(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 1);
	u8 v = t.value();
	cpu.sr.b.C = cpu.sr.b.V = 0;
	cpu.sr.b.N = (v&0x80)?1:0;
	cpu.sr.b.Z = (v==0);
	t.set(v|0x80); // possibly doesn't happen on genesis, which is my primary goal, we'll see.
}

void tst(m68k& cpu, u16 opc)
{
	target();
	cpu.sr.b.C = cpu.sr.b.V = 0;
	u32 v = t.value();
	cpu.sr.b.Z = (v==0);
	switch( t.s )
	{
	case 1:	cpu.sr.b.N = (v&0x80)?1:0; break;
	case 2: cpu.sr.b.N = (v&0x8000)?1:0; break;
	default: cpu.sr.b.N = (v&(1u<<31))?1:0; break;	
	}
}

void trap(m68k& cpu, u16 opc)
{
	cpu.icycles = 34;
	m68k_trap(cpu, (32 + (opc&0xf))*4, false);
}

void rte(m68k& cpu, u16)
{
	if( ! cpu.sr.b.S )
	{
		cpu.icycles = 34;
		m68k_trap(cpu, EXC_PRIV, true);
		return;
	}
	cpu.icycles = 20;
	u16 nsr = pop16(cpu);
	cpu.pc = pop32(cpu);
	cpu.sr.raw = nsr;
	cpu.sr.b.pad = 0;
	cpu.sr.b.pad2 = 0;
	cpu.sr.b.pad3 = 0;
}

void rtr(m68k& cpu, u16)
{
	u16 f = pop16(cpu)&0x1f;
	cpu.pc = pop32(cpu);
	cpu.sr.raw &= 0xff00;
	cpu.sr.raw |= f;
	cpu.icycles = 20;
}

void trapv(m68k& cpu, u16)
{
	if( cpu.sr.b.V ) 
	{
		m68k_trap(cpu, EXC_TRAPV, false);
		cpu.icycles = 34;
	}
}

void rts(m68k& cpu, u16)
{
	cpu.pc = pop32(cpu);
	cpu.icycles = 16;
	//printf("RTS to $%X\n", cpu.pc);
}

void jsr(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 4);
	push32(cpu, cpu.pc);
	cpu.pc = t.EA;
	cpu.icycles += 4;
	if( t.isReg ) cpu.icycles += 2;
	if( cpu.pc & 1 ) throw "dang"; //todo: exceptions
	//printf("jsr to $%X\n", cpu.pc);
}

void jmp(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 4);
	cpu.pc = t.EA;
	cpu.icycles -= 4;
	if( cpu.pc & 1 ) throw "dang"; //todo: exceptions
}

void movem(m68k& cpu, u16 opc)
{
	u8 dir = (opc>>10)&1;
	u8 sz = (opc&0x40)?4:2;
		
	if( dir )
	{ // mem2reg
		u16 rlist = cpu.read_code16(cpu.pc);
		cpu.pc += 2;
		M t(cpu, (opc>>3)&7, opc&7, sz);
	
		//printf("m2r s%i m%i r$%X\n", sz, t.m, rlist);
		for(u32 i = 0; i < 16; ++i)
		{
			if( !(rlist & (1<<i)) ) continue;
			if( sz == 2 )
			{
				if( !cpu.sr.b.S && i == 15 ) i = 16;
				cpu.regs[i].L = se16to32(cpu.mem_read16(t.EA));
			} else {
				if( !cpu.sr.b.S && i == 15 ) i = 16;
				cpu.regs[i].L = cpu.mem_read32(t.EA);
			}
			t.EA += sz;		
		}
		if( t.m == 3 ) cpu.r[t.r] = t.EA;
	} else {
		// reg2mem
		u32 R[17];
		for(u32 i = 0; i < 17; ++i) 
		{
			R[i] = cpu.r[i];
		}
		if( cpu.sr.b.S == 0 ) R[15] = R[16];
		
		u16 rlist = cpu.read_code16(cpu.pc);
		cpu.pc += 2;
		M t(cpu, (opc>>3)&7, opc&7, sz);
		for(u32 i = 0; i < 16; ++i)
		{
			if( (rlist & (1<<i)) == 0 ) continue;
			if( sz == 2 )
			{
				cpu.mem_write16(t.EA, R[(t.m==4)?(15-i):i]);
			} else {
				cpu.mem_write32(t.EA, R[(t.m==4)?(15-i):i]);
			}
			if( t.m == 4 ) t.EA -= sz;
			else t.EA += sz;
		}
		if( t.m == 4 ) cpu.r[t.r] = t.EA + sz;
	}
}

void lea(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 4);
	opc = 8 + ((opc>>9)&7);
	if( opc == 15 ) opc += (cpu.sr.b.S^1);
	cpu.r[opc] = t.EA;
	cpu.icycles -= 4;
}

void chk(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 2);
	s16 Dn = cpu.regs[(opc>>9)&7].W;
	s16 v = t.value();
	if( Dn < 0 ) 
	{	
		cpu.sr.b.N = 1;
	} else {
		 // docs say if Dn > v, but MAME microcode-based tests only pass if N works how you'd expect N to work
		cpu.sr.b.N = 0;
	}
	
	cpu.sr.b.Z = 0; // docs say undefined, Harte's tests require clearing for zvc
	cpu.sr.b.V = 0;	
	cpu.sr.b.C = 0;
	
	if( Dn < 0 || Dn > v ) 
	{
		m68k_trap(cpu, EXC_CHK, false);
		cpu.icycles += 34;	
	}
}

void DBcc(m68k& cpu, u16 opc)
{
	u32 pc = cpu.pc;
	s16 imm = cpu.read_code16(cpu.pc);
	cpu.pc += 2;
	if( isCond(cpu, (opc>>8)&0xf) )
	{
		cpu.icycles = 12;
		return;
	}
	
	cpu.icycles = 10;
	u8 r = opc&7;
	cpu.regs[r].W--;
	if( cpu.regs[r].W != 0xffff )
	{
		cpu.pc = pc + imm;
		//printf("$%X: d%i = $%X\n", cpu.pc, r, cpu.regs[r].W);
	}
}

void Scc(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 1);
	t.value(); // 68000&68008 read before write
	if( isCond(cpu, (opc>>8)&0xf) ) 
	{
		t.set(0xff);
		if( t.isReg ) cpu.icycles += 2;
	} else {
		t.set(0);
	}
}

void addq(m68k& cpu, u16 opc)
{
	target();
	u8 imm = (opc>>9)&7;
	if( imm == 0 ) imm = 8;
	if( t.m == 1 )
	{
		t.s = 4;
		t.set(imm + cpu.r[t.r]);
		return;
	}
	u64 val = t.value();
	u64 res = val + imm;
	t.set(res);
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.C = cpu.sr.b.X = (res>>8)&1;
		cpu.sr.b.Z = ((res&0xff)==0);
		cpu.sr.b.N = (res&0x80)?1:0;
		cpu.sr.b.V = ((res ^ val) & (res ^ imm) & 0x80)?1:0;
		break;
	case 1:
		cpu.sr.b.C = cpu.sr.b.X = (res>>16)&1;
		cpu.sr.b.Z = ((res&0xffff)==0);
		cpu.sr.b.N = (res&0x8000)?1:0;
		cpu.sr.b.V = ((res ^ val) & (res ^ imm) & 0x8000)?1:0;
		break;	
	case 2:
		cpu.sr.b.C = cpu.sr.b.X = (res>>32)&1;
		cpu.sr.b.Z = (u32(res)==0);
		cpu.sr.b.N = (res&(1ul<<31))?1:0;
		cpu.sr.b.V = ((res ^ val) & (res ^ imm) & (1ul<<31))?1:0;
		break;	
	}
}

void subq(m68k& cpu, u16 opc)
{
	target();
	u64 imm = ((opc>>9)&7);	
	if( imm == 0 ) imm = 8;
	if( t.m == 1 )
	{
		t.s = 4;
		t.set(cpu.r[t.r] - imm);
		return;
	}
	imm = ~imm;
	u64 val = t.value();
	u64 res = 0;
	switch( t.s )
	{
	case 1:
		res = val + (imm&0xff) + 1;
		cpu.sr.b.C = cpu.sr.b.X = ((res>>8)&1)^1;
		cpu.sr.b.Z = ((res&0xff)==0);
		cpu.sr.b.N = (res&0x80)?1:0;
		cpu.sr.b.V = ((res ^ val) & (res ^ imm) & 0x80)?1:0;
		break;
	case 2:
		res = val + (imm&0xffff) + 1;
		cpu.sr.b.C = cpu.sr.b.X = ((res>>16)&1)^1;
		cpu.sr.b.Z = ((res&0xffff)==0);
		cpu.sr.b.N = (res&0x8000)?1:0;
		cpu.sr.b.V = ((res ^ val) & (res ^ imm) & 0x8000)?1:0;
		break;	
	case 4:
		res = val + (imm&0xffffFFFF) + 1;
		cpu.sr.b.C = cpu.sr.b.X = ((res>>32)&1)^1;
		cpu.sr.b.Z = (u32(res)==0);
		cpu.sr.b.N = (res&(1ul<<31))?1:0;
		cpu.sr.b.V = ((res ^ val) & (res ^ imm) & (1ul<<31))?1:0;
		break;	
	}
	t.set(res);		
}

void bra(m68k& cpu, u16 opc)
{
	u32 disp = (s32)(s8)opc;
	if( disp == 0 )
	{
		disp = (s32)(s16)cpu.read_code16(cpu.pc);
	}
	cpu.pc += disp;
	cpu.icycles = 10;
}

void bsr(m68k& cpu, u16 opc)
{
	u32 disp = (s32)(s8)opc;
	if( disp == 0 )
	{
		disp = (s32)(s16)cpu.read_code16(cpu.pc);
		push32(cpu, cpu.pc+2);
	} else {
		push32(cpu, cpu.pc);
	}
	cpu.pc += disp;
	cpu.icycles = 18;
}

void Bcc(m68k& cpu, u16 opc)
{
	s16 imm = (s16)(s8)(opc&0xff);
	u32 pc = cpu.pc;
	cpu.icycles = 8;
	if( imm == 0 )
	{
		imm = cpu.read_code16(cpu.pc);
		cpu.pc += 2;
		cpu.icycles += 2;
	}
	if( isCond(cpu, (opc>>8)&0xf) )
	{
		cpu.pc = pc + imm;
		cpu.icycles += 2;
	}
}

void moveq(m68k& cpu, u16 opc)
{
	u8 r = (opc>>9)&7;
	cpu.r[r] = se8to32(opc);
	cpu.sr.b.V = cpu.sr.b.C = 0;
	cpu.sr.b.Z = (cpu.r[r] == 0);
	cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
}

void divu(m68k& cpu, u16 opc)
{
	u32 PC = cpu.pc - 2;
	M t(cpu, (opc>>3)&7, opc&7, 2);
	u32 den = (u32)(u16)t.value();
	u32 num = cpu.r[(opc>>9)&7];
	cpu.sr.b.C = 0;

	if( den == 0 )
	{
		cpu.sr.b.N = cpu.sr.b.Z = 0;
		u16 csr = cpu.sr.raw;
		cpu.sr.b.S = 1;
		push32(cpu, PC);
		push16(cpu, csr);
		cpu.pc = cpu.mem_read32(EXC_DIVZERO);
		//printf("divzero trap\n");
		return;
	}
	//printf("div %i($%X) by %i($%X) = $%X\n", num,num, den,den, num/den);
	if( u32(num/den) > 0x0ffffu )
	{
		cpu.sr.b.V = 1;
		cpu.sr.b.N = 1;
		cpu.sr.b.Z = (num==0)?1:0;
		return;
	}
	u16 rem = num % den;
	u16 quo = num / den;
	cpu.sr.b.N = (quo&0x8000)?1:0;
	cpu.sr.b.Z = (quo==0);
	cpu.sr.b.V = 0;
	
	cpu.r[(opc>>9)&7] = (rem<<16)|quo;
}

void divs(m68k& cpu, u16 opc)
{
	u32 PC = cpu.pc - 2;
	M t(cpu, (opc>>3)&7, opc&7, 2);
	cpu.sr.b.C = 0;
	s32 den = (s32)(s16)t.value();
	s32 num = cpu.r[(opc>>9)&7];
	if( den == 0 )
	{
		cpu.sr.b.N = cpu.sr.b.Z = 0;
		u16 csr = cpu.sr.raw;
		cpu.sr.b.S = 1;
		push32(cpu, PC);
		push16(cpu, csr);
		cpu.pc = cpu.mem_read32(EXC_DIVZERO);
		return;
	}
	if( (num/den) < -32768 || (num/den) > 32767 )
	{
		cpu.sr.b.V = 1;
		cpu.sr.b.N = 1;
		cpu.sr.b.Z = (num==0)?1:0;
		return;
	}
	
	s16 rem = num % den;
	s16 quo = num / den;
	cpu.sr.b.N = (quo<0);
	cpu.sr.b.Z = (quo==0);
	cpu.sr.b.V = 0;
	cpu.r[(opc>>9)&7] = (u16(rem)<<16)|u16(quo);
}

void op_or(m68k& cpu, u16 opc)
{
	target();
	u32 mask = size_mask[s1_to_size[S1(opc)]];
	u64 v = t.value() & mask;
	u64 R = cpu.r[(opc>>9)&7] & mask;
	u64 res = v|R;
	cpu.sr.b.C = cpu.sr.b.V = 0;		

	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res&0xff);
		} else {
			cpu.regs[(opc>>9)&7].B = res;
		}
		break;
	case 1:
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].W = res;
		}
		break;
	default:
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		if( opc & 0x100 )
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].L = res;
		}
		break;
	}
}

void sbcd(m68k& cpu, u16 opc)
{
	u16 src = 0;
	u16 dst = 0;
	
	u8 Ddst = 8 + ((opc>>9)&7);
	if( opc & 8 )
	{
		cpu.icycles = 18;
		u8 Dsrc = 8 + (opc&7);
		if( Dsrc == 15 ) Dsrc += cpu.sr.b.S^1;
		if( Ddst == 15 ) Ddst += cpu.sr.b.S^1;
		cpu.r[Dsrc] -= (Dsrc >= 15) ? 2 : 1;
		src = cpu.mem_read8(cpu.r[Dsrc]);
		cpu.r[Ddst] -= (Ddst >= 15) ? 2 : 1;
		dst = cpu.mem_read8(cpu.r[Ddst]);
	} else {
		cpu.icycles = 6;
		dst = cpu.regs[(opc>>9)&7].B;
		src = cpu.regs[opc&7].B;
	}
	
	// copied from: https://gendev.spritesmind.net/forum/viewtopic.php?t=1964
	uint8_t dd = dst - src - cpu.sr.b.X;
	// Normal carry computation for subtraction:
	// (sm & ~dm) | (rm & ~dm) | (sm & rm)
	uint8_t bc = ((~dst & src) | (dd & ~dst) | (dd & src)) & 0x88;
	uint8_t corf = bc - (bc >> 2);
	uint8_t rr = dd - corf;
	// Compute flags.
	// Carry has two parts: normal carry for subtraction
	// (computed above) OR'ed with normal carry for
	// subtraction with corf:
	// (sm & ~dm) | (rm & ~dm) | (sm & rm)
	// but simplified because sm = 0 and ~sm = 1 for corf:
	cpu.sr.b.X = cpu.sr.b.C = (bc | (~dd & rr)) >> 7;
	// Normal overflow computation for subtraction with corf:
	// (~sm & dm & ~rm) | (sm & ~dm & rm)
	// but simplified because sm = 0 and ~sm = 1 for corf:
	cpu.sr.b.V = (dd & ~rr) >> 7;
	// Accumulate zero flag:
	cpu.sr.b.Z = cpu.sr.b.Z & (rr == 0);
	cpu.sr.b.N = rr >> 7;
	//return rr;
	if( opc & 8 )
	{
		cpu.mem_write8(cpu.r[Ddst], rr);
	} else {
		cpu.regs[(opc>>9)&7].B = rr;
	}	
}

void suba(m68k& cpu, u16 opc)
{
	u8 r = 8 + ((opc>>9)&7);
	if( r == 15 ) r += cpu.sr.b.S^1;
	if( opc & 0x100 )
	{
		M t(cpu, (opc>>3)&7, opc&7, 4);
		u32 src = t.value();
		cpu.r[r] -= src;
	} else {
		M t(cpu, (opc>>3)&7, opc&7, 2);
		u32 src = se16to32(t.value());
		cpu.r[r] -= src;
	}
}

void sub(m68k& cpu, u16 opc)
{
	target();
	u32 mask = size_mask[s1_to_size[S1(opc)]];
	u64 v = t.value() & mask;
	u64 R = cpu.r[(opc>>9)&7] & mask;
	u64 res = 0;
	if( opc & 0x100 )
	{
		R = (~R & mask);
		res = v + R + 1;
	} else {
		v = (~v & mask);
		res = R + v + 1;
	}
	
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.X = cpu.sr.b.C = ((res>>8)&1)^1;
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x80) ? 1 : 0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		if( opc & 0x100 )
		{
			t.set(res&0xff);
		} else {
			cpu.regs[(opc>>9)&7].B = res;
		}
		break;
	case 1:
		cpu.sr.b.X = cpu.sr.b.C = ((res>>16)&1)^1;
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x8000) ? 1 : 0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].W = res;
		}
		break;
	default:
		cpu.sr.b.X = cpu.sr.b.C = ((res>>32)&1)^1;
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & (1ul<<31)) ? 1 : 0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		if( opc & 0x100 )
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].L = res;
		}
		break;
	}
}

void cmp(m68k& cpu, u16 opc)
{
	target();
	u32 mask = size_mask[s1_to_size[S1(opc)]];
	u64 v = t.value() & mask;
	u64 R = cpu.r[(opc>>9)&7] & mask;
	u64 res = 0;
	v = (~v) & mask;
	res = R + v + 1;

	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.C = ((res>>8)&1)^1;
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x80) ? 1 : 0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		break;
	case 1:
		cpu.sr.b.C = ((res>>16)&1)^1;
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x8000) ? 1 : 0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		break;
	default:
		cpu.sr.b.C = ((res>>32)&1)^1;
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & (1ul<<31)) ? 1 : 0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		break;
	}
}

void cmpm(m68k& cpu, u16 opc)
{
	u8 Ax = 8 + ((opc>>9)&7);
	if( Ax == 15 ) Ax += cpu.sr.b.S^1;
	u8 Ay = 8 + (opc&7);
	if( Ay == 15 ) Ay += cpu.sr.b.S^1;
	
	u8 sz = S1(opc);
	if( sz == 0 )
	{
		u16 s = cpu.mem_read8(cpu.r[Ay]);
		cpu.r[Ay] += (Ay >= 15) ? 2 : 1;
		u16 d = cpu.mem_read8(cpu.r[Ax]);
		cpu.r[Ax] += (Ax >= 15) ? 2 : 1;
		s ^= 0xff;
		u16 r = d + s + 1;
		cpu.sr.b.C = (r>>8)^1;
		r &= 0xff;
		cpu.sr.b.Z = (r==0);
		cpu.sr.b.N = (r&0x80)?1:0;
		cpu.sr.b.V = ((r^s)&(r^d)&0x80)?1:0;
	} else if( sz == 1 ) {
		u32 s = cpu.mem_read16(cpu.r[Ay]);
		cpu.r[Ay] += 2;
		u32 d = cpu.mem_read16(cpu.r[Ax]);
		cpu.r[Ax] += 2;
		s ^= 0xffff;
		u32 r = d + s + 1;
		cpu.sr.b.C = (r>>16)^1;
		r &= 0xffff;
		cpu.sr.b.Z = (r==0);
		cpu.sr.b.N = (r&0x8000)?1:0;
		cpu.sr.b.V = ((r^s)&(r^d)&0x8000)?1:0;	
	} else {
		u64 s = cpu.mem_read32(cpu.r[Ay]);
		cpu.r[Ay] += 4;
		u64 d = cpu.mem_read32(cpu.r[Ax]);
		cpu.r[Ax] += 4;
		s ^= 0xffffFFFF;
		u64 r = d + s + 1;
		cpu.sr.b.C = (r>>32)^1;
		r &= 0xffffFFFF;
		cpu.sr.b.Z = (r==0);
		cpu.sr.b.N = (r&(1ul<<31))?1:0;
		cpu.sr.b.V = ((r^s)&(r^d)&(1ul<<31))?1:0;	
	}
}

void eor(m68k& cpu, u16 opc)
{
	target();
	u32 mask = size_mask[s1_to_size[S1(opc)]];
	u64 v = t.value() & mask;
	u64 R = cpu.r[(opc>>9)&7] & mask;
	u64 res = v^R;
	cpu.sr.b.C = cpu.sr.b.V = 0;		

	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res&0xff);
		} else {
			cpu.regs[(opc>>9)&7].B = res;
		}
		break;
	case 1:
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].W = res;
		}
		break;
	default:
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		cpu.icycles += 4;
		if( opc & 0x100 )
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].L = res;
		}
		break;
	}
}

void cmpa(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, (opc&0x100)?4:2);
	u64 v = t.value();
	u64 R = 8 + ((opc>>9)&7);
	if( R == 15 ) R += cpu.sr.b.S^1;
	R = cpu.r[R];
	if( !(opc & 0x100) ) v = (s32)(s16)v;
	v = ((~v)&0xffffFFFF);
	u64 res = R + v + 1;
	
	cpu.sr.b.C = ((res>>32)&1)^1;
	cpu.sr.b.Z = u32(res)==0;
	cpu.sr.b.V = ((res ^ v) & (res ^ R) & (1ul<<31)) ? 1 : 0;
	cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
}

void mulu(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 2);
	u32 v = t.value();
	u32 s = cpu.regs[(opc>>9)&7].W;
	v *= s;
	cpu.regs[(opc>>9)&7].L = v;
	cpu.sr.b.C = cpu.sr.b.V = 0;
	cpu.sr.b.Z = v==0;
	cpu.sr.b.N = (v&(1u<<31))?1:0;
}

void muls(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 2);
	s32 v = (s32)(s16)t.value();
	s32 s = (s32)(s16)cpu.regs[(opc>>9)&7].W;
	v *= s;
	cpu.regs[(opc>>9)&7].L = v;
	cpu.sr.b.C = cpu.sr.b.V = 0;
	cpu.sr.b.Z = (v==0)?1:0;
	cpu.sr.b.N = (v<0)?1:0;
}

void exg(m68k& cpu, u16 opc)
{	
	u8 mode = (opc>>3)&0x1F;
	u8 x = (opc>>9)&7;
	u8 y = opc&7;
	
	if( mode == 0b01000 )
	{
		; // data regs, don't need to do anything
	} else if( mode == 0b01001 ) {
		// both are addr regs
		x += 8;
		if( x == 15 ) x += cpu.sr.b.S^1;
		y += 8;
		if( y == 15 ) y += cpu.sr.b.S^1;
	} else if( mode == 0b10001 ) {
		// only y is an addr reg
		y += 8;
		if( y == 15 ) y += cpu.sr.b.S^1;
	}
	
	std::swap(cpu.r[x], cpu.r[y]);
	cpu.icycles = 6;
	return;
}

void abcd(m68k& cpu, u16 opc)
{
	u16 src = 0;
	u16 dst = 0;
	
	u8 Ddst = 8 + ((opc>>9)&7);
	if( opc & 8 )
	{
		cpu.icycles = 18;
		u8 Dsrc = 8 + (opc&7);
		if( Dsrc == 15 ) Dsrc += cpu.sr.b.S^1;
		if( Ddst == 15 ) Ddst += cpu.sr.b.S^1;
		cpu.r[Dsrc] -= (Dsrc >= 15) ? 2 : 1;
		src = cpu.mem_read8(cpu.r[Dsrc]);
		cpu.r[Ddst] -= (Ddst >= 15) ? 2 : 1;
		dst = cpu.mem_read8(cpu.r[Ddst]);
	} else {
		cpu.icycles = 6;
		dst = cpu.regs[(opc>>9)&7].B;
		src = cpu.regs[opc&7].B;
	}
	
	u16 sum = dst + src + cpu.sr.b.X;
	u16 corf = 0;
	if( ((dst&0xf)+(src&0xf)+cpu.sr.b.X) > 9 ) corf += 6;
	if( (sum&0xff) > 0x99 || (sum&0x100) ) corf += 0x60;
	cpu.sr.b.C = cpu.sr.b.X = ((sum&0x1f0)>0x90)?1:0;
	dst = sum + corf;
	cpu.sr.b.V = ((dst ^ sum) & (dst ^ corf) & 0x80)?1:0;
	cpu.sr.b.C = (cpu.sr.b.X |= (dst&0x100)?1:0);
	
	if( dst&0xff ) cpu.sr.b.Z = 0;
	cpu.sr.b.N = (dst&0x80)?1:0;
	
	if( opc & 8 )
	{
		cpu.mem_write8(cpu.r[Ddst], dst);
	} else {
		cpu.regs[(opc>>9)&7].B = dst;
	}
}

void op_and(m68k& cpu, u16 opc)
{
	target();
	u32 mask = size_mask[s1_to_size[S1(opc)]];
	u64 v = t.value() & mask;
	u64 R = cpu.r[(opc>>9)&7] & mask;
	u64 res = v&R;
	cpu.sr.b.C = cpu.sr.b.V = 0;		

	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res&0xff);
		} else {
			cpu.regs[(opc>>9)&7].B = res;
		}
		break;
	case 1:
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].W = res;
		}
		break;
	default:
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		if( opc & 0x100 )
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].L = res;
		}
		break;
	}
}

void adda(m68k& cpu, u16 opc)
{
	u8 r = 8 + ((opc>>9)&7);
	if( r == 15 ) r += cpu.sr.b.S^1;
	if( opc & 0x100 )
	{
		M t(cpu, (opc>>3)&7, opc&7, 4);
		u32 src = t.value();
		cpu.r[r] += src;
	} else {
		M t(cpu, (opc>>3)&7, opc&7, 2);
		u32 src = (s32)(s16)t.value();
		cpu.r[r] += src;
	}
}

void addx8(m68k& cpu, u16 opc)
{
	u16 src = 0;
	u16 dst = 0;
	
	u8 Ddst = 8 + ((opc>>9)&7);
	if( opc & 8 )
	{
		u8 Dsrc = 8 + (opc&7);
		if( Dsrc == 15 ) Dsrc += cpu.sr.b.S^1;
		if( Ddst == 15 ) Ddst += cpu.sr.b.S^1;
		cpu.r[Dsrc] -= (Dsrc >= 15) ? 2 : 1;
		src = cpu.mem_read8(cpu.r[Dsrc]);
		cpu.r[Ddst] -= (Ddst >= 15) ? 2 : 1;
		dst = cpu.mem_read8(cpu.r[Ddst]);
	} else {
		dst = cpu.regs[(opc>>9)&7].B;
		src = cpu.regs[opc&7].B;
	}
	
	u16 sum = src + dst + cpu.sr.b.X;
	cpu.sr.b.C = cpu.sr.b.X = (sum>>8);
	sum &= 0xff;
	if( sum ) cpu.sr.b.Z = 0;
	cpu.sr.b.V = ((sum ^ src) & (sum ^ dst) & 0x80)?1:0;
	cpu.sr.b.N = (sum&0x80)?1:0;
	
	if( opc & 8 )
	{
		cpu.mem_write8(cpu.r[Ddst], sum);
	} else {
		cpu.regs[(opc>>9)&7].B = sum;
	}
}

void addx16(m68k& cpu, u16 opc)
{
	u32 src = 0;
	u32 dst = 0;
	
	u8 Ddst = 8 + ((opc>>9)&7);
	if( opc & 8 )
	{
		u8 Dsrc = 8 + (opc&7);
		if( Dsrc == 15 ) Dsrc += cpu.sr.b.S^1;
		if( Ddst == 15 ) Ddst += cpu.sr.b.S^1;
		cpu.r[Dsrc] -= 2;
		src = cpu.mem_read16(cpu.r[Dsrc]);
		cpu.r[Ddst] -= 2;
		dst = cpu.mem_read16(cpu.r[Ddst]);
	} else {
		dst = cpu.regs[(opc>>9)&7].W;
		src = cpu.regs[opc&7].W;
	}
	
	u32 sum = src + dst + cpu.sr.b.X;
	cpu.sr.b.C = cpu.sr.b.X = (sum>>16);
	sum &= 0xffff;
	if( sum ) cpu.sr.b.Z = 0;
	cpu.sr.b.V = ((sum ^ src) & (sum ^ dst) & 0x8000)?1:0;
	cpu.sr.b.N = (sum&0x8000)?1:0;
	
	if( opc & 8 )
	{
		cpu.mem_write16(cpu.r[Ddst], sum);
	} else {
		cpu.regs[(opc>>9)&7].W = sum;
	}
}

void addx32(m68k& cpu, u16 opc)
{
	u64 src = 0;
	u64 dst = 0;
	
	u8 Ddst = 8 + ((opc>>9)&7);
	if( opc & 8 )
	{
		u8 Dsrc = 8 + (opc&7);
		if( Dsrc == 15 ) Dsrc += cpu.sr.b.S^1;
		if( Ddst == 15 ) Ddst += cpu.sr.b.S^1;
		cpu.r[Dsrc] -= 4;
		src = cpu.mem_read32(cpu.r[Dsrc]);
		cpu.r[Ddst] -= 4;
		dst = cpu.mem_read32(cpu.r[Ddst]);
	} else {
		dst = cpu.regs[(opc>>9)&7].L;
		src = cpu.regs[opc&7].L;
	}
	
	u64 sum = src + dst + cpu.sr.b.X;
	cpu.sr.b.C = cpu.sr.b.X = (sum>>32);
	sum &= 0xffffFFFF;
	if( sum ) cpu.sr.b.Z = 0;
	cpu.sr.b.V = ((sum ^ src) & (sum ^ dst) & (1ul<<31))?1:0;
	cpu.sr.b.N = (sum&(1ul<<31))?1:0;
	
	if( opc & 8 )
	{
		cpu.mem_write32(cpu.r[Ddst], sum);
	} else {
		cpu.regs[(opc>>9)&7].L = sum;
	}
}

void addx(m68k& cpu, u16 opc)
{
	switch( (opc>>6)&3 )
	{
	case 0: addx8(cpu, opc); break;
	case 1: addx16(cpu, opc); break;
	default: addx32(cpu, opc); break;	
	}
}

void subx8(m68k& cpu, u16 opc)
{
	u16 src = 0;
	u16 dst = 0;
	
	u8 Ddst = 8 + ((opc>>9)&7);
	if( opc & 8 )
	{
		u8 Dsrc = 8 + (opc&7);
		if( Dsrc == 15 ) Dsrc += cpu.sr.b.S^1;
		if( Ddst == 15 ) Ddst += cpu.sr.b.S^1;
		cpu.r[Dsrc] -= (Dsrc >= 15) ? 2 : 1;
		src = cpu.mem_read8(cpu.r[Dsrc]);
		cpu.r[Ddst] -= (Ddst >= 15) ? 2 : 1;
		dst = cpu.mem_read8(cpu.r[Ddst]);
	} else {
		dst = cpu.regs[(opc>>9)&7].B;
		src = cpu.regs[opc&7].B;
	}
	
	src ^= 0xff;
	u16 sum = src + dst + (cpu.sr.b.X^1);
	cpu.sr.b.C = cpu.sr.b.X = (sum>>8)^1;
	sum &= 0xff;
	if( sum ) cpu.sr.b.Z = 0;
	cpu.sr.b.V = ((sum ^ src) & (sum ^ dst) & 0x80)?1:0;
	cpu.sr.b.N = (sum&0x80)?1:0;
	
	if( opc & 8 )
	{
		cpu.mem_write8(cpu.r[Ddst], sum);
	} else {
		cpu.regs[(opc>>9)&7].B = sum;
	}
}

void subx16(m68k& cpu, u16 opc)
{
	u32 src = 0;
	u32 dst = 0;
	
	u8 Ddst = 8 + ((opc>>9)&7);
	if( opc & 8 )
	{
		u8 Dsrc = 8 + (opc&7);
		if( Dsrc == 15 ) Dsrc += cpu.sr.b.S^1;
		if( Ddst == 15 ) Ddst += cpu.sr.b.S^1;
		cpu.r[Dsrc] -= 2;
		src = cpu.mem_read16(cpu.r[Dsrc]);
		cpu.r[Ddst] -= 2;
		dst = cpu.mem_read16(cpu.r[Ddst]);
	} else {
		dst = cpu.regs[(opc>>9)&7].W;
		src = cpu.regs[opc&7].W;
	}
	
	src ^= 0xffff;
	u32 sum = src + dst + (cpu.sr.b.X^1);
	cpu.sr.b.C = cpu.sr.b.X = (sum>>16)^1;
	sum &= 0xffff;
	if( sum ) cpu.sr.b.Z = 0;
	cpu.sr.b.V = ((sum ^ src) & (sum ^ dst) & 0x8000)?1:0;
	cpu.sr.b.N = (sum&0x8000)?1:0;
	
	if( opc & 8 )
	{
		cpu.mem_write16(cpu.r[Ddst], sum);
	} else {
		cpu.regs[(opc>>9)&7].W = sum;
	}
}

void subx32(m68k& cpu, u16 opc)
{
	u64 src = 0;
	u64 dst = 0;
	
	u8 Ddst = 8 + ((opc>>9)&7);
	if( opc & 8 )
	{
		u8 Dsrc = 8 + (opc&7);
		if( Dsrc == 15 ) Dsrc += cpu.sr.b.S^1;
		if( Ddst == 15 ) Ddst += cpu.sr.b.S^1;
		cpu.r[Dsrc] -= 4;
		src = cpu.mem_read32(cpu.r[Dsrc]);
		cpu.r[Ddst] -= 4;
		dst = cpu.mem_read32(cpu.r[Ddst]);
	} else {
		dst = cpu.regs[(opc>>9)&7].L;
		src = cpu.regs[opc&7].L;
	}
	
	src ^= 0xffffFFFF;
	u64 sum = src + dst + (cpu.sr.b.X^1);
	cpu.sr.b.C = cpu.sr.b.X = (sum>>32)^1;
	sum &= 0xffffFFFF;
	if( sum ) cpu.sr.b.Z = 0;
	cpu.sr.b.V = ((sum ^ src) & (sum ^ dst) & (1ul<<31))?1:0;
	cpu.sr.b.N = (sum&(1ul<<31))?1:0;
	
	if( opc & 8 )
	{
		cpu.mem_write32(cpu.r[Ddst], sum);
	} else {
		cpu.regs[(opc>>9)&7].L = sum;
	}
}

void subx(m68k& cpu, u16 opc)
{
	switch( (opc>>6)&3 )
	{
	case 0: subx8(cpu, opc); break;
	case 1: subx16(cpu, opc); break;
	default: subx32(cpu, opc); break;	
	}
}

void negx(m68k& cpu, u16 opc)
{
	target();
	u64 dst = t.value();
	u64 res = 0;
	switch( S1(opc) )
	{
	case 0:
		dst ^= 0xff;
		res = dst + (cpu.sr.b.X^1);
		if( res & 0xff ) cpu.sr.b.Z = 0;
		cpu.sr.b.N = (res&0x80)?1:0;
		cpu.sr.b.V = ((res^dst)&(res^0)&0x80)?1:0;
		cpu.sr.b.C = cpu.sr.b.X = (res>>8)^1;
		res &= 0xff;
		break;
	case 1:
		dst ^= 0xffff;
		res = dst + (cpu.sr.b.X^1);
		if( res & 0xffff ) cpu.sr.b.Z = 0;
		cpu.sr.b.N = (res&0x8000)?1:0;
		cpu.sr.b.V = ((res^dst)&(res^0)&0x8000)?1:0;
		cpu.sr.b.C = cpu.sr.b.X = (res>>16)^1;
		res &= 0xffff;
		break;
	default:
		dst ^= 0xffffFFFF;
		res = dst + (cpu.sr.b.X^1);
		if( u32(res) ) cpu.sr.b.Z = 0;
		cpu.sr.b.N = (res&(1ul<<31))?1:0;
		cpu.sr.b.V = ((res^dst)&(res^0)&(1ul<<31))?1:0;
		cpu.sr.b.C = cpu.sr.b.X = (res>>32)^1;
		res &= 0xffffFFFF;	
		break;
	}
	t.set(res);
}

void add(m68k& cpu, u16 opc)
{
	target();
	u64 v = t.value() & size_mask[s1_to_size[S1(opc)]];
	u64 R = cpu.r[(opc>>9)&7] & size_mask[s1_to_size[S1(opc)]];
	u64 res = v + R;
	//printf("v($%lX) + R($%lX) = $%lX\n", v, R, res);
	switch( S1(opc) )
	{
	case 0:
		cpu.sr.b.X = cpu.sr.b.C = (res>>8)&1;
		cpu.sr.b.Z = (res&0xff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x80) ? 1 : 0;
		cpu.sr.b.N = (res&0x80) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res&0xff);
		} else {
			cpu.regs[(opc>>9)&7].B = res;
		}
		break;
	case 1:
		cpu.sr.b.X = cpu.sr.b.C = (res>>16)&1;
		cpu.sr.b.Z = (res&0xffff)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & 0x8000) ? 1 : 0;
		cpu.sr.b.N = (res&0x8000) ? 1 : 0;
		if( opc & 0x100 ) 
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].W = res;
		}
		break;
	default:
		cpu.sr.b.X = cpu.sr.b.C = (res>>32)&1;
		cpu.sr.b.Z = u32(res)==0;
		cpu.sr.b.V = ((res ^ v) & (res ^ R) & (1ul<<31)) ? 1 : 0;
		cpu.sr.b.N = (res&(1ul<<31)) ? 1 : 0;
		if( opc & 0x100 )
		{
			t.set(res);
		} else {
			cpu.regs[(opc>>9)&7].L = res;
		}
		break;
	}
}

void ASd_mem(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 2);
	u16 v = t.value();
	cpu.sr.b.V = 0;
	if( opc & 0x100 )
	{ // left
		u32 ov = v;
		cpu.sr.b.C = cpu.sr.b.X = (v>>15);
		v <<= 1;
		cpu.sr.b.V = (v^ov)>>15;
	} else {
		// right
		cpu.sr.b.C = cpu.sr.b.X = (v&1);
		v = (v&(1<<15)) | (v>>1);
		cpu.sr.b.V = 0;
	}
	cpu.sr.b.Z = (v==0);
	cpu.sr.b.N = (v>>15);
	t.set(v);
}

void ASd_reg(m68k& cpu, u16 opc)
{
	u8 sz = S1(opc);
	u8 amt = (opc>>9)&7;
	u8 r = opc&7;
	if( opc & 0x20 )
	{
		amt = cpu.r[amt] & 0x3F;
	} else {
		if( amt == 0 ) amt = 8;
	}
	cpu.sr.b.V = 0;
	
	if( opc & 0x100 )
	{	// left
		if( sz == 0 )
		{
			if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else if( amt == 8 ) {
				cpu.sr.b.C = cpu.sr.b.X = cpu.regs[r].B & 1;
				cpu.sr.b.V = (cpu.regs[r].B != 0);
				cpu.regs[r].B = 0;
			} else if( amt > 8 ) {
				cpu.sr.b.C = cpu.sr.b.X = 0;
				cpu.sr.b.V = (cpu.regs[r].B != 0);
				cpu.regs[r].B = 0;
			} else {
				for(u32 i = 0; i < amt; ++i)
				{
					u32 old = cpu.regs[r].B;
					cpu.sr.b.C = cpu.sr.b.X = (cpu.regs[r].B>>7);
					cpu.regs[r].B <<= 1;
					cpu.sr.b.V |= (old^cpu.regs[r].B)>>7;
				}
			}	
			cpu.sr.b.Z = (cpu.regs[r].B==0);
			cpu.sr.b.N = (cpu.regs[r].B&0x80)?1:0;
		} else if( sz == 1 ) {
			if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else if( amt == 16 ) {
				cpu.sr.b.C = cpu.sr.b.X = cpu.regs[r].W & 1;
				cpu.sr.b.V = (cpu.regs[r].W != 0);
				cpu.regs[r].W = 0;
			} else if( amt > 16 ) {
				cpu.sr.b.C = cpu.sr.b.X = 0;
				cpu.sr.b.V = (cpu.regs[r].W != 0);
				cpu.regs[r].W = 0;
			} else {
				for(u32 i = 0; i < amt; ++i)
				{
					u32 old = cpu.regs[r].W;
					if( i < 16 ) cpu.sr.b.C = cpu.sr.b.X = (cpu.regs[r].W>>15);
					cpu.regs[r].W <<= 1;
					cpu.sr.b.V |= (old^cpu.regs[r].W)>>15;
				}
			}		
			cpu.sr.b.Z = (cpu.regs[r].W==0);
			cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
		} else {
			if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else if( amt == 32 ) {
				cpu.sr.b.C = cpu.sr.b.X = cpu.regs[r].L & 1;
				cpu.sr.b.V = (cpu.regs[r].L != 0);
				cpu.regs[r].L = 0;
			} else if( amt > 32 ) {
				cpu.sr.b.C = cpu.sr.b.X = 0;
				cpu.sr.b.V = (cpu.regs[r].L != 0);
				cpu.regs[r].L = 0;
			} else {
				for(u32 i = 0; i < amt; ++i)
				{
					u32 old = cpu.regs[r].L;
					if( i < 32 ) cpu.sr.b.C = cpu.sr.b.X = (cpu.regs[r].L>>31);
					cpu.regs[r].L <<= 1;
					cpu.sr.b.V |= (old^cpu.regs[r].L)>>31;
				}
			}
			cpu.sr.b.Z = (cpu.r[r]==0);
			cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
		}
		return;
	}
	
	//right
	if( sz == 0 )
	{
		if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else if( amt >= 8 ) {
			if( cpu.regs[r].B & 0x80 )
			{
				cpu.sr.b.C = cpu.sr.b.X = 1;
				cpu.regs[r].B = 0xff;
			} else {
				cpu.sr.b.C = cpu.sr.b.X = 0;
				cpu.regs[r].B = 0;
			}
		} else {
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = cpu.sr.b.X = (cpu.regs[r].B&1);
				cpu.regs[r].B = (cpu.regs[r].B&0x80) | (cpu.regs[r].B>>1);
			}
		}
		cpu.sr.b.Z = (cpu.regs[r].B==0);
		cpu.sr.b.N = (cpu.regs[r].B&0x80)?1:0;
	} else if( sz == 1 ) {
		if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else if( amt >= 16 ) {
			if( cpu.regs[r].W & 0x8000 )
			{
				cpu.sr.b.C = cpu.sr.b.X = 1;
				cpu.regs[r].W = 0xffff;
			} else {
				cpu.sr.b.C = cpu.sr.b.X = 0;
				cpu.regs[r].W = 0;
			}
		} else {
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = cpu.sr.b.X = (cpu.regs[r].W&1);
				cpu.regs[r].W = (cpu.regs[r].W&0x8000) | (cpu.regs[r].W>>1);
			}
		}
		cpu.sr.b.Z = (cpu.regs[r].W==0);
		cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
	} else {
		if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else if( amt >= 32 ) {
			if( cpu.regs[r].L & (1u<<31) )
			{
				cpu.sr.b.C = cpu.sr.b.X = 1;
				cpu.regs[r].L = 0xffffFFFFu;
			} else {
				cpu.sr.b.C = cpu.sr.b.X = 0;
				cpu.regs[r].L = 0;
			}
		} else {
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = cpu.sr.b.X = (cpu.regs[r].L&1);
				cpu.regs[r].L = (cpu.regs[r].L&0x80000000u) | (cpu.regs[r].L>>1);
			}
		}
		cpu.sr.b.Z = (cpu.r[r]==0);
		cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
	}
}

void ROXd_mem(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 2);
	u32 v = 0;
	cpu.sr.b.V = 0;
	if( opc & 0x100 )
	{  // left
		v = cpu.sr.b.X;
		v |= t.value()<<1;
		
		cpu.sr.b.C = (v>>16)&1;
		v = (v<<1)|(v>>16);
		cpu.sr.b.N = (v>>16)&1;
		
		cpu.sr.b.X = v&1;
		v>>=1;
	} else {
		// right
		v = cpu.sr.b.X;
		v = (v<<16) | t.value();
		
		cpu.sr.b.C = (v&1);
		v = (v>>1)|(v<<16);
		cpu.sr.b.N = (v>>15)&1;
		
		cpu.sr.b.X = v>>16;
		v &= 0xffff;
	}
	
	cpu.sr.b.Z = (v==0);
	t.set(v);
}

void ROXd_reg(m68k& cpu, u16 opc)
{
	u8 sz = S1(opc);
	u8 amt = (opc>>9)&7;
	u8 r = opc&7;
	if( opc & 0x20 )
	{
		amt = cpu.r[amt] & 0x3F;
	} else {
		if( amt == 0 ) amt = 8;
	}
	cpu.sr.b.V = 0;
	
	if( opc & 0x100 )
	{	// left
		if( sz == 0 )
		{
			if( amt == 0 ) {
				cpu.sr.b.C = cpu.sr.b.X;
			} else {
				u16 v = cpu.sr.b.X;
				v |= cpu.regs[r].B<<1;
				for(u32 i = 0; i < amt; ++i)
				{
					cpu.sr.b.C = v>>8;
					v = (v<<1)|(v>>8);
				}
				cpu.sr.b.X = v&1;
				cpu.regs[r].B = v>>1;
			}	
			cpu.sr.b.Z = (cpu.regs[r].B==0);
			cpu.sr.b.N = (cpu.regs[r].B&0x80)?1:0;
		} else if( sz == 1 ) {
			if( amt == 0 ) {
				cpu.sr.b.C = cpu.sr.b.X;
			} else {
				u32 v = cpu.sr.b.X;
				v |= cpu.regs[r].W<<1;
				for(u32 i = 0; i < amt; ++i)
				{
					cpu.sr.b.C = v>>16;
					v = (v<<1)|(v>>16);
				}
				cpu.sr.b.X = v&1;
				cpu.regs[r].W = v>>1;
			}		
			cpu.sr.b.Z = (cpu.regs[r].W==0);
			cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
		} else {
			if( amt == 0 ) {
				cpu.sr.b.C = cpu.sr.b.X;
			} else {
				u64 v = cpu.regs[r].L;
				v <<= 1;
				v |= cpu.sr.b.X;
				for(u32 i = 0; i < amt; ++i)
				{
					cpu.sr.b.C = (v>>32)&1;
					v = (v<<1)|(v>>32);
				}
				cpu.sr.b.X = v&1;
				cpu.regs[r].L = v>>1;
			}
			cpu.sr.b.Z = (cpu.r[r]==0);
			cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
		}
		return;
	}
	
	//right
	if( sz == 0 )
	{
		if( amt == 0 ) {
			cpu.sr.b.C = cpu.sr.b.X;
		} else {
			u16 v = cpu.sr.b.X;
			v <<= 8;
			v |= cpu.regs[r].B;
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = v&1;
				v = (v>>1)|(v<<8);
			}
			cpu.regs[r].B = v;
			cpu.sr.b.X = v>>8;
		}
		cpu.sr.b.Z = (cpu.regs[r].B==0);
		cpu.sr.b.N = (cpu.regs[r].B&0x80)?1:0;
	} else if( sz == 1 ) {
		if( amt == 0 ) {
			cpu.sr.b.C = cpu.sr.b.X;
		} else {
			u32 v = cpu.sr.b.X;
			v <<= 16;
			v |= cpu.regs[r].W;
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = v&1;
				v = (v>>1)|(v<<16);
			}
			cpu.regs[r].W = v;
			cpu.sr.b.X = v>>16;
		}
		cpu.sr.b.Z = (cpu.regs[r].W==0);
		cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
	} else {
		if( amt == 0 ) {
			cpu.sr.b.C = cpu.sr.b.X;
		} else {
			u64 v = cpu.sr.b.X;
			v <<= 32;
			v |= cpu.regs[r].L;
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = v&1;
				v = (v>>1)|(v<<32);
			}
			cpu.regs[r].L = v;
			cpu.sr.b.X = v>>32;
		}
		cpu.sr.b.Z = (cpu.r[r]==0);
		cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
	}
	return;
}

void ROd_mem(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 2);
	u16 v = t.value();
	cpu.sr.b.V = 0;
	if( opc & 0x100 )
	{  // left
		cpu.sr.b.C = (v>>15);
		v = (v<<1)|(v>>15);
		cpu.sr.b.N = (v>>15);
	} else {
		// right
		cpu.sr.b.C = (v&1);
		v = (v>>1)|(v<<15);
		cpu.sr.b.N = (v>>15);
	}
	cpu.sr.b.Z = (v==0);
	t.set(v);
}

void ROd_reg(m68k& cpu, u16 opc)
{
	u8 sz = S1(opc);
	u8 amt = (opc>>9)&7;
	u8 r = opc&7;
	if( opc & 0x20 )
	{
		amt = cpu.r[amt] & 0x3F;
	} else {
		if( amt == 0 ) amt = 8;
	}
	cpu.sr.b.V = 0;
	
	if( opc & 0x100 )
	{	// left
		if( sz == 0 )
		{
			if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else {
				for(u32 i = 0; i < amt; ++i)
				{
					cpu.sr.b.C = cpu.regs[r].B>>7;
					cpu.regs[r].B = (cpu.regs[r].B<<1)|(cpu.regs[r].B>>7);
				}
			}		
			cpu.sr.b.Z = (cpu.regs[r].B==0);
			cpu.sr.b.N = (cpu.regs[r].B&0x80)?1:0;
		} else if( sz == 1 ) {
			if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else {
				for(u32 i = 0; i < amt; ++i)
				{
					cpu.sr.b.C = cpu.regs[r].W>>15;
					cpu.regs[r].W = (cpu.regs[r].W<<1)|(cpu.regs[r].W>>15);
				}
			}		
			cpu.sr.b.Z = (cpu.regs[r].W==0);
			cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
		} else {
			if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else {
				for(u32 i = 0; i < amt; ++i)
				{
					cpu.sr.b.C = cpu.regs[r].L>>31;
					cpu.regs[r].L = (cpu.regs[r].L<<1)|(cpu.regs[r].L>>31);
				}
			}
			cpu.sr.b.Z = (cpu.r[r]==0);
			cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
		}
		return;
	}
	
	//right
	if( sz == 0 )
	{
		if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else {
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = cpu.regs[r].B&1;
				cpu.regs[r].B = (cpu.regs[r].B>>1)|(cpu.regs[r].B<<7);
			}
		}
		cpu.sr.b.Z = (cpu.regs[r].B==0);
		cpu.sr.b.N = (cpu.regs[r].B&0x80)?1:0;
	} else if( sz == 1 ) {
		if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else {
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = cpu.regs[r].W&1;
				cpu.regs[r].W = (cpu.regs[r].W>>1)|(cpu.regs[r].W<<15);
			}
		}
		cpu.sr.b.Z = (cpu.regs[r].W==0);
		cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
	} else {
		if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else {
			for(u32 i = 0; i < amt; ++i)
			{
				cpu.sr.b.C = cpu.regs[r].L&1;
				cpu.regs[r].L = (cpu.regs[r].L>>1)|(cpu.regs[r].L<<31);
			}
		}
		cpu.sr.b.Z = (cpu.r[r]==0);
		cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
	}
	return;
}

void LSd_mem(m68k& cpu, u16 opc)
{
	M t(cpu, (opc>>3)&7, opc&7, 2);
	u16 v = t.value();
	cpu.sr.b.V = 0;
	
	if( opc & 0x100 )
	{  // left
		cpu.sr.b.X = cpu.sr.b.C = (v>>15);
		v <<= 1;
		cpu.sr.b.N = (v>>15);
	} else {
		// right
		cpu.sr.b.X = cpu.sr.b.C = (v&1);
		v >>= 1;
		cpu.sr.b.N = 0;
	}
	cpu.sr.b.Z = (v==0);
	t.set(v);
}

void LSd_reg(m68k& cpu, u16 opc)
{
	u8 sz = S1(opc);
	u8 amt = (opc>>9)&7;
	u8 r = opc&7;
	if( opc & 0x20 )
	{
		amt = cpu.r[amt] & 0x3F;
	} else {
		if( amt == 0 ) amt = 8;
	}
	cpu.sr.b.V = 0;
	
	if( opc & 0x100 )
	{	// left
		if( sz == 0 )
		{
			if( amt > 7 )
			{
				cpu.sr.b.X = cpu.sr.b.C = (amt==8) ? (cpu.regs[r].B&1) :0;
				cpu.regs[r].B = 0;
			} else if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else {
				cpu.sr.b.X = cpu.sr.b.C = (cpu.regs[r].B&(1<<(8-amt)))?1:0;
				cpu.regs[r].B <<= amt;
			}		
			cpu.sr.b.Z = (cpu.regs[r].B==0);
			cpu.sr.b.N = (cpu.regs[r].B&0x80)?1:0;
		} else if( sz == 1 ) {
			if( amt > 15 )
			{
				cpu.sr.b.X = cpu.sr.b.C = (amt==16) ? (cpu.regs[r].W&1) :0;
				cpu.regs[r].W = 0;
			} else if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else {
				cpu.sr.b.X = cpu.sr.b.C = (cpu.regs[r].W&(1<<(16-amt)))?1:0;
				cpu.regs[r].W <<= amt;
			}		
			cpu.sr.b.Z = (cpu.regs[r].W==0);
			cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
		} else {
			if( amt > 31 )
			{
				cpu.sr.b.X = cpu.sr.b.C = (amt==32) ? (cpu.regs[r].L&1) :0;
				cpu.regs[r].L = 0;
			} else if( amt == 0 ) {
				cpu.sr.b.C = 0;
			} else {
				cpu.sr.b.X = cpu.sr.b.C = (cpu.regs[r].L&(1<<(32-amt)))?1:0;
				cpu.regs[r].L <<= amt;
			}
			cpu.sr.b.Z = (cpu.r[r]==0);
			cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
		}
		return;
	}
	
	//right
	if( sz == 0 )
	{
		if( amt > 7 )
		{
			cpu.sr.b.X = cpu.sr.b.C = (amt==8) ? (cpu.regs[r].B>>7) :0;
			cpu.regs[r].B = 0;
		} else if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else {
			cpu.sr.b.X = cpu.sr.b.C = (cpu.regs[r].B&(1<<(amt-1)))?1:0;
			cpu.regs[r].B >>= amt;
		}		
		cpu.sr.b.Z = (cpu.regs[r].B==0);
		cpu.sr.b.N = (cpu.regs[r].B&0x80)?1:0;
	} else if( sz == 1 ) {
		if( amt > 15 )
		{
			cpu.sr.b.X = cpu.sr.b.C = (amt==16) ? (cpu.regs[r].W>>15) :0;
			cpu.regs[r].W = 0;
		} else if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else {
			cpu.sr.b.X = cpu.sr.b.C = (cpu.regs[r].W&(1<<(amt-1)))?1:0;
			cpu.regs[r].W >>= amt;
		}
		cpu.sr.b.Z = (cpu.regs[r].W==0);
		cpu.sr.b.N = (cpu.regs[r].W&0x8000)?1:0;
	} else {
		if( amt > 31 )
		{
			cpu.sr.b.X = cpu.sr.b.C = (amt==32) ? (cpu.regs[r].L>>31) :0;
			cpu.regs[r].L = 0;
		} else if( amt == 0 ) {
			cpu.sr.b.C = 0;
		} else {
			cpu.sr.b.X = cpu.sr.b.C = (cpu.regs[r].L&(1<<(amt-1)))?1:0;
			cpu.regs[r].L >>= amt;
		}
		cpu.sr.b.Z = (cpu.r[r]==0);
		cpu.sr.b.N = (cpu.r[r]&(1u<<31))?1:0;
	}
	return;
}

opcode map[] = {
	{ 0xffff, 0b0000'0000'0011'1100, ori_to_ccr },
	{ 0xffff, 0b0000'0000'0111'1100, ori_to_sr },
	{ 0xff00, 0x0000, ori },
	{ 0xffff, 0b0000'0010'0011'1100, andi_to_ccr },
	{ 0xffff, 0b0000'0010'0111'1100, andi_to_sr },
	{ 0xff00, 0x0200, andi },
	{ 0xff00, 0x0400, subi },
	{ 0xff00, 0x0600, addi },
	{ 0xffff, 0b0000'1010'0011'1100, eori_to_ccr },
	{ 0xffff, 0b0000'1010'0111'1100, eori_to_sr },
	{ 0xff00, 0x0A00, eori },
	{ 0xff00, 0x0C00, cmpi },
	{ 0xffc0, 0x0800, btst },
	{ 0xffc0, 0x0840, bchg },
	{ 0xffc0, 0x0880, bclr },
	{ 0xffc0, 0x08c0, bset },
	{ 0xf138, 0x0108, movep },
	{ 0xf1c0, 0x0100, btst2 },
	{ 0xf1c0, 0x0140, bchg2 },
	{ 0xf1c0, 0x0180, bclr2 },
	{ 0xf1c0, 0x01c0, bset2 },
	{ 0xc1c0, 0x0040, movea },
	{ 0xc000, 0x0000, move },
	{ 0xffc0, 0x40c0, move_from_sr },
	{ 0xffc0, 0x44c0, move_to_ccr },
	{ 0xffc0, 0x46c0, move_to_sr },
	{ 0xff00, 0x4000, negx },
	{ 0xff00, 0x4200, clr },
	{ 0xff00, 0x4400, neg },
	{ 0xff00, 0x4600, op_not },
	{ 0xffc0, 0x4800, nbcd },
	{ 0xffb8, 0x4880, ext },
	{ 0xfff8, 0x4840, swap },
	{ 0xffc0, 0x4840, pea },
	{ 0xffff, 0b0100'1010'1111'1100, illegal },
	{ 0xffc0, 0x4AC0, tas },
	{ 0xff00, 0x4A00, tst },
	{ 0xfff0, 0x4E40, trap },
	{ 0xfff8, 0x4E50, link },
	{ 0xfff8, 0x4E58, unlk },
	{ 0xfff0, 0x4E60, move_usp },
	{ 0xffff, 0b0100'1110'0111'0000, reset },
	{ 0xffff, 0b0100'1110'0111'0001, nop },
	{ 0xffff, 0b0100'1110'0111'0010, stop },
	
	{ 0xffff, 0b0100'1110'0111'0011, rte },
	{ 0xffff, 0b0100'1110'0111'0101, rts },
	{ 0xffff, 0b0100'1110'0111'0110, trapv },
	{ 0xffff, 0b0100'1110'0111'0111, rtr },
	{ 0xffc0, 0x4E80, jsr },
	{ 0xffc0, 0x4EC0, jmp },
	{ 0xfb80, 0x4880, movem },
	{ 0xf1c0, 0x41C0, lea },
	{ 0xf1c0, 0x4180, chk },
	
	{ 0xf0f8, 0x50C8, DBcc },
	{ 0xf0c0, 0x50c0, Scc },
	{ 0xf100, 0x5000, addq },
	{ 0xf100, 0x5100, subq },
	
	{ 0xff00, 0x6000, bra },
	{ 0xff00, 0x6100, bsr },
	{ 0xf000, 0x6000, Bcc },
	
	{ 0xf100, 0x7000, moveq },
	
	{ 0xf1c0, 0x80c0, divu },
	{ 0xf1c0, 0x81c0, divs },
	{ 0xf1f0, 0x8100, sbcd },
	{ 0xf000, 0x8000, op_or },
	
	{ 0xf0c0, 0x90c0, suba },
	{ 0xf130, 0x9100, subx },
	{ 0xf000, 0x9000, sub },
	
	{ 0xf0c0, 0xB0C0, cmpa },
	{ 0xf100, 0xB000, cmp },
	{ 0xf138, 0xB108, cmpm },
	{ 0xf100, 0xB100, eor },
	
	{ 0xf1c0, 0xC0C0, mulu },
	{ 0xf1f0, 0xC100, abcd }, // <-+
	{ 0xf1c0, 0xC1C0, muls }, // <-- the order of these three uncertain
	{ 0xf130, 0xC100, exg },  // <-+
	{ 0xf000, 0xC000, op_and },
	
	{ 0xf0c0, 0xD0C0, adda },
	{ 0xf130, 0xD100, addx },
	{ 0xf000, 0xD000, add },
	
	{ 0xfec0, 0xE0C0, ASd_mem },
	{ 0xfec0, 0xE2C0, LSd_mem },
	{ 0xfec0, 0xE4C0, ROXd_mem },
	{ 0xfec0, 0xE6C0, ROd_mem },
	{ 0xf018, 0xE000, ASd_reg },
	{ 0xf018, 0xE008, LSd_reg },
	{ 0xf018, 0xE010, ROXd_reg },
	{ 0xf018, 0xE018, ROd_reg },	
};

m68k_instr opcodes[0x10000];

// m68k::init() to be called once in main
void m68k::init()
{
	for(u32 opc = 0; opc < 0x10000; ++opc)
	{
		for(u32 i = 0; i < sizeof(map)/sizeof(opcode); ++i)
		{
			if( (opc&map[i].mask) == map[i].res )
			{
				//printf("$%X: running opcode = $%X = %i\n", pc-2, opc, i);
				opcodes[opc] = map[i].i;
				break;		
			}
		}
	}
}

void m68k::step()
{
	icycles = 4;
	
	if( sr.b.IPL < pending_irq ) 
	{
		m68k_trap(*this, EXC_LEVEL_1 + (pending_irq-1)*4, false);
		printf("68K: IRQ %i\n", pending_irq);
		sr.b.IPL = pending_irq;
		pending_irq = 0;
		intack();
		if( halted )
		{
			halted = false;
			//cpu.pc += 2;
		}
		icycles = 34;
		return;
	}

	/*if( hip && (sr.b.IPL<4) )
	{
		hip = false;
		m68k_trap(*this, EXC_LEVEL_4, false);
		sr.b.IPL = 4;
		if( halted )
		{
			halted = false;	
			cpu.pc += 2;
		}
	} else if( vip && (sr.b.IPL<6) ) {
		vip = false;
		m68k_trap(*this, EXC_LEVEL_6, false);
		sr.b.IPL = 6;
		if( halted )
		{
			halted = false;
			cpu.pc += 2;
		}
	}*/
	
	if( halted ) return;

	u16 opc = read_code16(pc);
	pc += 2;
	
	/*
	if( (opc&0xffc0) == 0xc0 || (opc&0xffc0)== 0x2c0 
	 || (opc&0xffc0)== 0x4c0 || (opc&0xffc0)== 0x6c0
	 || (opc&0xffc0)== 0xAc0 || (opc&0xffc0)== 0xCc0
	 || (opc&0xff00)== 0x0e00 || (opc&0xffc0)== 0x42c0 )
	{
		m68k_trap(EXC_ILLEGAL, true);
		return;	
	}
	*/
	
	/*for(u32 i = 0; i < sizeof(map)/sizeof(opcode); ++i)
	{
		if( (opc&map[i].mask) == map[i].res )
		{
			//printf("$%X: running opcode = $%X = %i\n", pc-2, opc, i);
			map[i].i(*this, opc);
			return;		
		}
	}*/
	opcodes[opc](*this, opc);
	
	/*
	if( (opc&0xf000) != 0xA000 )
	{
		m68k_trap(EXC_ILLEGAL, true);
		return;
	}
	*/
	
	//printf("Error: step shouldn't reach the end, pc=$%X, opcode = $%X\n", cpu.pc-2, opc);
	//printf("USP = $%X, SSP = $%X\n", cpu.r[16], cpu.r[15]);
	//exit(1);
}









