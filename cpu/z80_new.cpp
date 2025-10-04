#include <print>
#include "z80_new.h"

//#define BIT(a) (1u<<(a))

#define F r[6]
#define FLAG_S BIT(7)
#define FLAG_Z BIT(6)
#define FLAG_5 BIT(5)
#define FLAG_H BIT(4)
#define FLAG_3 BIT(3)
#define FLAG_P BIT(2)
#define FLAG_V BIT(2)
#define FLAG_N BIT(1)
#define FLAG_C BIT(0)

#define clearnc F &= ~3

#define A r[7]

#define B r[0]
#define C r[1]
#define D r[2]
#define E r[3]
#define H r[4]
#define L r[5]

#define BC ( (r[0]<<8)|r[1] )
#define DE ( (r[2]<<8)|r[3] )
#define HL ( (r[4]<<8)|r[5] )
#define AF ( (r[7]<<8)|r[6] )
#define USE_X 0xDD
#define USE_Y 0xFD

#define HL_OVERRIDE ((prefix==USE_X)?ix:((prefix==USE_Y)?iy:HL))

#define setBC(a) B=((a)>>8); C=((a)&0xff)
#define setDE(a) D=((a)>>8); E=((a)&0xff)

void z80n::step_exec()
{
	R = (R&0x80)|((R+1)&0x7f);

	if( iff1 && irq_line && !irq_blocked )
	{
		push16(pc);
		pc = 0x38;
		iff1 = iff2 = false;
		halted = false;
	}
	irq_blocked = false;
	
	if( halted ) return;

	prefix = 0;
	u8 opc = read8(pc);
	pc += 1;
	
	while( opc == USE_X || opc == USE_Y )
	{
		prefix = opc;
		opc = read8(pc);
		pc += 1;
		stamp += 4;
	}
	
	if( opc == 0xED )
	{
		opc = read8(pc);
		pc += 1;
		prefix = 0;
		exec_ed(opc);
		return;
	}
	
	if( opc == 0xCB )
	{
		if( prefix ) { load_disp(); }
		opc = read8(pc);
		pc += 1;
		exec_cb(opc);
		return;	
	}
	
	const u32 x = opc>>6;
	const u32 y = (opc>>3)&7;
	const u32 z = opc&7;
	const u32 p = y>>1;
	const u32 q = y&1;

	if( x == 1 )
	{
		if( z == 6 && y == 6 ) { halted = true; return; }
		if( prefix && (z == 6 || y == 6) ) { load_disp(); }
		table_r(y, table_r(z, y!=6), z!=6);	
		return;
	}

	if( x == 2 )
	{
		if( prefix && z == 6 ) { load_disp(); }
		alu(y, table_r(z));
		return;
	}
	
	if( x == 3 )
	{
		if( z == 0 ) { if( cond(y) ) { pc = pop16(); } return; }
		if( z == 1 )
		{
			if( q == 0 ) { rp2(p, pop16()); return; }
			if( p == 0 ) { pc = pop16(); return; }
			if( p == 1 ) { for(u32 i = 0; i < 6; ++i) { std::swap(r[i], rbank[i]); } return; }
			if( p == 2 ) { pc = HL_OVERRIDE; return; }
			if( p == 3 ) { sp = HL_OVERRIDE; return; }
			return;
		}
		if( z == 2 ) { u16 i = imm16(); if( cond(y) ) { pc = i; } return; }
		if( z == 3 )
		{
			if( y == 0 ) { u16 i = imm16(); pc = i; return; }
			// y == 1 is CB prefix
			if( y == 2 ) { out8((A<<8)|imm8(), A); return; }
			if( y == 3 ) { A = in8((A<<8)|imm8()); return; }
			if( y == 4 ) { u16 t = read16(sp); write16(sp, HL_OVERRIDE); setHL(t); return; } // ex (sp), hl (prefixable!)
			if( y == 5 ) { std::swap(D, H); std::swap(E, L); return; } // ex de, hl (not prefixable)
			if( y == 6 ) { iff1 = iff2 = false; return; }
			if( y == 7 ) { irq_blocked = true; iff1 = iff2 = true; return; }		
		}
		if( z == 4 ) { u16 i = imm16(); if( cond(y) ) { push16(pc); pc = i; } return; }
		if( z == 5 )
		{
			if( q == 0 ) { push16(rp2(p)); return; }
			if( p == 0 ) { u16 i = imm16(); push16(pc); pc = i; return; }
			// p=1 DD, p=2 ED, p=3 FD
			return;
		}
		if( z == 6 ) { alu(y, imm8()); return; }
		if( z == 7 ) { rst(y*8); return; }
		return;
	}
	
	// x==0
	if( z == 0 )
	{
		if( y == 0 ) { return; } // nop
		if( y == 1 ) { std::swap(r[7], rbank[7]); std::swap(r[6], rbank[6]); return; }
		if( y == 2 ) { s8 i = imm8(); B-=1; if( B ) { pc += i; } return; }
		s8 i = imm8(); if( y==3 || cond(y-4) ) { pc += i; } return;
		return;
	}
	
	if( z == 1 )
	{
		if( q == 0 ) { rp(p, imm16()); return; }
		u16 a = HL_OVERRIDE;
		u16 b = rp(p);
		u32 t = a;
		t += b;
		setH( (a&0xfff)+(b&0xfff) > 0xfff );
		setC( t>>16 );
		setN(0);
		setHL(t);
		set53(t>>8);
		return;
	}
	
	if( z == 2 )
	{
		if( q == 0 ) switch( p )
		{
		case 0: write8(BC, A); return;
		case 1: write8(DE, A); return;
		case 2: write16(imm16(), HL_OVERRIDE); return;
		case 3: write8(imm16(), A); return;		
		}
		switch( p )
		{
		case 0: A = read8(BC); return;
		case 1: A = read8(DE); return;
		case 2: setHL(read16(imm16())); return;
		case 3: A = read16(imm16()); return;
		}
		std::println("error z2, q1, p{}", p);
		exit(1);
	}
	
	if( z == 3 )
	{ // 16bit inc/dec
		u16 t = rp(p);
		rp(p, q ? t-1 : t+1);	
		return;
	}
	
	if( z == 4 ) { if(prefix&&y==6) { load_disp(); } table_r(y, inc8(table_r(y))); return; }
	if( z == 5 ) { if(prefix&&y==6) { load_disp(); } table_r(y, dec8(table_r(y))); return; }	
	if( z == 6 ) { if(prefix&&y==6) { load_disp(); } table_r(y, imm8()); return; }
	
	// z == 7
	if( y == 0 )
	{ // RLCA
		A = (A<<1)|(A>>7);
		setC(A&1);
		set53(A);
		setN(0);
		setH(0);
		return;
	}
	if( y == 1 )
	{ // RRCA
		A = (A>>1)|(A<<7);
		setC(A>>7);
		set53(A);
		setN(0);
		setH(0);
		return;	
	}
	if( y == 2 )
	{ // RLA
		u8 oldC = F&FLAG_C;
		setC(A>>7);
		A = (A<<1)|oldC;
		set53(A);
		setN(0);
		setH(0);
		return;
	}
	if( y == 3 )
	{ // RRA
		u8 oldC = F&FLAG_C;
		setC(A&1);
		A = (A>>1)|(oldC<<7);
		set53(A);
		setN(0);
		setH(0);
		return;
	}
	if( y == 4 )
	{
		daa();
		return;
	}
	if( y == 5 ) { A ^= 0xff; set53(A); F |= FLAG_N|FLAG_H; return; }
	if( y == 6 ) { F |= FLAG_C; F &= ~(FLAG_N|FLAG_H); set53(A); return; }
	if( y == 7 ) { setH(F&FLAG_C); F ^= FLAG_C; F &= ~FLAG_N; set53(A); return; }
	std::println("Error: got thru z80n::step, x{} y{} z{}", x, y, z);
	exit(1);
}

