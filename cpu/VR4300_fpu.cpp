#include <climits>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cfenv>
#include "VR4300.h"
#define JTYPE u64 target = ((opc<<2)&0x0fffFFFFu) + (cpu.npc&~0x0fff'ffffull)
#define RTYPE u32 d = (opc>>11)&0x1f; u32 s = (opc>>21)&0x1f; u32 t = (opc>>16)&0x1f; u32 sa=(opc>>6)&0x1f
#define ITYPE u32 t = (opc>>16)&0x1f; u32 s = (opc>>21)&0x1f; u16 imm16 = opc
#define INSTR [](VR4300& cpu, u32 opc)
#define OVERFLOW32(r, a, b) (((r)^(a))&((r)^(b))&BIT(31))
#define OVERFLOW64(r, a, b) (((r)^(a))&((r)^(b))&BITL(63))
#define LINK cpu.nnpc
#define DS_REL_ADDR (cpu.npc + (s32(s16(imm16))<<2))
#define LIKELY cpu.npc = cpu.nnpc; cpu.nnpc += 4;
#define FLTYPE u32 fs = (opc>>11)&0x1f; u32 rt = (opc>>16)&0x1f
#define FITYPE  if( cpu.COPUnusable(1) ) return; \
		u32 fs = (opc>>11)&0x1f; \
		u32 fd = (opc>>6)&0x1f;  \
		u32 ft = (opc>>16)&0x1f; \
		cpu.FCSR &= ~0x3f000

typedef void(*vr4300_instr) (VR4300&, u32);
vr4300_instr cop1_word(VR4300&, u32);
vr4300_instr cop1_long(VR4300&, u32);

u32 fnan = 0x7FBFFFFF;
u64 dnan = 0x7FF7FFFFFFFFFFFFull;

bool VR4300::isQNaN_f(float a)
{
	u32 v;
	memcpy(&v, &a, 4);
	return isNaN_f(a) && (v & BIT(22));
}

bool VR4300::isQNaN_d(double a)
{
	u64 v;
	memcpy(&v, &a, 8);
	return isNaN_d(a) && (v & BITL(51));
}

bool VR4300::isNaN_f(float a)
{
	u32 v;
	memcpy(&v, &a, 4);
	u32 exp = (v >> 23)&0xff;
	u32 mant = v & (BIT(23)-1);
	return (exp == 0xff) && (mant != 0);
}

bool VR4300::isNaN_d(double a)
{
	u64 v;
	memcpy(&v, &a, 8);
	u32 exp = (v >> 52) & 0x7ff;
	u64 mant = v & (BITL(52)-1);
	return (exp == 0x7ff) && (mant != 0);
}

bool VR4300::isSubnormf(float a)
{
	u32 v;
	memcpy(&v, &a, 4);
	return ((v & 0x7F800000) == 0) && ((v & 0x7FFFFF) != 0);
}

bool VR4300::isSubnormd(double a)
{
	u64 v;
	memcpy(&v, &a, 8);
	return ((v & 0x7FF80000'00000000ull) == 0) && ((v & 0x7FFFF'FFFFFFFFull) != 0);
}

