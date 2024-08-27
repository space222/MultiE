#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "cp1600.h"

//#include "intellivision.h"
//extern console* sys;
//#define ITV (*dynamic_cast<intellivision*>(sys))

#define BITC (f&0x10)
#define BITO (f&0x20)
#define BITZ (f&0x40)
#define BITS (f&0x80)

#define setC(a) f = (f&~0x10)|((a)?0x10:0)
#define setO(a) f = (f&~0x20)|((a)?0x20:0)
#define setZ(a) f = (f&~0x40)|((a)?0x40:0)
#define setS(a) f = (f&~0x80)|((a)?0x80:0)
#define PC r[7]
#define SP r[6]

void cp1600::mvo(u16 opc)
{
	u16 temp = 0;
	u8 mode = (opc>>3) & 7;
	switch( (opc>>3) & 7 )
	{
	case 0: temp = read(PC++); write(temp, r[opc&7]); break;
	case 1:
	case 2:
	case 3: write(r[mode], r[opc&7]); break;
	case 4:
	case 5:
	case 6:
	case 7: temp = r[mode]++; write(temp, r[opc&7]); break;
	//case 6:	temp = r[6]++; write(temp, r[opc&7]); break;
	}
}

void cp1600::add(u16 opc)
{
	u16 val = mem(opc);
	int R = (opc&7);
	u32 res = r[R];
	res += val;
	setO((r[R]^res) & (val^res) & 0x8000);
	setC(res&0x10000);
	r[R] = res;
	setS(r[R]&0x8000);
	setZ(r[R]==0);
}

void cp1600::sub(u16 opc)
{
	u16 val = (mem(opc)^0xffff) + 1;
	int R = (opc&7);
	u32 res = r[R];
	res += val;
	//res++;
	setO((r[R]^res) & (val^res) & 0x8000);
	setC(res&0x10000);
	r[R] = res;
	setS(r[R]&0x8000);
	setZ(r[R]==0);
}

void cp1600::cmp(u16 opc)
{
	u16 val = (mem(opc)^0xffff)+1;
	int R = (opc&7);
	u32 res = r[R];
	res += val;
	//res++;
	setO((r[R]^res) & (val^res) & 0x8000);
	setC(res&0x10000);
	setS(res&0x8000);
	setZ((res&0xffff)==0);
}

void cp1600::op_and(u16 opc)
{
	u16 val = mem(opc);
	int R = (opc&7);
	r[R] &= val;
	setS(r[R]&0x8000);
	setZ(r[R]==0);
}

void cp1600::op_xor(u16 opc)
{
	u16 val = mem(opc);
	int R = (opc&7);
	r[R] ^= val;
	setS(r[R]&0x8000);
	setZ(r[R]==0);
}

void cp1600::branch(u16 opc)
{
	const u16 disp = read(PC++) ^ ((opc&0x20)?0xffff:0);
	bool br = false;
	icyc = 7;
	
	if( opc & 0x10 )
	{
		printf("branch ex set, but unconnected in Intellivision\n");
		return;
	}
	
	switch( opc & 7 )
	{
	case 0: br = true; break;
	case 1: br = BITC; break;
	case 2: br = BITO; break;
	case 3: br = !BITS; break;
	case 4: br = BITZ; break;
	case 5: br = (BITS?1:0) ^ (BITO?1:0); break;
	case 6: br = BITZ || ((BITS?1:0) ^ (BITO?1:0)); break;
	case 7: br = (BITC?1:0) ^ (BITS?1:0); break;
	}
	
	if( opc & 8 ) br = !br;	
	if( !br ) return;
	icyc = 9;
	
	PC += disp;
}

void cp1600::movr(u16 opc)
{
	const u8 ddd = opc&7;
	const u8 sss = (opc>>3)&7;
	if( ddd >= 6 ) icyc++;
	r[ddd] = r[sss];
	setS(r[ddd]&0x8000);
	setZ(r[ddd]==0);
}

void cp1600::xorr(u16 opc)
{
	const u8 ddd = opc&7;
	const u8 sss = (opc>>3)&7;
	r[ddd] ^= r[sss];
	setS(r[ddd]&0x8000);
	setZ(r[ddd]==0);
}

void cp1600::andr(u16 opc)
{
	const u8 ddd = opc&7;
	const u8 sss = (opc>>3)&7;
	r[ddd] &= r[sss];
	setS(r[ddd]&0x8000);
	setZ(r[ddd]==0);
}

void cp1600::addr(u16 opc)
{
	const u8 ddd = opc&7;
	const u8 sss = (opc>>3)&7;
	u32 res = r[ddd];
	res += r[sss];
	setO((r[ddd]^res) & (r[sss]^res) & 0x8000);
	setC(res&0x10000);
	r[ddd] = res;
	setS(r[ddd]&0x8000);
	setZ(r[ddd]==0);
}