u8 z80n::inc8(u8 v)
{
	v += 1;
	set53(v);
	setszp(v);
	setN(0);
	setH((v&15)==0);
	setV(v==0x80);
	return v;
}

u8 z80n::dec8(u8 v)
{
	v -= 1;
	set53(v);
	setszp(v);
	setN(1);
	setH((v&15)==15);
	setV(v==0x7f);
	return v;
}

void z80n::alu(const u32 op, u8 z)
{
	u8 cf = F&FLAG_C;
	u8 a = A;
	u8 b = z;
	switch( op )
	{
	case 0:{ // ADD A, z
		u16 res = a;
		res += b;
		setszp(res&0xff);
		setV((res^a)&(res^b)&0x80);
		setN(0);
		setH((a&15)+(b&15)>15);
		setC(res>>8);
		A = res;
		set53(A);
		}break;
	case 1:{ // ADC A, z
		u16 res = a;
		res += b;
		res += cf;
		setszp(res&0xff);
		setV((res^a)&(res^b)&0x80);
		setN(0);
		setH((a&15)+(b&15)+cf>15);
		setC(res>>8);
		A = res;		
		set53(A);
		}break;
	case 2:{ // SUB A, z
		b ^= 0xff;
		u16 res = a;
		res += b;
		res += 1;
		setszp(res&0xff);
		setV((res^a)&(res^b)&0x80);
		setN(1);
		setH((a&15)+(b&15)+1>15);
		setC(res>>8);
		F ^= FLAG_C|FLAG_H;
		A = res;			
		set53(A);
		}break;
	case 3:{ // SBC A, z
		b ^= 0xff; cf ^= 1;
		u16 res = a;
		res += b;
		res += cf;
		setszp(res&0xff);
		setV((res^a)&(res^b)&0x80);
		setN(1);
		setH((a&15)+(b&15)+cf>15);
		setC(res>>8);
		F ^= FLAG_C|FLAG_H;
		A = res;		
		set53(A);
		}break;
	case 4:{ // AND A, z
		A &= z;
		setszp(A);
		setH(1);
		clearnc;
		set53(A);
		}break;
	case 5:{ // XOR A, z
		A ^= z;
		setszp(A);
		setH(0);
		clearnc;
		set53(A);
		}break;
	case 6:{ // OR A, z
		A |= z;
		setszp(A);
		setH(0);
		clearnc;
		set53(A);
		}break;
	case 7:{ // CP A, z
		b ^= 0xff;
		u16 res = a;
		res += b;
		res += 1;
		setszp(res&0xff);
		setV((res^a)&(res^b)&0x80);
		setN(1);
		setH((a&15)+(b&15)+1>15);
		setC(res>>8);
		F ^= FLAG_C|FLAG_H;
		set53(b^0xff);
		}break;
	default:
		std::println("alu error op = {}", op);
		exit(1);
	}
}

