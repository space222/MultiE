// Epson S1C88
#include <print>
#include <functional>
#include "s1c88.h"

#define HL_ADDR ((EP<<16)|HL.v)
#define PC_ADDR ((CB<<16)|PC)
#define LL_ADDR ((EP<<16)|(BR<<8)|t)
#define IX_ADDR ((XP<<16)|IX)
#define IY_ADDR ((YP<<16)|IY)
#define fetch read(getpc()); PC+=1
#define A BA.b.l
#define B BA.b.h
#define H HL.b.h
#define L HL.b.l
#define fZ (SC&1)
#define fC ((SC>>1)&1)

u64 s1c88::step()
{
	u16 t = 0;
	u16 u = 0;
	u8 opc = fetch;
	std::println("${:X}:${:X}: opc ${:X}", CB, PC-1, opc);
	
	switch( opc )
	{
	case 0x00: A = add8(A, A, 0); return 2;
	case 0x01: A = add8(A, B, 0); return 2;
	case 0x02: t=fetch; A = add8(A, t, 0); return 2;

	case 0x10: A = add8(A, A^0xff, 1); return 2;	
	case 0x11: A = add8(A, B^0xff, 1); return 2;
	case 0x12: t=fetch; A = add8(A, t^0xff, 1); return 2;
	case 0x13: A = add8(A, read(HL_ADDR)^0xff, 1); return 2;
	
	
	case 0x20: A &= A; setz(A==0); setn(A&0x80); return 2;
	case 0x21: A &= B; setz(A==0); setn(A&0x80); return 2;
	case 0x22: A &= fetch; setz(A==0); setn(A&0x80); return 2;
	case 0x23: A &= read(HL_ADDR); setz(A==0); setn(A&0x80); return 2;
	case 0x24: t=fetch; A &= read(LL_ADDR); setz(A==0); setn(A&0x80); return 3;
	case 0x25: A &= read(fetch16()); setz(A==0); setn(A&0x80); return 4;
	case 0x26: A &= read(IX_ADDR);  setz(A==0); setn(A&0x80); return 2;
	case 0x27: A &= read(IY_ADDR);  setz(A==0); setn(A&0x80); return 2;
	case 0x28: A |= A; setz(A==0); setn(A&0x80); return 2;
	case 0x29: A |= B; setz(A==0); setn(A&0x80); return 2;
	case 0x2A: A |= fetch; setz(A==0); setn(A&0x80); return 2;
	case 0x2B: A |= read(HL_ADDR); setz(A==0); setn(A&0x80); return 2;
	case 0x2C: t=fetch; A |= read(LL_ADDR); setz(A==0); setn(A&0x80); return 3;
	case 0x2D: A |= read(fetch16()); setz(A==0); setn(A&0x80); return 4;
	case 0x2E: A |= read(IX_ADDR);  setz(A==0); setn(A&0x80); return 2;
	case 0x2F: A |= read(IY_ADDR);  setz(A==0); setn(A&0x80); return 2;

	case 0x32: t=fetch; add8(A, t^0xff, 1); return 2;

	case 0x37: add8(A, read(IY_ADDR)^0xff, 1); return 2;
	case 0x38: A ^= A; setz(A==0); setn(A&0x80); return 2;
	case 0x39: A ^= B; setz(A==0); setn(A&0x80); return 2;
	case 0x3A: A ^= fetch; setz(A==0); setn(A&0x80); return 2;
	case 0x3B: A ^= read(HL_ADDR); setz(A==0); setn(A&0x80); return 2;
	case 0x3C: t=fetch; A ^= read(LL_ADDR); setz(A==0); setn(A&0x80); return 3;
	case 0x3D: A ^= read(fetch16()); setz(A==0); setn(A&0x80); return 4;
	case 0x3E: A ^= read(IX_ADDR);  setz(A==0); setn(A&0x80); return 2;
	case 0x3F: A ^= read(IY_ADDR);  setz(A==0); setn(A&0x80); return 2;
	case 0x40: A = A; return 1;
	case 0x41: A = B; return 1;
	case 0x42: A = L; return 1;
	case 0x43: A = H; return 1;
	case 0x44: t=fetch; A = read(LL_ADDR); return 3;
	case 0x45: A = read(HL_ADDR); return 2;
	case 0x46: A = read(IX_ADDR); return 2;
	case 0x47: A = read(IY_ADDR); return 2;
	case 0x48: B = A; return 1;
	case 0x49: B = B; return 1;
	case 0x4A: B = L; return 1;
	case 0x4B: B = H; return 1;
	case 0x4C: t=fetch; B = read(LL_ADDR); return 3;
	case 0x4D: B = read(HL_ADDR); return 2;
	case 0x4E: B = read(IX_ADDR); return 2;
	case 0x4F: B = read(IY_ADDR); return 2;
	case 0x50: L = A; return 1;
	case 0x51: L = B; return 1;
	case 0x52: L = L; return 1;
	case 0x53: L = H; return 1;
	case 0x54: t=fetch; L = read(LL_ADDR); return 3;
	case 0x55: L = read(HL_ADDR); return 2;
	case 0x56: L = read(IX_ADDR); return 2;
	case 0x57: L = read(IY_ADDR); return 2;
	case 0x58: H = A; return 1;
	case 0x59: H = B; return 1;
	case 0x5A: H = L; return 1;
	case 0x5B: H = H; return 1;
	case 0x5C: t=fetch; H = read(LL_ADDR); return 3;
	case 0x5D: H = read(HL_ADDR); return 2;
	case 0x5E: H = read(IX_ADDR); return 2;
	case 0x5F: H = read(IY_ADDR); return 2;
	case 0x60: write(IX_ADDR, A); return 2;
	case 0x61: write(IX_ADDR, B); return 2;
	case 0x62: write(IX_ADDR, L); return 2;
	case 0x63: write(IX_ADDR, H); return 2;

	case 0x68: write(HL_ADDR, A); return 2;
	case 0x69: write(HL_ADDR, B); return 2;
	case 0x6A: write(HL_ADDR, L); return 2;
	case 0x6B: write(HL_ADDR, H); return 2;
	case 0x6C: t=fetch; write(HL_ADDR, read(LL_ADDR)); return 4;
	case 0x6D: write(HL_ADDR, read(HL_ADDR)); return 3;
	case 0x6E: write(HL_ADDR, read(IX_ADDR)); return 3;
	case 0x6F: write(HL_ADDR, read(IY_ADDR)); return 3;
	case 0x70: write(IY_ADDR, A); return 2;
	case 0x71: write(IY_ADDR, B); return 2;
	case 0x72: write(IY_ADDR, L); return 2;
	case 0x73: write(IY_ADDR, H); return 2;
	case 0x74: t=fetch; write(IY_ADDR, read(LL_ADDR)); return 4;
	case 0x75: write(IY_ADDR, read(HL_ADDR)); return 3;
	case 0x76: write(IY_ADDR, read(IX_ADDR)); return 3;
	case 0x77: write(IY_ADDR, read(IY_ADDR)); return 3;
	case 0x78: t=fetch; write(LL_ADDR, A); return 2;
	case 0x79: t=fetch; write(LL_ADDR, B); return 2;
	case 0x7A: t=fetch; write(LL_ADDR, L); return 2;
	case 0x7B: t=fetch; write(LL_ADDR, H); return 2;
	// $7C listed as invalid
	case 0x7D: t=fetch; write(LL_ADDR, read(HL_ADDR)); return 4;
	case 0x7E: t=fetch; write(LL_ADDR, read(IX_ADDR)); return 4;
	case 0x7F: t=fetch; write(LL_ADDR, read(IY_ADDR)); return 4;
	case 0x80: A+=1; setz(A==0); return 2;
	case 0x81: B+=1; setz(B==0); return 2;
	case 0x82: L+=1; setz(L==0); return 2;
	case 0x83: H+=1; setz(H==0); return 2;
	case 0x84: BR+=1; setz(BR==0); return 2;
	
	case 0x87: SP+=1; setz(SP==0); return 2;
	case 0x88: A-=1; setz(A==0); return 2;
	case 0x89: B-=1; setz(B==0); return 2;
	case 0x8A: L-=1; setz(L==0); return 2;
	case 0x8B: H-=1; setz(H==0); return 2;
	case 0x8C: BR-=1; setz(BR==0); return 2;

	case 0x8F: SP-=1; setz(SP==0); return 2;

	case 0x90: BA.v+=1; setz(BA.v==0); return 2;
	case 0x91: HL.v+=1; setz(HL.v==0); return 2;
	case 0x92: IX+=1; setz(IX==0); return 2;
	case 0x93: IY+=1; setz(IY==0); return 2;
	case 0x94: t=A&B; setz(t==0); setn(t&0x80); return 2;
	case 0x95: t=fetch; t&=read(HL_ADDR); setz(t==0); setn(t&0x80); return 3;
	case 0x96: t=A&fetch; setz(t==0); setn(t&0x80); return 2;
	case 0x97: t=B&fetch; setz(t==0); setn(t&0x80); return 2;
	case 0x98: BA.v-=1; setz(BA.v==0); return 2;
	case 0x99: HL.v-=1; setz(HL.v==0); return 2;
	case 0x9A: IX-=1; setz(IX==0); return 2;
	case 0x9B: IY-=1; setz(IY==0); return 2;
	case 0x9C: SC &= fetch; return 3;
	case 0x9D: SC |= fetch; return 3;
	case 0x9E: SC ^= fetch; return 3;
	case 0x9F: SC = fetch; return 3;
	case 0xA0: push16(BA.v); return 4;
	case 0xA1: push16(HL.v); return 4;
	case 0xA2: push16(IX); return 4;
	case 0xA3: push16(IY); return 4;
	case 0xA4: push(BR); return 3;
	case 0xA5: push(EP); return 4;
	case 0xA6: push(XP); push(YP); return 4;
	case 0xA7: push(SC); return 3;
	case 0xA8: BA.v = pop16(); return 3;
	case 0xA9: HL.v = pop16(); return 3;
	case 0xAA: IX = pop16(); return 3;
	case 0xAB: IY = pop16(); return 3;
	case 0xAC: BR = pop(); return 2;
	case 0xAD: EP = pop(); return 2;
	case 0xAE: YP = pop(); XP = pop(); return 3;
	case 0xAF: SC = pop(); return 2;
	case 0xB0: A = fetch; return 2;
	case 0xB1: B = fetch; return 2;
	case 0xB2: L = fetch; return 2;
	case 0xB3: H = fetch; return 2;
	case 0xB4: BR = fetch; return 2;
	case 0xB5: t = fetch; write(HL_ADDR, t); return 3;
	case 0xB6: t = fetch; write(IX_ADDR, t); return 3;
	case 0xB7: t = fetch; write(IY_ADDR, t); return 3;
	case 0xB8: BA.v = fetch16(); return 5;
	case 0xB9: HL.v = fetch16(); return 5;
	case 0xBA: IX = fetch16(); return 5;
	case 0xBB: IY = fetch16(); return 5;
	case 0xBC: t=fetch16(); write((EP<<16)+t, A); t++; write((EP<<16)+t, B); return 5;
	case 0xBD: t=fetch16(); write((EP<<16)+t, L); t++; write((EP<<16)+t, H); return 5;
	case 0xBE: t=fetch16(); write((EP<<16)+t, IX&0xff); t++; write((EP<<16)+t, IX>>8); return 5;
	case 0xBF: t=fetch16(); write((EP<<16)+t, IY&0xff); t++; write((EP<<16)+t, IY>>8); return 5;
	case 0xC0: t=fetch16(); BA.v = add16(BA.v, t, 0); return 3;
	case 0xC1: t=fetch16(); HL.v = add16(HL.v, t, 0); return 3;
	case 0xC2: t=fetch16(); IX = add16(IX, t, 0); return 3;
	case 0xC3: t=fetch16(); IY = add16(IY, t, 0); return 3;
	case 0xC4: A = fetch; B = fetch; return 3;
	case 0xC5: L = fetch; H = fetch; return 3;
	case 0xC6: IX = fetch16(); return 3;
	case 0xC7: IY = fetch16(); return 3;
	case 0xC8: std::swap(BA, HL); return 3;
	case 0xC9: t=BA.v; BA.v=IX; IX=t; return 3;
	case 0xCA: t=BA.v; BA.v=IY; IY=t; return 3;
	case 0xCB: t=BA.v; BA.v=SP; SP=t; return 3;
	case 0xCC: std::swap(A, B); return 2;
	case 0xCD:
		t = read(HL_ADDR);
		write(HL_ADDR, A);
		A = t;
		return 3;
	case 0xCE: return extCE();
	case 0xCF: return extCF();

	case 0xD4:
		t = fetch16();
		add16(BA.v, t^0xffff, 1);
		return 3;
	case 0xD5:
		t = fetch16();
		add16(HL.v, t^0xffff, 1);
		return 3;
	case 0xD6:
		t = fetch16();
		add16(IX, t^0xffff, 1);
		return 3;
	case 0xD7:
		t = fetch16();
		add16(IY, t^0xffff, 1);
		return 3;
	case 0xD8:
		t = fetch;
		u = fetch;
		u &= read(LL_ADDR);
		setz(u==0); setn(u&0x80);
		write(LL_ADDR, u);
		return 5;
	case 0xD9:
		t = fetch;
		u = fetch;
		u |= read(LL_ADDR);
		setz(u==0); setn(u&0x80);
		write(LL_ADDR, u);
		return 5;
		
	case 0xDC:
		t = fetch;
		u = fetch;
		u &= read(LL_ADDR);
		setz(u==0);
		setn(u&0x80);
		return 4;	
	case 0xDD:
		t = fetch;
		u = fetch;
		write(LL_ADDR, u);
		return 4;
	
	case 0xDE: A = (A&15)|(B<<4); return 2;
	case 0xDF: B = A>>4; A &= 15; return 2;
	
	case 0xE2:
		t = fetch;
		if( fZ )
		{
			push(CB);
			push16(PC);
			PC += s8(t) - 1;
			return 5;
		}
		NB = CB;
		return 2;
		
	case 0xE6:
		t = fetch;
		if( fZ )
		{
			PC += s8(t) - 1;
			return 5;
		}
		NB = CB;
		return 2;	
	case 0xE7:
		t = fetch;
		if( !fZ )
		{
			PC += s8(t) - 1;
			return 5;
		}
		NB = CB;
		return 2;
	
	case 0xEF: t=fetch16(); if( !fZ ) { PC += t - 1; CB = NB; } return 3;
	case 0xF0: t=fetch; push(CB); push16(PC); PC += s8(t) - 1; CB = NB; return 5;
	case 0xF1: t=fetch; PC += s8(t) - 1; CB = NB; return 2;
	case 0xF2: t=fetch16(); push(CB); push16(PC); PC += t - 1; CB = NB; return 6;
	case 0xF3: t=fetch16(); PC += t - 1; CB = NB; return 3;
	case 0xF4: PC = HL.v; CB = NB; return 2;
	case 0xF5: B-=1; setz(B==0); t=fetch; if( B ) { PC += s8(t) - 1; CB = NB; return 6; } NB = CB/*??*/; return 3;
	case 0xF6: A = (A<<4)|(A>>4); return 2;
	case 0xF7: t = read(HL_ADDR); t=(t<<4)|(t>>4); write(HL_ADDR, t); return 4;
	case 0xF8: PC = pop16(); NB = CB = pop(); return 4;
	case 0xF9: SC=pop(); PC=pop16(); NB=CB=pop(); return 5;
	case 0xFA: PC = pop16(); PC+=2; NB = CB = pop(); return 4;


	case 0xFD: t=fetch; PC = read16(t); CB=NB; return 4;

	case 0xFF: return 2; //nop	
	default:
		std::println("Unimpl opc ${:X}", opc);
		exit(1);
	}
}