void cp1600::subr(u16 opc)
{
	const u8 ddd = opc&7;
	const u8 sss = (opc>>3)&7;
	u16 val = (r[sss]^0xffff) + 1;
	u32 res = r[ddd];
	res += val;
	//res++;
	setO((r[ddd]^res) & (val^res) & 0x8000);
	setC(res&0x10000);
	r[ddd] = res;
	setS(r[ddd]&0x8000);
	setZ(r[ddd]==0);
}

void cp1600::cmpr(u16 opc)
{
	const u8 ddd = opc&7;
	const u8 sss = (opc>>3)&7;
	u16 val = (r[sss]^0xffff) + 1;
	u32 res = r[ddd];
	res += val;
	//res++;
	setO((r[ddd]^res) & (val^res) & 0x8000);
	setC(res&0x10000);
	setS(res&0x8000);
	setZ((res&0xffff)==0);
}

void cp1600::shift(u16 opc)
{
	const u8 dd = (opc&3);
	const u16 oldC = BITC ? 1 : 0;
	const u16 oldO = BITO ? 1 : 0;

	interruptible = false;
	icyc = 6 + ((opc&4)?2:0);

	switch( (opc>>3)&7 )
	{
	case 0: // swap
		if( opc & 4 )
		{
			r[dd] = (r[dd]<<8)|(r[dd]&0xff);
		} else {
			r[dd] = (r[dd]<<8)|(r[dd]>>8);		
		}
		setS(r[dd]&0x80);
		setZ(r[dd]==0);
		break;
	case 1: // sll
		r[dd] <<= ( opc & 4 ) ? 2 : 1;
		setS(r[dd]&0x8000);
		setZ(r[dd]==0);
		break;
	case 2: // rlc
		setC(r[dd]&0x8000);
		if( opc & 4 )
		{
			setO(r[dd]&0x4000);
			r[dd] <<= 2;
			r[dd] |= oldC<<1;
			r[dd] |= oldO;	
		} else {
			r[dd] <<= 1;
			r[dd] |= oldC;
		}
		setS(r[dd]&0x8000);
		setZ(r[dd]==0);
		break;
	case 3: // sllc
		setC(r[dd]&0x8000);
		if( opc & 4 )
		{
			setO(r[dd]&0x4000);
			r[dd] <<= 2;
		} else {
			r[dd] <<= 1;
		}
		setS(r[dd]&0x8000);
		setZ(r[dd]==0);
		break;
	case 4: // slr
		r[dd] >>= (opc&4)?2:1;
		setS(r[dd]&0x80);
		setZ(r[dd]==0);
		break;
	case 5: // sar
		if( opc & 4 )
		{
			r[dd] = ((r[dd]&0x8000)?0xc000:0)|(r[dd]>>2);
		} else {
			r[dd] = (r[dd]&0x8000)|(r[dd]>>1);
		}
		setS(r[dd]&0x80);
		setZ(r[dd]==0);
		break;
	case 6: // rrc
		if( opc & 4 )
		{
			setC(r[dd]&1);
			setO(r[dd]&2);
			r[dd] >>= 2;
			r[dd] |= oldC<<14;
			r[dd] |= oldO<<15;
		} else {
			setC(r[dd]&1);
			r[dd] >>= 1;
			r[dd] |= oldC<<15;
		}
		setS(r[dd]&0x80);
		setZ(r[dd]==0);
		break;
	case 7: // sarc
		if( opc & 4 )
		{
			setC(r[dd]&1);
			setO(r[dd]&2);
			r[dd] = ((r[dd]&0x8000)?0xc000:0)|(r[dd]>>2);
		} else {
			setC(r[dd]&1);
			r[dd] = (r[dd]&0x8000)|(r[dd]>>2);
		}
		setS(r[dd]&0x80);
		setZ(r[dd]==0);
		break;
	}
}

void cp1600::incr(u16 opc)
{
	r[opc&7]++;
	setS(r[opc&7]&0x8000);
	setZ(r[opc&7]==0);
}

void cp1600::decr(u16 opc)
{
	r[opc&7]--;
	setS(r[opc&7]&0x8000);
	setZ(r[opc&7]==0);
}

void cp1600::adcr(u16 opc)
{
	const u8 ddd = opc&7;
	if( BITC )
	{
		u32 res = r[ddd];
		res += 1;
		setO((res^r[ddd]) & (res^1) & 0x8000);
		setC(res&0x10000);
		r[ddd] = res;
	} else {
		setO(0);
		setC(0);
	}
	setS(r[ddd]&0x8000);
	setZ(r[ddd]==0);
}