u8 z80n::table_r(u32 reg, bool use_override)
{
	if( !prefix || !use_override ) { if( reg == 6 ) return read8(HL); return r[reg]; }		
	if( reg == 4 && prefix == USE_X ) return ix>>8;
	if( reg == 4 && prefix == USE_Y ) return iy>>8;
	if( reg == 5 && prefix == USE_X ) return ix;
	if( reg == 5 && prefix == USE_Y ) return iy;
	if( reg == 6 && prefix == USE_X ) return read8(ix + disp8);
	if( reg == 6 && prefix == USE_Y ) return read8(iy + disp8);
	return r[reg];
}

void z80n::table_r(u32 reg, u8 v, bool use_override)
{
	if( !prefix || !use_override ) { if( reg == 6 ) write8(HL,v); else r[reg] = v; return; }
	if( reg == 4 && prefix == USE_X ) { ix &= 0x00ff; ix |= v<<8; return; }
	if( reg == 4 && prefix == USE_Y ) { iy &= 0x00ff; iy |= v<<8; return; }
	if( reg == 5 && prefix == USE_X ) { ix &= 0xff00; ix |= v; return; }
	if( reg == 5 && prefix == USE_Y ) { iy &= 0xff00; iy |= v; return; }
	if( reg == 6 && prefix == USE_X ) { write8(ix + disp8, v); return; }
	if( reg == 6 && prefix == USE_Y ) { write8(iy + disp8, v); return; }
	r[reg] = v;
}

u16 z80n::rp(u32 reg)
{
	if( reg == 2 )
	{
		if( prefix == USE_X ) return ix;
		if( prefix == USE_Y ) return iy;
	}
	switch( reg )
	{
	case 0: return BC;
	case 1: return DE;
	case 2: return HL;
	case 3: return sp;
	}
	std::println("error in rp, reg = {}", reg);
	exit(1);
	return 0xcafe;
}

void z80n::rp(u32 reg, u16 v)
{
	if( reg == 2 )
	{
		if( prefix == USE_X ) { ix = v; return; }
		if( prefix == USE_Y ) { iy = v; return; }
	}
	switch( reg )
	{
	case 0: B = v>>8; C = v; return;
	case 1: D = v>>8; E = v; return;
	case 2: H = v>>8; L = v; return;
	case 3: sp = v; return;
	}
	std::println("error in set_rp, reg = {}", reg);
	exit(1);
}