u64 s1c88::extCE()
{
	u8 opc = fetch;
	u16 t=0, u=0;
	switch( opc )
	{
	case 0x20: t=fetch; A&=read(IX_ADDR+s8(t)); setz(A==0); setn(A&0x80); return 4;
	case 0x21: t=fetch; A&=read(IY_ADDR+s8(t)); setz(A==0); setn(A&0x80); return 4;
	case 0x22: A&=read(IX_ADDR+s8(L)); setz(A==0); setn(A&0x80); return 4;
	case 0x23: A&=read(IY_ADDR+s8(L)); setz(A==0); setn(A&0x80); return 4;
	case 0x24: t=read(HL_ADDR) & A; write(HL_ADDR, t); setz(t==0); setn(t&0x80); return 4;
	case 0x25: u=fetch; t=read(HL_ADDR)&u; write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 5;
	case 0x26: t=read(IX_ADDR)&read(HL_ADDR); write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 4;
	case 0x27: t=read(IY_ADDR)&read(HL_ADDR); write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 4;
	case 0x28: t=fetch; A|=read(IX_ADDR+s8(t)); setz(A==0); setn(A&0x80); return 4;
	case 0x29: t=fetch; A|=read(IY_ADDR+s8(t)); setz(A==0); setn(A&0x80); return 4;
	case 0x2A: A|=read(IX_ADDR+s8(L)); setz(A==0); setn(A&0x80); return 4;
	case 0x2B: A|=read(IY_ADDR+s8(L)); setz(A==0); setn(A&0x80); return 4;
	case 0x2C: t=read(HL_ADDR) | A; write(HL_ADDR, t); setz(t==0); setn(t&0x80); return 4;
	case 0x2D: u=fetch; t=read(HL_ADDR)|u; write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 5;
	case 0x2E: t=read(IX_ADDR)|read(HL_ADDR); write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 4;
	case 0x2F: t=read(IY_ADDR)|read(HL_ADDR); write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 4;

	case 0x38: t=fetch; A^=read(IX_ADDR+s8(t)); setz(A==0); setn(A&0x80); return 4;
	case 0x39: t=fetch; A^=read(IY_ADDR+s8(t)); setz(A==0); setn(A&0x80); return 4;
	case 0x3A: A^=read(IX_ADDR+s8(L)); setz(A==0); setn(A&0x80); return 4;
	case 0x3B: A^=read(IY_ADDR+s8(L)); setz(A==0); setn(A&0x80); return 4;
	case 0x3C: t=read(HL_ADDR) ^ A; write(HL_ADDR, t); setz(t==0); setn(t&0x80); return 4;
	case 0x3D: u=fetch; t=read(HL_ADDR)^u; write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 5;
	case 0x3E: t=read(IX_ADDR)^read(HL_ADDR); write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 4;
	case 0x3F: t=read(IY_ADDR)^read(HL_ADDR); write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 4;

	case 0x40: t=fetch; A = read(IX_ADDR+s8(t)); return 4;
	case 0x41: t=fetch; A = read(IY_ADDR+s8(t)); return 4;
	case 0x42: A = read(IX_ADDR+s8(L)); return 4;
	case 0x43: A = read(IY_ADDR+s8(L)); return 4;
	case 0x44: t=fetch; write(IX_ADDR+s8(t), A); return 4;
	case 0x45: t=fetch; write(IY_ADDR+s8(t), A); return 4;
	case 0x46: write(IX_ADDR+s8(L), A); return 4;
	case 0x47: write(IY_ADDR+s8(L), A); return 4;
	case 0x48: t=fetch; B = read(IX_ADDR+s8(t)); return 4;
	case 0x49: t=fetch; B = read(IY_ADDR+s8(t)); return 4;
	case 0x4A: B = read(IX_ADDR+s8(L)); return 4;
	case 0x4B: B = read(IY_ADDR+s8(L)); return 4;
	case 0x4C: t=fetch; write(IX_ADDR+s8(t), B); return 4;
	case 0x4D: t=fetch; write(IY_ADDR+s8(t), B); return 4;
	case 0x4E: write(IX_ADDR+s8(L), B); return 4;
	case 0x4F: write(IY_ADDR+s8(L), B); return 4;
	case 0x50: t=fetch; L = read(IX_ADDR+s8(t)); return 4;
	case 0x51: t=fetch; L = read(IY_ADDR+s8(t)); return 4;
	case 0x52: L = read(IX_ADDR+s8(L)); return 4;
	case 0x53: L = read(IY_ADDR+s8(L)); return 4;
	case 0x54: t=fetch; write(IX_ADDR+s8(t), L); return 4;
	case 0x55: t=fetch; write(IY_ADDR+s8(t), L); return 4;
	case 0x56: write(IX_ADDR+s8(L), L); return 4;
	case 0x57: write(IY_ADDR+s8(L), L); return 4;
	case 0x58: t=fetch; H = read(IX_ADDR+s8(t)); return 4;
	case 0x59: t=fetch; H = read(IY_ADDR+s8(t)); return 4;
	case 0x5A: H = read(IX_ADDR+s8(L)); return 4;
	case 0x5B: H = read(IY_ADDR+s8(L)); return 4;
	case 0x5C: t=fetch; write(IX_ADDR+s8(t), H); return 4;
	case 0x5D: t=fetch; write(IY_ADDR+s8(t), H); return 4;
	case 0x5E: write(IX_ADDR+s8(L), H); return 4;
	case 0x5F: write(IY_ADDR+s8(L), H); return 4;

	case 0x8C: setc(A&1); A >>= 1; setn(0); setz(A==0); return 3;
	case 0x8D: setc(B&1); B >>= 1; setn(0); setz(B==0); return 3;

	case 0x90: t = (fC?1:0); setc(A>>7); A = (A<<1)|t; setz(A==0); setn(A&0x80); return 3; 
	case 0x91: t = (fC?1:0); setc(B>>7); B = (B<<1)|t; setz(B==0); setn(B&0x80); return 3; 

	case 0x98: t = (fC?1:0)<<7; setc(A&1); A = (A>>1)|t; setz(A==0); setn(A&0x80); return 3; 
	case 0x99: t = (fC?1:0)<<7; setc(B&1); B = (B>>1)|t; setz(B==0); setn(B&0x80); return 3; 

	case 0xA0: A = ~A; setz(A==0); setn(A&0x80); return 3;
	case 0xA1: B = ~B; setz(B==0); setn(B&0x80); return 3;
	case 0xA2: t=fetch; u=read(LL_ADDR)^0xff; write(LL_ADDR,t); setz(t==0); setn(t&0x80); return 5;
	case 0xA3: t=read(HL_ADDR)^0xff; write(HL_ADDR,t); setz(t==0); setn(t&0x80); return 4;

	case 0xA8: BA.v = s16(A<<8)>>8; return 3;
	
	case 0xB0: B &= fetch; setz(B==0); setn(B&0x80); return 3;
	case 0xB1: L &= fetch; setz(L==0); setn(L&0x80); return 3;
	case 0xB2: H &= fetch; setz(H==0); setn(H&0x80); return 3;

	case 0xB4: B |= fetch; setz(B==0); setn(B&0x80); return 3;
	case 0xB5: L |= fetch; setz(L==0); setn(L&0x80); return 3;
	case 0xB6: H |= fetch; setz(H==0); setn(H&0x80); return 3;
	
	case 0xB8: B ^= fetch; setz(B==0); setn(B&0x80); return 3;
	case 0xB9: L ^= fetch; setz(L==0); setn(L&0x80); return 3;
	case 0xBA: H ^= fetch; setz(H==0); setn(H&0x80); return 3;

	case 0xBC: t = fetch; add8(B, t^0xff, 1); return 3;
	case 0xBD: t = fetch; add8(L, t^0xff, 1); return 3;
	case 0xBE: t = fetch; add8(H, t^0xff, 1); return 3;
	case 0xBF: t = fetch; add8(BR, t^0xff, 1); return 3;

	case 0xC0: A = BR; return 2;
	case 0xC1: A = SC; return 2;
	case 0xC2: BR = A; return 2;
	case 0xC3: SC = A; return 3;
	case 0xC4: NB = fetch; return 4;
	case 0xC5: EP = fetch; return 3;
	case 0xC6: XP = fetch; return 3;
	case 0xC7: YP = fetch; return 3;
	case 0xC8: A = NB; return 2;
	case 0xC9: A = EP; return 2;
	case 0xCA: A = XP; return 2;
	case 0xCB: A = YP; return 2;
	case 0xCC: NB = A; return 3;
	case 0xCD: EP = A; return 2;
	case 0xCE: XP = A; return 2;
	case 0xCF: YP = A; return 2;
	
	case 0xD0: t=fetch16(); A = read((EP<<16)|t); return 5;
	case 0xD1: t=fetch16(); B = read((EP<<16)|t); return 5;
	case 0xD2: t=fetch16(); L = read((EP<<16)|t); return 5;
	case 0xD3: t=fetch16(); H = read((EP<<16)|t); return 5;	
	case 0xD4: t=fetch16(); write((EP<<16)|t, A); return 5;
	case 0xD5: t=fetch16(); write((EP<<16)|t, B); return 5;
	case 0xD6: t=fetch16(); write((EP<<16)|t, L); return 5;
	case 0xD7: t=fetch16(); write((EP<<16)|t, H); return 5;
	
	case 0xD8: HL.v = L; HL.v *= A; setz(HL.v==0); setn(HL.v&0x8000); setc(0); setv(0); return 12;
	case 0xD9:
		if( A == 0 )
		{
			std::println("Div zero exception todo");
			return 13;
		}
		t = HL.v / A;
		H = HL.v % A;
		L = t;
		setv(0); //todo: overflow
		setn(L&0x80);
		setz(L==0);
		return 13;
	
	default:
		std::println("Unimpl CE opc ${:X}", opc);
		exit(1);
	}
}