void cp1600::comr(u16 opc)
{
	const u8 ddd = opc&7;
	r[ddd] ^= 0xffff;
	setS(r[ddd]&0x8000);
	setZ(r[ddd]==0);
}

void cp1600::negr(u16 opc)
{
	const u8 ddd = opc&7;
	setC(r[ddd]==0);
	setO(r[ddd]==0x8000);
	r[ddd] = (r[ddd]^0xffff) + 1;
	setS(r[ddd]&0x8000);
	setZ(r[ddd]==0);
}

void cp1600::gswd(u16 opc)
{
	const u8 ddd = opc&7;
	if( ddd < 4 )
	{
		r[ddd] = (f<<8)|f;
		return;
	}
	if( ddd < 6 ) return;
	//todo: trap?
}

void cp1600::rswd(u16 opc)
{
	f = r[opc&7] & 0xf0;
}

void cp1600::jmp()
{
	const u16 a = read(PC++) & 0x3FF;
	const u16 b = read(PC++) & 0x3FF;
	const u16 target = (((a>>2)&0x3F)<<10) | b;
	icyc = 12;
	
	if( (a&0x300) != 0x300 ) r[4 + (a>>8)] = PC;
	PC = target;
	if( (a&3) == 1 ) I = true;
	else if( (a&3) == 2 ) I = false;
}

void cp1600::internal_ctrl(u16 opc)
{
	icyc = 4;
	switch( opc & 7 )
	{
	case 0: printf("cp1600 halt unimpl.\n"); break; // hlt 
	case 2: I = true; interruptible = false; break; // eis
	case 3: I = false; interruptible = false; break; // dis
	case 4: jmp(); break;
	case 5: interruptible = false; break; // tci (unconnected on Intellivision)
	case 6: setC(0); interruptible = false; break; // clc
	case 7: setC(1); interruptible = false; break; // stc
	}
}

u32 cp1600::step()
{
	u16 opc = read(PC++);
	//printf("$%04X: opc = $%04X\n", PC-1, opc);
	opc &= 0x3FF;
	icyc = 6;
	
	if( interruptible && busrq )
	{
		busak = true;
		return stamp += 1;
	}
	
	interruptible = true;
	
	if( opc == 1 )
	{  // sdbd
		D = true;
		return stamp += 4;
	}
		
	if( opc & (1<<9) )
	{
		switch( (opc>>6) & 7 )
		{
		case 0: branch(opc); break;
		case 1: mvo(opc); break;
		case 2: r[opc&7] = mem(opc); break; //mvi
		case 3: add(opc); break;
		case 4: sub(opc); break;
		case 5: cmp(opc); break;
		case 6: op_and(opc); break;
		case 7: op_xor(opc); break;		
		}
	} else {
		switch( (opc>>6) & 7 )
		{
		case 0: 
			switch( (opc>>3) & 7 )
			{
			case 0: internal_ctrl(opc); break;
			case 1: incr(opc); break;
			case 2: decr(opc); break;
			case 3: comr(opc); break;
			case 4: negr(opc); break;
			case 5: adcr(opc); break;
			case 6: gswd(opc); break;
			case 7: rswd(opc); break;
			}		
			break;
		case 1: shift(opc); break;
		case 2: movr(opc); break;
		case 3: addr(opc); break;
		case 4: subr(opc); break;
		case 5: cmpr(opc); break;
		case 6: andr(opc); break;
		case 7: xorr(opc); break;		
		}
	}
		
	if( interruptible && I && irq_asserted )
	{
		irq_asserted = false; // pretend the full ack happened
		write(SP, PC);
		SP++;
		PC = 0x1004;
		icyc += 7;
	}
	
	D = false;
	stamp += icyc;
	return icyc;
}

void cp1600::reset()
{
	PC = 0x1000;
	D = I = irq_asserted = false;
	interruptible = true;
	//halted = false;
	//halt_cnt = 0;
}

u16 cp1600::mem(u16 opc)
{
	u16 res = 0;
	u8 mode = (opc>>3) & 7;
	u16 temp = 0;
	switch( mode )
	{
	case 0: temp = read(PC++); res = read(temp); if( D ) { icyc+=2; res = (res&0xff)|(read(temp+1)<<8); } break;
	case 1:
	case 2:
	case 3: res = read(r[mode]); if( D ) { icyc+=2; res = (res<<8)|(res&0xff); } break;
	case 4: 
	case 5:
	case 7: res = read(r[mode]++); if( D ) { icyc+=2; res = (res&0xff)|(read(r[mode]++)<<8); } break;
	case 6:	res = read(--r[6]); if( D ) { printf("D used for stack access\n"); exit(1); } break; // ?? for D
	}
	return res;
}