u16 z80n::rp2(u32 reg)
{
	if( reg == 2 )
	{
		if( prefix == USE_X ) return ix;
		if( prefix == USE_Y ) return iy;
	}
	switch( reg )
	{
	case 0: return BC;
	case 1: return DE;
	case 2: return HL;
	case 3: return AF;
	}
	std::println("error in rp2, reg = {}", reg);
	exit(1);
	return 0xcafe;
}

void z80n::rp2(u32 reg, u16 v)
{
	if( reg == 2 )
	{
		if( prefix == USE_X ) { ix = v; return; }
		if( prefix == USE_Y ) { iy = v; return; }
	}
	switch( reg )
	{
	case 0: B = v>>8; C = v; return;
	case 1: D = v>>8; E = v; return;
	case 2: H = v>>8; L = v; return;
	case 3: A = v>>8; F = v; return;
	}
	std::println("error in set_rp2, reg = {}", reg);
	exit(1);
}

bool z80n::cond(u8 cc)
{
	switch(cc)
	{
	case 0: return !(F & FLAG_Z);
	case 1: return F & FLAG_Z;
	case 2: return !(F & FLAG_C);
	case 3: return F & FLAG_C;
	case 4: return !(F & FLAG_P);
	case 5: return F & FLAG_P;
	case 6: return !(F & FLAG_S);
	case 7: return F & FLAG_S;	
	}
	std::println("error in cond = {}", cc);
	exit(1);
	return true;
}

u8 z80n::setszp(const u8 v)
{
	F &= ~(FLAG_Z|FLAG_S|FLAG_P);
	F |= ((v==0)?FLAG_Z:0);
	F |= ((v&0x80)?FLAG_S:0);
	F |= ((std::popcount(v)&1)?0:FLAG_P);
	return v;
}

void z80n::setH(u8 v) { F &= ~FLAG_H; if(v) F |= FLAG_H; }
void z80n::setC(u8 v) { F &= ~FLAG_C; if(v) F |= FLAG_C; }
void z80n::setV(u8 v) { F &= ~FLAG_V; if(v) F |= FLAG_V; }
void z80n::setN(u8 v) { F &= ~FLAG_N; if(v) F |= FLAG_N; }
void z80n::setZ(u8 v) { F &= ~FLAG_Z; if(v) F |= FLAG_Z; }

void z80n::set53(u8 v) { F &= ~(FLAG_3|FLAG_5); F |= v & (FLAG_3|FLAG_5); }

u8 z80n::imm8() { u8 t = read8(pc); pc += 1; return t; }
u16 z80n::imm16() { u16 t = read8(pc); t |= read8(pc+1)<<8; pc += 2; return t; }

void z80n::push16(u16 v)
{
	sp -= 1;
	write8(sp, v>>8);
	sp -= 1;
	write8(sp, v);
}

u16 z80n::pop16()
{
	u16 t = read8(sp);
	sp += 1;
	t |= read8(sp)<<8;
	sp += 1;
	return t;
}

void z80n::rst(u8 a)
{
	push16(pc);
	pc = a;
}

void z80n::reset()
{
	pc = 0;
	sp = 0xfffe;
}