vr4300_instr cop1_d(VR4300& proc, u32 opcode)
{
	if( (opcode & 0x3F) >= 0x30 )
	{	// All the compares
		return INSTR {
			FITYPE;
			double a, b;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			bool res = false; //(opc & 1) && isnan;
			res = res || ((opc & 2) && (a==b));
			res = res || ((opc & 4) && (a <b));
			cpu.fpu_cond = res;
			cpu.FCSR &= ~BIT(23);
			if( res ) cpu.FCSR |= BIT(23);
		};
	}

	switch( opcode & 0x3F )
	{
	case 0: // ADD.D
		return INSTR {
			FITYPE;
			double a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			c = a + b;
			memcpy(&cpu.f[fd<<3], &c, 8);
		};	
	case 1: // SUB.D
		return INSTR {
			FITYPE;
			double a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			c = a - b;
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 2: // MUL.D
		return INSTR {
			FITYPE;
			double a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			c = a * b;
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 3: // DIV.D
		return INSTR {
			FITYPE;
			double a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			memcpy(&b, &cpu.f[ft<<3], 8);
			if( b == 0 ) { cpu.signal_fpu(cpu.FPU_DIVZERO); return; }
			c = a / b;
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 4: // SQRT.D
		return INSTR {
			FITYPE;
			double a, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			if( a < 0 )
			{
				if( !cpu.signal_fpu(cpu.FPU_INVALID) )
				{
					memcpy(&cpu.f[fd<<3], &dnan, 8);
				}
				return;
			} else {
				c = sqrt(a);
			}
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 5: // ABS.D
		return INSTR {
			FITYPE;
			double a, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			c = (a < 0) ? -a : a;
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 6: // MOV.D
		return INSTR {
			auto temp = cpu.FCSR;
			FITYPE;
			cpu.FCSR = temp;
			memcpy(&cpu.f[fd<<3], &cpu.f[fs<<3], 8);
		};
	case 7: // NEG.D
		return INSTR {
			FITYPE;
			double a, c;
			memcpy(&a, &cpu.f[fs<<3], 8);
			c = -a;
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 8: // ROUND.L.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			u64 c = llround(a);
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 9: // TRUNC.L.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			u64 c = trunc(a);
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 10: // CEIL.L.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			u64 c = ceil(a);
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 11: // FLOOR.L.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			u64 c = floor(a);
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 12: // ROUND.W.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			u32 c = round(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 13: // TRUNC.W.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			u32 c = trunc(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 14: // CEIL.W.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			u32 c = ceil(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 15: // FLOOR.W.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			u32 c = floor(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};

	case 0x20: // CVT.S.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			float b = a;
			memcpy(&cpu.f[fd<<3], &b, 4);		
		};

	case 0x24: // CVT.W.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			s32 b = a;
			memcpy(&cpu.f[fd<<3], &b, 4);		
		};
	case 0x25: // CVT.L.D
		return INSTR {
			FITYPE;
			double a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			s64 b = a;
			memcpy(&cpu.f[fd<<3], &b, 8);		
		};
	default: return INSTR {};
	}
}

vr4300_instr cop1_s(VR4300& proc, u32 opcode)
{
	if( (opcode & 0x3F) >= 0x30 )
	{	// All the compares
		return INSTR {
			FITYPE;
			float a, b;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			bool res = false;
			//res = res || ((opc & 1) && (cpu.isQNaN_f(a)||cpu.isSNaN_f(a)||cpu.isQNaN_f(b)||cpu.isSNaN_f(b)));
			res = res || ((opc & 2) && (a==b));
			res = res || ((opc & 4) && (a <b));
			cpu.fpu_cond = res;
			cpu.FCSR &= ~BIT(23);
			if( res ) cpu.FCSR |= BIT(23);
		};
	}

	switch( opcode & 0x3F )
	{
	case 0: // ADD.S
		return INSTR {
			FITYPE;
			float a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			c = a + b;
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 1: // SUB.S
		return INSTR {
			FITYPE;
			float a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			c = a - b;
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 2: // MUL.S
		return INSTR {
			FITYPE;
			float a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			c = a * b;
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 3: // DIV.S
		return INSTR {
			FITYPE;
			float a, b, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			memcpy(&b, &cpu.f[ft<<3], 4);
			if( b == 0 && cpu.signal_fpu(cpu.FPU_DIVZERO) ) { return; }
			c = a / b;
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 4: // SQRT.S
		return INSTR {
			FITYPE;
			float a, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			c = sqrt(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 5: // ABS.S
		return INSTR {
			FITYPE;
			float a, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			c = (a < 0) ? -a : a;
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 6: // MOV.S
		return INSTR {
			auto temp = cpu.FCSR;
			FITYPE;
			cpu.FCSR = temp;
			memcpy(&cpu.f[fd<<3], &cpu.f[fs<<3], 8);
		};
	case 7: // NEG.S
		return INSTR {
			FITYPE;
			float a, c;
			memcpy(&a, &cpu.f[fs<<3], 4);
			c = -a;
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 8: // ROUND.L.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			u64 c = llround(a);
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 9: // TRUNC.L.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			u64 c = trunc(a);
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 10: // CEIL.L.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			u64 c = ceil(a);
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 11: // FLOOR.L.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			u64 c = floor(a);
			memcpy(&cpu.f[fd<<3], &c, 8);
		};
	case 12: // ROUND.W.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			u32 c = round(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 13: // TRUNC.W.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			u32 c = trunc(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 14: // CEIL.W.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			u32 c = ceil(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
	case 15: // FLOOR.W.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			u32 c = floor(a);
			memcpy(&cpu.f[fd<<3], &c, 4);
		};
		
	case 0x21: // CVT.D.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			double b = a;
			memcpy(&cpu.f[fd<<3], &b, 8);		
		};
		
	case 0x24: // CVT.W.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			s32 b = a;
			memcpy(&cpu.f[fd<<3], &b, 4);		
		};
	case 0x25: // CVT.L.S
		return INSTR {
			FITYPE;
			float a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			s64 b = a;
			memcpy(&cpu.f[fd<<3], &b, 8);		
		};
	default: return INSTR {};
	}
}

vr4300_instr cop1(VR4300& proc, u32 opcode)
{
	switch( (opcode>>21) & 0x1F )
	{
	case 0: // MFC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( !(cpu.STATUS & BIT(26)) )
			{
				fs = ((fs&~1)<<3) + ((fs&1)?4:0);
			} else { 
				fs <<= 3;
			}
			memcpy(&cpu.r[rt], &cpu.f[fs], 4);
			cpu.r[rt] = s32(cpu.r[rt]);
		};
	case 1: // DMFC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( !(cpu.STATUS & BIT(26)) ) fs &= ~1;
			memcpy(&cpu.r[rt], &cpu.f[fs<<3], 8);
		};
	case 2: // CFC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( fs == 0 )
			{
				cpu.r[rt] = 0xa00;
			} else {
				cpu.r[rt] = s32(cpu.FCSR);
			}
		};
		
	case 4: // MTC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( !(cpu.STATUS & BIT(26)) )
			{
				fs = ((fs&~1)<<3) + ((fs&1)?4:0);
			} else { 
				fs <<= 3;
			}
			memcpy(&cpu.f[fs], &cpu.r[rt], 4);
		};
	case 5: // DMTC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( !(cpu.STATUS & BIT(26)) ) fs &= ~1;
			memcpy(&cpu.f[fs<<3], &cpu.r[rt], 8);
		};
	case 6: // CTC1
		return INSTR {
			FLTYPE;
			if( cpu.COPUnusable(1) ) return;
			if( fs != 0 )
			{
				//u32 oldrnd = cpu.FCSR & 3;
				cpu.FCSR = cpu.r[rt] & 0x183ffffu;
				if( ((cpu.FCSR>>7) & (cpu.FCSR>>12) & 0x1F) || (cpu.FCSR & BIT(17)) )
				{
					cpu.exception(15);
				}
				if( true ) //oldrnd != (cpu.FCSR&3) )
				{
					switch( cpu.FCSR&3 )
					{
					case 0: fesetround(FE_TONEAREST); break;
					case 1: fesetround(FE_TOWARDZERO); break;
					case 2: fesetround(FE_UPWARD); break;
					case 3: fesetround(FE_DOWNWARD); break;
					}
				}
				cpu.fpu_cond = cpu.FCSR & BIT(23);
			}
		};
	case 8: // BC
		return INSTR {
			ITYPE;
			if( cpu.COPUnusable(1) ) return;
			switch( (opc>>16)&0x1f )
			{
			case 0: // BCF
				cpu.ndelay = true;
				if( !cpu.fpu_cond )
				{
					cpu.branch(DS_REL_ADDR);
				}
				break;
			case 1: // BCT
				cpu.ndelay = true;
				if( cpu.fpu_cond )
				{
					cpu.branch(DS_REL_ADDR);
				}
				break;
			case 2: // BCF Likely
				if( !cpu.fpu_cond )
				{
					cpu.branch(DS_REL_ADDR);
				} else {
					LIKELY;
				}
				break;
			case 3: // BCT Likely
				if( cpu.fpu_cond )
				{
					cpu.branch(DS_REL_ADDR);
				} else {
					LIKELY;
				}
				break;
			default: 
				printf("cop1_bc unimpl opcode = $%X\n", (opc>>16)&0x1f);
				break;
			}
		};
		
	case 0x10: return cop1_s(proc, opcode);
	case 0x11: return cop1_d(proc, opcode);
	case 0x14: return cop1_word(proc, opcode);
	case 0x15: return cop1_long(proc, opcode);
	default:
		//printf("VR4300: Unimpl or undef cop1 opcode = $%X\n", opcode);
		return INSTR { 	if( cpu.COPUnusable(1) ) return; };
	}
	return nullptr;
}

vr4300_instr cop1_word(VR4300&, u32 opcode)
{
	switch( opcode & 0x3F )
	{
	case 0x20: // CVT.S
		return INSTR {
			FITYPE;
			s32 a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			float b = a;
			memcpy(&cpu.f[fd<<3], &b, 4);
		};
	case 0x21: // CVT.D
		return INSTR {
			FITYPE;
			s32 a;
			memcpy(&a, &cpu.f[fs<<3], 4);
			double b = a;
			memcpy(&cpu.f[fd<<3], &b, 8);
		};
	default: 
		printf("cop1_word unimpl opcode = $%X\n", opcode & 0x3F);
		return INSTR {};
	}
}

vr4300_instr cop1_long(VR4300&, u32 opcode)
{
	switch( opcode & 0x3F )
	{
	case 0x20: // CVT.S
		return INSTR {
			FITYPE;
			s64 a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			float b = a;
			memcpy(&cpu.f[fd<<3], &b, 4);
		};
	case 0x21: // CVT.D
		return INSTR {
			FITYPE;
			s64 a;
			memcpy(&a, &cpu.f[fs<<3], 8);
			double b = a;
			memcpy(&cpu.f[fd<<3], &b, 8);
		};	
	default: 
		printf("cop1_long unimpl opcode = $%X\n", opcode & 0x3F);
		return INSTR {};
	}
}


