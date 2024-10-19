#include <cstdio>
#include <cstdlib>
#include "itypes.h"
#include "sm83.h"
//#include "dmg.h"
#include "gbc.h"

#define B(a) r.b[((a)^1)]
#define W(a) r.w[(a)]
#define BC W(0)
#define DE W(1)
#define HL W(2)
#define AF ((REG_A<<8)|(REG_F))
#define REG_B B(0)
#define REG_C B(1)
#define REG_D B(2)
#define REG_E B(3)
#define REG_H B(4)
#define REG_L B(5)
#define REG_A r.b[6]
#define REG_F r.b[7]
#define FLAG_C ((REG_F>>4)&1)
// zshc

extern u64 sm83_cycles[256];

void sm83::reset()
{
	PC = 0x100;
	SP = 0xFFFE;
	REG_A = 1;
	REG_B = 0;
	ime = false;
	halted = false;
	ime_delay = 0;
}

u8 sm83::reg(u8 p)
{
	p &= 7;
	if( p == 6 ) return read(HL);
	return B(p);	
}

void sm83::set_reg(u8 p, u8 val)
{
	p &= 7;
	if( p == 6 )
	{
		write(HL, val);
	} else {
		B(p) = val;
	}
}

extern console* sys;
#define GB (*dynamic_cast<gbc*>(sys))