void z80n::exec_ed(u8 opc)
{
	const u32 x = opc>>6;
	const u32 y = (opc>>3)&7;
	const u32 z = opc&7;
	const u32 p = y>>1;
	const u32 q = y&1;

	if( x == 0 || x == 3 )
	{
		std::println("error: $ed x0||3");
		exit(1);
		return;
	}
	
	if( x == 2 ) 
	{
		if( z > 3 || y < 4 ) 
		{ 
			std::println("error: $ed x2 y{} z{}", y, z); 
			exit(1);
			return;
		}
		block(y-4, z);
		return; 
	}
	
	//x==1
	if( z == 0 ) { if( y == 6 ) { setszp(in8(BC)); } else { table_r(y, setszp(in8(BC))); } setN(0); setH(0); return; }
	if( z == 1 ) { if( y == 6 ) { out8(BC,0); } else { out8(BC, table_r(y)); } return; }
	if( z == 2 )
	{
		u16 a = HL;
		u16 b = rp(p);
		u16 c = F&FLAG_C;
		if( q == 0 ) { c ^= 1; b ^= 0xffff; }
		u32 t = a;
		t += b;
		t += c;
		F &= ~FLAG_S;
		if( t & 0x8000 ) F |= FLAG_S;
		setV(((t^a)&(t^b)&0x8000)?1:0);
		setC(t>>16);
		setZ((t&0xffff)==0);
		set53(t>>8);
		setN(q==0);
		setH((a&0xfff)+(b&0xfff)+c > 0xfff);
		if( q == 0 ) { F ^= FLAG_C; F ^= FLAG_H; }
		setHL(t);
		return;
	}
	if( z == 3 )
	{
		if( q == 0 )
		{
			write16(imm16(), rp(p));
		} else {
			rp(p, read16(imm16()));
		}
		return;
	}
	if( z == 4 )
	{ // neg
		u8 a = 0;
		u8 b = A^0xff;
		u16 t = a;
		t += b;
		t += 1;
		setszp(t);
		setC((t>>8)^1);
		set53(t);
		setN(1);
		setH(((a&15)+(b&15)+1)>15);
		F ^= FLAG_H;
		setV((t^a)&(t^b)&0x80);
		A = t;
		return;
	}
	if( z == 5 )
	{
		if( y == 1 )
		{
			// retn
			iff1 = iff2;
			pc = pop16();
		} else {
			// reti  todo: actually have a mechanism for intack
			pc = pop16();
		}
		return;
	}
	if( z == 6 )
	{
		//todo: im im[y]
		return;
	}
	// z == 7
	switch( y )
	{
	case 0: I = A; return;
	case 1: R = A; return;
	case 2: A = I; return;
	case 3: A = R; return;
	case 4:{ // RRD
		u8 t = A&0xf;
		u8 mem = read8(HL);
		A &= 0xf0;
		A |= mem&0xf;
		mem >>= 4;
		mem |= t<<4;
		write8(HL, mem);
		setszp(A);
		set53(A);
		setH(0);
		setN(0);	
		}return;
	case 5:{ // RLD
		u8 t = A&0xf;
		u8 mem = read8(HL);
		A &= 0xf0;
		A |= mem>>4;
		mem <<= 4;
		mem |= t;
		write8(HL, mem);
		setszp(A);
		set53(A);
		setH(0);
		setN(0);	
		}return;	
	case 6: return; // nop
	case 7: return; // nop
	}
	// end of $ED prefixed instructions
}

void z80n::exec_cb(u8 opc)
{
	const u32 x = opc>>6;
	const u32 y = (opc>>3)&7;
	const u32 z = opc&7;
	
	if( x == 1 )
	{ // bit
		u8 t = 0;
		if( prefix )
		{
			t = (prefix==USE_X) ? read8(ix+disp8) : read8(iy+disp8);
		} else {
			t = table_r(z);
		}
		t &= BIT(y);
		setZ(t==0);
		F &= ~FLAG_S;
		if( t&0x80 ) F |= FLAG_S;
		set53(t);
		setH(1);
		setN(0);
		setV(F&FLAG_Z);
		return;
	}
	
	if( x == 2 )
	{ // res
		u8 t = 0;
		if( prefix )
		{
			t = ((prefix==USE_X) ? read8(ix+disp8) : read8(iy+disp8));
		} else {
			t = table_r(z);
		}
		t &= ~BIT(y);
		if( prefix )
		{
			if( prefix == USE_X ) { write8(ix+disp8, t); } else { write8(iy+disp8, t); }
			if( z != 6 ) table_r(z, t, false);
		} else {
			table_r(z, t);
		}
		return;	
	}
	
	if( x == 3 )
	{ // set
		u8 t = 0;
		if( prefix )
		{
			t = ((prefix==USE_X) ? read8(ix+disp8) : read8(iy+disp8));
		} else {
			t = table_r(z);
		}
		t |= BIT(y);
		if( prefix )
		{
			if( prefix == USE_X ) { write8(ix+disp8, t); } else { write8(iy+disp8, t); }
			if( z != 6 ) table_r(z, t, false);
		} else {
			table_r(z, t);
		}
		return;	
	}	
	
	u8 t = 0;
	if( prefix )
	{
		t = ((prefix==USE_X) ? read8(ix+disp8) : read8(iy+disp8));
	} else {
		t = table_r(z);
	}	
	
	switch( y )
	{
	case 0:{ // rlc
		t = (t<<1)|(t>>7);
		setC(t&1);
		}break;
	case 1:{ // rrc
		t = (t>>1)|(t<<7);
		setC(t>>7);
		}break;
	case 2:{ // rl
		u8 oldC = F&FLAG_C;
		setC(t>>7);
		t = (t<<1)|oldC;
		}break;
	case 3:{ // rr
		u8 oldC = (F&FLAG_C)<<7;
		setC(t&1);
		t = oldC|(t>>1);
		}break;
	case 4:{ // sla
		setC(t>>7);
		t <<= 1;
		}break;
	case 5:{ // sra
		setC(t&1);
		t = (t&0x80)|(t>>1);
		}break;
	case 6:{ // sll
		setC(t>>7);
		t <<= 1; t |= 1;
		}break;
	case 7:{ // srl
		setC(t&1);
		t >>= 1;
		}break;
	}
	setszp(t);
	set53(t);	
	F &= ~(FLAG_H|FLAG_N);

	if( prefix )
	{
		if( prefix == USE_X ) { write8(ix+disp8, t); } else { write8(iy+disp8, t); }
		if( z != 6 ) table_r(z, t, false);
	} else {
		table_r(z, t);
	}	
}