u64 s1c88::extCF()
{
	u8 opc = fetch;
	u16 t=0, u=0;
	switch( opc )
	{
	
	case 0x38: add16(HL.v, BA.v^0xffff, 1); return 4;
	case 0x39: add16(HL.v, HL.v^0xffff, 1); return 4;
	case 0x3A: add16(HL.v, IX^0xffff, 1); return 4;
	case 0x3B: add16(HL.v, IY^0xffff, 1); return 4;
	
	case 0x6E: SP = fetch16(); return 4;
	
	case 0xB0: push(A); return 3;
	case 0xB1: push(B); return 3;
	case 0xB2: push(L); return 3;
	case 0xB3: push(H); return 3;	
	case 0xB4: A = pop(); return 3;
	case 0xB5: B = pop(); return 3;
	case 0xB6: L = pop(); return 3;
	case 0xB7: H = pop(); return 3;
	case 0xB8: push16(BA.v); push16(HL.v); push16(IX); push16(IY); push(BR); return 12;
	case 0xB9: push16(BA.v); push16(HL.v); push16(IX); push16(IY); push(BR); push(EP); push(XP); push(YP); return 15;

	case 0xBC: BR = pop(); IY = pop16(); IX = pop16(); HL.v = pop16(); BA.v = pop16(); return 11;
	case 0xBD: YP = pop(); XP = pop(); EP = pop(); BR = pop(); IY = pop16(); IX=pop16(); HL.v=pop16(); BA.v=pop16(); return 14;

	case 0xC4: write16(HL_ADDR, BA.v); return 5;
	case 0xC5: write16(HL_ADDR, HL.v); return 5;
	case 0xC6: write16(HL_ADDR, IX); return 5;
	case 0xC7: write16(HL_ADDR, IY); return 5;

	case 0xD0: BA.v = read16(IX_ADDR); return 5;
	case 0xD1: HL.v = read16(IX_ADDR); return 5;
	case 0xD2: IX = read16(IX_ADDR); return 5;
	case 0xD3: IY = read16(IX_ADDR); return 5;	
	case 0xD4: write16(IX_ADDR, BA.v); return 5;
	case 0xD5: write16(IX_ADDR, HL.v); return 5;
	case 0xD6: write16(IX_ADDR, IX); return 5;
	case 0xD7: write16(IX_ADDR, IY); return 5;
	case 0xD8: BA.v = read16(IY_ADDR); return 5;
	case 0xD9: HL.v = read16(IY_ADDR); return 5;
	case 0xDA: IX = read16(IY_ADDR); return 5;
	case 0xDB: IY = read16(IY_ADDR); return 5;	
	case 0xDC: write16(IY_ADDR, BA.v); return 5;
	case 0xDD: write16(IY_ADDR, HL.v); return 5;
	case 0xDE: write16(IY_ADDR, IX); return 5;
	case 0xDF: write16(IY_ADDR, IY); return 5;
	case 0xE0: BA.v = BA.v; return 2;
	case 0xE1: BA.v = HL.v; return 2;
	case 0xE2: BA.v = IX; return 2;
	case 0xE3: BA.v = IY; return 2;
	case 0xE4: HL.v = BA.v; return 2;
	case 0xE5: HL.v = HL.v; return 2;
	case 0xE6: HL.v = IX; return 2;
	case 0xE7: HL.v = IY; return 2;
	case 0xE8: IX = BA.v; return 2;
	case 0xE9: IX = HL.v; return 2;
	case 0xEA: IX = IX; return 2;
	case 0xEB: IX = IY; return 2;
	case 0xEC: IY = BA.v; return 2;
	case 0xED: IY = HL.v; return 2;	
	case 0xEE: IY = IX; return 2;
	case 0xEF: IY = IY; return 2;
	case 0xF0: SP = BA.v; return 2;
	case 0xF1: SP = HL.v; return 2;
	case 0xF2: SP = IX; return 2;
	case 0xF3: SP = IY; return 2;
	case 0xF4: HL.v = SP; return 2;
	case 0xF5: HL.v = PC; return 2;
	case 0xF8: BA.v = SP; return 2;
	case 0xF9: BA.v = PC; return 2;
	case 0xFA: IX = SP; return 2;
	case 0xFE: IY = SP; return 2;
	
	default:
		std::println("Unimpl CF opc ${:X}", opc);
		exit(1);
	}
}