u64 sm83::step()
{
	u8 service = GB.io[0xf] & GB.io[0xff] & 0x1F;
	
	if( service && !ime_delay )
	{
		if( halted ) 
		{
			halted = false;
			PC++;
		}
		
		if( ime )
		{
			for(int i = 0; i < 5; ++i)
			{
				int bit = (1 << i);
				if( !(service & bit) ) continue;
				//printf("service %X\n", bit);
				GB.io[0xf] &= ~bit;
				ime = false;
				push(PC);
				PC = 0x40|(i<<3);
				//printf("Interrupt %i!\n", i);
				break;
			}
		}	
	}
	
	if( ime_delay )
	{
		ime_delay--;
		if( ime_delay == 0 ) ime = true;
	}
	
	if( halted ) { stamp += 4; return 4; }

	u8 opc = read(PC++);
	//printf("SM83:$%X: opc = $%X\n", PC-1, opc);
	u64 extra = 0;
	
	switch( opc )
	{
	case 0x00: break;
	case 0x01: BC = read16(PC); PC += 2; break;
	case 0x02: write(BC, REG_A); break;
	case 0x03: BC++; break;
	case 0x04: REG_B = inc(REG_B); break;
	case 0x05: REG_B = dec(REG_B); break;
	case 0x06: REG_B = read(PC++); break;
	case 0x07: REG_A = rlc(REG_A); REG_F &= 0x10; break;
	case 0x08: t16 = read16(PC);
		   PC += 2;
		   write(t16, SP);
		   write(t16+1, SP>>8);
		   break;
	case 0x09: HL = add16(HL, BC); break;
	case 0x0A: REG_A = read(BC); break;
	case 0x0B: BC--; break;
	case 0x0C: REG_C = inc(REG_C); break;
	case 0x0D: REG_C = dec(REG_C); break;
	case 0x0E: REG_C = read(PC++); break;
	case 0x0F: REG_A = rrc(REG_A); REG_F &= 0x10; break;
	case 0x10: PC++; break; //todo: stop
	case 0x11: DE = read16(PC); PC += 2; break;
	case 0x12: write(DE, REG_A); break;
	case 0x13: DE++; break;
	case 0x14: REG_D = inc(REG_D); break;
	case 0x15: REG_D = dec(REG_D); break;
	case 0x16: REG_D = read(PC++); break;
	case 0x17: REG_A = rl(REG_A); REG_F &= 0x10; break;
	case 0x18: t8 = read(PC++); PC += (s8)t8; break;
	case 0x19: HL = add16(HL, DE); break;
	case 0x1A: REG_A = read(DE); break;
	case 0x1B: DE--; break;
	case 0x1C: REG_E = inc(REG_E); break;
	case 0x1D: REG_E = dec(REG_E); break;
	case 0x1E: REG_E = read(PC++); break;
	case 0x1F: REG_A = rr(REG_A); REG_F &= 0x10; break;
	case 0x20: t8 = read(PC++); if( !(REG_F&0x80) ) PC += (s8)t8; break;
	case 0x21: HL = read16(PC); PC += 2; break;
	case 0x22: write(HL++, REG_A); break;
	case 0x23: HL++; break;
	case 0x24: REG_H = inc(REG_H); break;
	case 0x25: REG_H = dec(REG_H); break;
	case 0x26: REG_H = read(PC++); break;
	case 0x27: daa(); break; //todo: daa
	case 0x28: t8 = read(PC++); if( REG_F&0x80 ) PC += (s8)t8; break;
	case 0x29: HL = add16(HL, HL); break;
	case 0x2A: REG_A = read(HL++); break;
	case 0x2B: HL--; break;
	case 0x2C: REG_L = inc(REG_L); break;
	case 0x2D: REG_L = dec(REG_L); break;
	case 0x2E: REG_L = read(PC++); break;
	case 0x2F: REG_A = ~REG_A; REG_F |= 0x60; break;
	case 0x30: t8 = read(PC++); if( !(REG_F&0x10) ) PC += (s8)t8; break;
	case 0x31: SP = read16(PC); PC += 2; break;
	case 0x32: write(HL--, REG_A); break;
	case 0x33: SP++; break;
	case 0x34: write(HL, inc(read(HL))); break;
	case 0x35: write(HL, dec(read(HL))); break;
	case 0x36: write(HL, read(PC++)); break;
	case 0x37: REG_F &= 0x80; REG_F |= 0x10; break;
	case 0x38: t8 = read(PC++); if( REG_F&0x10 ) PC += (s8)t8; break;
	case 0x39: HL = add16(HL, SP); break;
	case 0x3A: REG_A = read(HL--); break;
	case 0x3B: SP--; break;
	case 0x3C: REG_A = inc(REG_A); break;
	case 0x3D: REG_A = dec(REG_A); break;
	case 0x3E: REG_A = read(PC++); break;
	case 0x3F: REG_F ^= 0x10; REG_F &= 0x90; break;	
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x47: B(0) = B(opc&7); break;
	case 0x46: B(0) = read(HL); break;
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4F: B(1) = B(opc&7); break;
	case 0x4E: B(1) = read(HL); break;
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x57: B(2) = B(opc&7); break;
	case 0x56: B(2) = read(HL); break;
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5F: B(3) = B(opc&7); break;
	case 0x5E: B(3) = read(HL); break;
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x64:
	case 0x65:
	case 0x67: B(4) = B(opc&7); break;
	case 0x66: B(4) = read(HL); break;
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6F: B(5) = B(opc&7); break;
	case 0x6E: B(5) = read(HL); break;
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x77: write(HL, B(opc&7)); break;
	case 0x76: halted = true; PC--; break;
	case 0x78:
	case 0x79:
	case 0x7A:
	case 0x7B:
	case 0x7C:
	case 0x7D:
	case 0x7F: B(7) = B(opc&7); break;
	case 0x7E: B(7) = read(HL); break;
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x87: REG_A = add(REG_A, B(opc&7), 0); break;
	case 0x86: REG_A = add(REG_A, read(HL), 0); break;
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
	case 0x8D:
	case 0x8F: REG_A = add(REG_A, B(opc&7), FLAG_C); break;
	case 0x8E: REG_A = add(REG_A, read(HL), FLAG_C); break;
	case 0x90:
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x97: REG_A = add(REG_A, ~B(opc&7), 1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0x96: REG_A = add(REG_A, ~read(HL), 1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0x98:
	case 0x99:
	case 0x9A:
	case 0x9B:
	case 0x9C:
	case 0x9D:
	case 0x9F: REG_A = add(REG_A, ~B(opc&7), FLAG_C^1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0x9E: REG_A = add(REG_A, ~read(HL), FLAG_C^1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	case 0xA4:
	case 0xA5:
	case 0xA7: REG_A &= B(opc&7); REG_F = (REG_A==0)?0xA0:0x20; break;
	case 0xA6: REG_A &= read(HL); REG_F = (REG_A==0)?0xA0:0x20; break;
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAF: REG_A ^= B(opc&7); REG_F = (REG_A==0)?0x80:0; break;
	case 0xAE: REG_A ^= read(HL); REG_F = (REG_A==0)?0x80:0; break;
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB7: REG_A |= B(opc&7); REG_F = (REG_A==0)?0x80:0; break;
	case 0xB6: REG_A |= read(HL); REG_F = (REG_A==0)?0x80:0; break;
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBF: add(REG_A, ~B(opc&7), 1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0xBE: add(REG_A, ~read(HL), 1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0xC0: if( !(REG_F&0x80) )
		   {
		   	extra = 16;
			PC = pop();
		   }
		   break;
	case 0xC1: BC = pop(); break;
	case 0xC2: if( !(REG_F&0x80) )
		   {
		   	extra = 4;
		   	PC = read16(PC);
		   } else {
		   	PC+=2;
		   }
		   break;
	case 0xC3: PC = read16(PC); break;
	case 0xC4: if( !(REG_F&0x80) )
		   {
		   	extra = 12;
		   	t16 = read16(PC);
		   	push(PC+2);
		   	PC = t16;
		   } else {
		   	PC+=2;
		   }
		   break;
	case 0xC5: push(BC); break;
	case 0xC6: REG_A = add(REG_A, read(PC++), 0); break;
	case 0xC7: rst(0); break;
	case 0xC8: if( REG_F&0x80 )
		   {
		   	extra = 16;
			PC = pop();
		   }
		   break;
	case 0xC9: PC = pop(); break;
	case 0xCA: if( REG_F&0x80 )
		   {
		   	extra = 4;
		   	PC = read16(PC);
		   } else {
		   	PC+=2;
		   }
		   break;
	case 0xCB: extra = prefix(); break;
	case 0xCC: if( REG_F&0x80 )
		   {
		   	extra = 12;
		   	push(PC+2);
		   	PC = read16(PC);
		   } else {
		   	PC+=2;
		   }
		   break;
	case 0xCD: t16 = read16(PC);
		   push(PC+2);
		   PC = t16;
		   break;
	case 0xCE: REG_A = add(REG_A, read(PC++), FLAG_C); break;
	case 0xCF: rst(8); break;	
	case 0xD0: if( !(REG_F&0x10) )
		   {
		   	extra = 16;
			PC = pop();
		   }
		   break;
	case 0xD1: DE = pop(); break;
	case 0xD2: if( !(REG_F&0x10) )
		   {
		   	extra = 4;
		   	PC = read16(PC);
		   } else {
		   	PC+=2;
		   }
		   break;
	case 0xD4: if( !(REG_F&0x10) )
		   {
		   	t16 = read16(PC);
		   	extra = 12;
		   	push(PC+2);
		   	PC = t16;
		   } else {
		   	PC+=2;
		   }
		   break;
	case 0xD5: push(DE); break;
	case 0xD6: REG_A = add(REG_A, ~read(PC++), 1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0xD7: rst(0x10); break;
	case 0xD8: if( REG_F&0x10 )
		   {
		   	extra = 16;
			PC = pop();
		   }
		   break;
	case 0xD9: PC = pop(); ime = true; break; //reti
	case 0xDA: if( REG_F&0x10 )
		   {
		   	extra = 4;
		   	PC = read16(PC);
		   } else {
		   	PC+=2;
		   }
		   break;
	case 0xDC: if( REG_F&0x10 )
		   {
		   	extra = 12;
		   	t16 = read16(PC);
		   	push(PC+2);
		   	PC = t16;
		   } else {
		   	PC+=2;
		   }
		   break;
	case 0xDE: REG_A = add(REG_A, ~read(PC++), FLAG_C^1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0xDF: rst(0x18); break;
	
	case 0xE0: write(0xff00 + read(PC++), REG_A); break;
	case 0xE1: HL = pop(); break;
	case 0xE2: write(0xff00 + REG_C, REG_A); break;
	case 0xE5: push(HL); break;
	case 0xE6: REG_A &= read(PC++); REG_F = (REG_A==0)?0xA0:0x20; break;
	case 0xE7: rst(0x20); break;
	case 0xE8: SP = addsp(read(PC++)); break;
	case 0xE9: PC = HL; break;
	case 0xEA: t16 = read16(PC);
		   PC += 2;
		   write(t16, REG_A);
		   break;
	case 0xEE: REG_A ^= read(PC++); REG_F = (REG_A==0)?0x80:0; break;
	case 0xEF: rst(0x28); break;	
	case 0xF0: REG_A = read(0xff00+read(PC++)); break;
	case 0xF1: t16 = pop(); REG_A = t16>>8; REG_F = t16; REG_F &= 0xf0;  break;
	case 0xF2: REG_A = read(0xff00+REG_C); break;
	case 0xF3: ime_delay = 0; ime = false; break; //di
	case 0xF5: push(AF); break;
	case 0xF6: REG_A |= read(PC++); REG_F = (REG_A==0)?0x80:0; break;
	case 0xF7: rst(0x30); break;
	case 0xF8: HL = addsp(read(PC++)); break;
	case 0xF9: SP = HL; break;
	case 0xFA: t16 = read16(PC);
		   PC+=2;
		   REG_A = read(t16);
		   break;
	case 0xFB: ime_delay = 1; break; //ei
	case 0xFE: add(REG_A, ~read(PC++), 1); REG_F ^= 0x30; REG_F |= 0x40; break;
	case 0xFF: rst(0x38); break;
	default:
		//printf("Unimpl opc = $%02X\n", opc);
		//exit(1);
		break;
	}
	
	extra += sm83_cycles[opc];
	stamp += extra;
	return extra;
}

u64 sm83::prefix()
{
	u8 opc = read(PC++);
	if( opc >= 0xC0 )
	{
		u8 b = reg(opc&7);
		b |= 1<<((opc>>3)&7);
		set_reg(opc&7, b);
		return 8+(((opc&7)==6)?8:0);
	}
	if( opc >= 0x80 )
	{
		u8 b = reg(opc&7);
		b &= ~(1<<((opc>>3)&7));		
		set_reg(opc&7, b);
		return 8+(((opc&7)==6)?8:0);
	}
	if( opc >= 0x40 )
	{
		u8 b = reg(opc&7);
		b &= 1<<((opc>>3)&7);
		REG_F &= 0x10;
		REG_F |= (b==0)?0xA0:0x20;
		return 8+(((opc&7)==6)?4:0);
	}
	switch( opc )
	{
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
		t8 = reg(opc&7);
		t8 = (t8<<4)|(t8>>4);
		set_reg(opc&7, t8);
		REG_F = (t8==0)?0x80:0;
		return 8+(((opc&7)==6)?8:0);
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B:
	case 0x3C:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		t8 = reg(opc&7);
		REG_F = ((t8<<4)&0x10);
		t8 >>= 1;
		REG_F |= (t8==0)?0x80:0;
		set_reg(opc&7, t8);
		return 8+(((opc&7)==6)?8:0);
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
		t8 = reg(opc&7);
		REG_F = ((t8>>3)&0x10);
		t8 <<= 1;
		REG_F |= (t8==0)?0x80:0;
		set_reg(opc&7, t8);
		return 8+(((opc&7)==6)?8:0);
	case 0x28:
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
		t8 = reg(opc&7);
		REG_F = ((t8<<4)&0x10);
		t8 = (t8&0x80) | (t8>>1);
		REG_F |= (t8==0)?0x80:0;
		set_reg(opc&7, t8);
		return 8+(((opc&7)==6)?8:0);
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		set_reg(opc&7, rr(reg(opc&7)));
		return 8+(((opc&7)==6)?8:0);
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
		set_reg(opc&7, rl(reg(opc&7)));
		return 8+(((opc&7)==6)?8:0);
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
		set_reg(opc&7, rlc(reg(opc&7)));
		return 8+(((opc&7)==6)?8:0);
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
		set_reg(opc&7, rrc(reg(opc&7)));
		return 8+(((opc&7)==6)?8:0);
	default:
		printf("Unimpl opc 0xCB 0x%02X\n", opc);
		exit(1);
		break;	
	}
	return 0;
}

void sm83::push(u16 v)
{
	write(--SP, v>>8);
	write(--SP, v);
}

u16 sm83::pop()
{
	u16 R = read(SP++);
	R |= read(SP++) << 8;
	return R;
}

void sm83::rst(u8 v)
{
	push(PC);
	PC = v;
}

u8 sm83::rl(u8 v)
{
	u8 oldc = (REG_F>>4)&1;
	REG_F = ((v>>7)!=0)?0x10:0;
	v = (v<<1)|oldc;
	REG_F |= (v==0)?0x80:0;
	return v;
}

u8 sm83::rlc(u8 v)
{
	v = (v<<1)|(v>>7);
	REG_F = ((v&1)!=0)?0x10:0;
	REG_F |= (v==0)?0x80:0;
	return v;
}

u8 sm83::rr(u8 v)
{
	u8 oldc = (REG_F>>4)&1;
	REG_F = ((v&1)!=0)?0x10:0;
	v = (v>>1)|(oldc<<7);
	REG_F |= (v==0)?0x80:0;
	return v;
}

u8 sm83::rrc(u8 v)
{
	v = (v>>1)|(v<<7);
	REG_F = ((v>>7)!=0)?0x10:0;
	REG_F |= (v==0)?0x80:0;
	return v;
}

inline u8 getH(u8 v1, u8 v2, u8 c)
{
	return (((v1&0xf)+(v2&0xf)+c)<<1)&0x20;
}

u8 sm83::add(u8 v1, u8 v2, u32 c)
{
	u16 res = c;
	res += v1;
	res += v2;
	REG_F = getH(v1, v2, c);
	REG_F |= (res >> 4)&0x10;
	REG_F |= ((res&0xff)==0)?0x80:0; 
	return res;
}

u8 sm83::inc(u8 v)
{
	v++;
	REG_F &= 0x10;
	REG_F |= (v==0)?0x80:0;
	REG_F |= ((v&0xf)==0)?0x20:0;
	return v;
}

u8 sm83::dec(u8 v)
{
	v--;
	REG_F &= 0x10;
	REG_F |= 0x40;
	REG_F |= (v==0)?0x80:0;
	REG_F |= ((v&0xf)==0xf)?0x20:0;
	return v;
}

u16 sm83::add16(u16 a, u16 b)
{
	u32 res = a;
	res += b;
	REG_F &= 0x80;
	REG_F |= (res&0x10000)?0x10:0;
	if( (a&0xfff)+(b&0xfff) > 0xfff ) REG_F |= 0x20;
	return res;
}

void sm83::daa()
{
	u8 FLAG_H = (REG_F&0x20);
	u8 FLAG_S = (REG_F&0x40);
	u8 offset = 0;
	
	if( (!FLAG_S && (REG_A&0xf)>9) || FLAG_H ) offset = 6;	
	if( (!FLAG_S && (REG_A > 0x99)) || FLAG_C ) 
	{
		offset |= 0x60;
		REG_F |= 0x10;
	} else {
		REG_F &=~0x10;
	}

    	if( FLAG_S ) REG_A -= offset;
    	else REG_A += offset;
    	
    	REG_F &=~0xA0;
    	REG_F |= (REG_A==0)?0x80:0;
}

u16 sm83::addsp(u8 v)
{
	u16 R = (SP&0xff);
	R += v;
	REG_F = (R>>4)&0x10;
	REG_F |= (((SP&0xf)+(v&0xf))>0xf)?0x20:0;
	return SP + (s8)v;
}

u64 sm83_cycles[256] = 
{
4,12,8,8, 4,4,8,4, 20,8,8,8, 4,4,8,4,
4,12,8,8, 4,4,8,4, 12,8,8,8, 4,4,8,4,
8,12,8,8, 4,4,8,4, 8,8,8,8, 4,4,8,4,
8,12,8,8, 12,12,12,4, 8,8,8,8, 4,4,8,4,
4,4,4,4, 4,4,8,4, 4,4,4,4, 4,4,8,4,
4,4,4,4, 4,4,8,4, 4,4,4,4, 4,4,8,4,
4,4,4,4, 4,4,8,4, 4,4,4,4, 4,4,8,4,
8,8,8,8, 8,8,4,8, 4,4,4,4, 4,4,8,4,
4,4,4,4, 4,4,8,4, 4,4,4,4, 4,4,8,4,
4,4,4,4, 4,4,8,4, 4,4,4,4, 4,4,8,4,
4,4,4,4, 4,4,8,4, 4,4,4,4, 4,4,8,4,
4,4,4,4, 4,4,8,4, 4,4,4,4, 4,4,8,4,
8,12,12,16, 12,16,8,16, 8,16,12,0, 12,24,8,16,
8,12,12,4, 12,16,8,16, 8,16,12,4, 12,4,8,16,
12,12,8,4, 4,16,8,16, 16,4,16,4, 4,4,8,16,
12,12,8,4, 4,16,8,16, 12,8,16,4, 4,4,8,16, 
};
