void z80n::setHL(u16 v)
{
	if( prefix == USE_X ) { ix = v; }
	else if( prefix == USE_Y ) { iy = v; }
	else { H = v>>8; L = v; }
}

void z80n::block(u8 a, u8 b)
{ // todo: strange flags for in/out cases
	a &= 3;
	b &= 3;
	if( b == 0 )
	{
		switch( a )
		{
		case 0:{ // ldi
			u8 n = read8(HL);
			write8(DE, n);
			setBC(BC-1);
			setDE(DE+1);
			setHL(HL+1);
			setN(0);
			setH(0);
			setV(BC!=0);
			n += A;
			set53(((n<<4)&BIT(5))|(n&BIT(3))); 
			}break;
		case 1:{ // ldd
			u8 n = read8(HL);
			write8(DE, n);
			setBC(BC-1);
			setDE(DE-1);
			setHL(HL-1);		
			setH(0);
			setN(0);
			setV(BC!=0);
			n += A;
			set53(((n<<4)&BIT(5))|(n&BIT(3))); 
			}break;
		case 2:{ // ldir
			u8 n = read8(HL);
			write8(DE, n);
			setBC(BC-1);
			setDE(DE+1);
			setHL(HL+1);
			setH(0);
			setN(0);
			setV(BC!=0);
			n += A;
			set53(((n<<4)&BIT(5))|(n&BIT(3))); 
			if( BC != 0 )
			{
				pc -= 2;
			}
			}break;
		case 3:{ // lddr
			u8 n = read8(HL);
			write8(DE, n);
			setBC(BC-1);
			setDE(DE-1);
			setHL(HL-1);
			setH(0);
			setN(0);
			setV(BC!=0);
			n += A;
			set53(((n<<4)&BIT(5))|(n&BIT(3))); 
			if( BC != 0 )
			{
				pc -= 2;
			}
			}break;		
		}
		return;
	}

	if( b == 1 )
	{
		u8 t = read8(HL);
		t ^= 0xff;
		switch( a )
		{
		case 0:{ // cpi
			u16 res = A;
			res += t;
			res += 1;
			F &= ~(FLAG_S|FLAG_H);
			if( res&0x80 ) F |= FLAG_S;
			if( (A&15)+(t&15)+1 > 15 ) F |= FLAG_H;
			F ^= FLAG_H;
			setZ((res&0xff)==0);
			setHL(HL+1);
			setBC(BC-1);
			setV(BC!=0);
			setN(1);
			u8 n = A - (t^0xff) - ((F&FLAG_H)?1:0);
			set53(((n<<4)&BIT(5))|(n&BIT(3)));
			}break;
		case 1:{ // cpd
			u16 res = A;
			res += t;
			res += 1;
			F &= ~(FLAG_S|FLAG_H);
			if( res&0x80 ) F |= FLAG_S;
			if( (A&15)+(t&15)+1 > 15 ) F |= FLAG_H;
			F ^= FLAG_H;
			setZ((res&0xff)==0);
			setHL(HL-1);
			setBC(BC-1);
			setV(BC!=0);
			setN(1);
			u8 n = A - (t^0xff) - ((F&FLAG_H)?1:0);
			set53(((n<<4)&BIT(5))|(n&BIT(3)));
			}break;
		case 2:{ // cpir
			u16 res = A;
			res += t;
			res += 1;
			F &= ~(FLAG_S|FLAG_H);
			if( res&0x80 ) F |= FLAG_S;
			if( (A&15)+(t&15)+1 > 15 ) F |= FLAG_H;
			F ^= FLAG_H;
			setZ((res&0xff)==0);
			setHL(HL+1);
			setBC(BC-1);
			setV(BC!=0);
			setN(1);
			u8 n = A - (t^0xff) - ((F&FLAG_H)?1:0);
			set53(((n<<4)&BIT(5))|(n&BIT(3)));
			if( BC != 0 && !(F&FLAG_Z) )
			{
				pc -= 2;
			}
			}break;
		case 3:{ // cpdr
			u16 res = A;
			res += t;
			res += 1;
			F &= ~(FLAG_S|FLAG_H);
			if( res&0x80 ) F |= FLAG_S;
			if( (A&15)+(t&15)+1 > 15 ) F |= FLAG_H;
			F ^= FLAG_H;
			setZ((res&0xff)==0);
			setHL(HL-1);
			setBC(BC-1);
			setV(BC!=0);
			setN(1);
			u8 n = A - (t^0xff) - ((F&FLAG_H)?1:0);
			set53(((n<<4)&BIT(5))|(n&BIT(3)));
			if( BC != 0 && !(F&FLAG_Z) )
			{
				pc -= 2;
			}
			}break;		
		}
		return;
	}
	
	if( b == 2 )
	{
		switch( a )
		{
		case 0: // ini
			write8(HL, in8(BC));
			B = dec8(B);
			setHL(HL+1);
			break;
		case 1: // ind
			write8(HL, in8(BC));
			B = dec8(B);
			setHL(HL-1);
			break;
		case 2: // inir
			write8(HL, in8(BC));
			B = dec8(B);
			setHL(HL+1);
			if( B != 0 )
			{
				pc -= 2;
			}
			break;
		case 3: // indr
			write8(HL, in8(BC));
			B = dec8(B);
			setHL(HL-1);
			if( B != 0 )
			{
				pc -= 2;
			}
			break;
		}
		return;
	}

	if( b == 3 )
	{
		switch( a )
		{
		case 0: // outi
			out8(BC, read8(HL));
			B = dec8(B);
			setHL(HL+1);
			break;
		case 1: // outd
			out8(BC, read8(HL));
			B = dec8(B);
			setHL(HL-1);
			break;
		case 2: // otir
			out8(BC, read8(HL));
			B = dec8(B);
			setHL(HL+1);
			if( B != 0 )
			{
				pc -= 2;
			}
			break;
		case 3: // otdr
			out8(BC, read8(HL));
			B = dec8(B);
			setHL(HL-1);
			if( B != 0 )
			{
				pc -= 2;
			}
			break;
		}
		return;
	}
}

void z80n::daa()
{
	u8 diff = 0;
	u8 CF = F&FLAG_C;
	u8 HF = F&FLAG_H;
	u8 NF = F&FLAG_N;
	u8 hn = A>>4;
	u8 ln = A&15;
	
	if( CF == 0 )
	{
		if( hn<10 && !HF && ln<10 ) diff = 0;
		else if( hn<10 && HF && ln<10 ) diff = 6;
		else if( hn<9 && ln>9 ) diff = 6;
		else if( hn>9 && !HF && ln<10 ) diff = 0x60;
		else if( hn>8 && ln>9 ) diff = 0x66;
		else if( hn>9 && HF && ln<10 ) diff = 0x66;	
	} else {
		if( !HF && ln<10 ) diff = 0x60;
		else if( HF && ln < 10 ) diff = 0x66;
		else if( ln > 9 ) diff = 0x66;
	}
	
	if( !CF )
	{
		if( hn>8 && ln>9 ) F |= FLAG_C;
		else if( hn>9 && ln<10 ) F|=FLAG_C;	
	}

	if( !NF )
	{
		setH(ln>9);
	} else if( HF ) {
		setH(ln<6);
	}
	
	if( NF )
	{
		A -= diff;
	} else {
		A += diff;
	}
	setszp(A);
	set53(A);
}



















