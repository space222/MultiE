#include <cstdio>
#include <cstdlib>
#include <bit>
#include "8086.h"

#define AX r[0].v
#define CX r[1].v
#define DX r[2].v
#define BX r[3].v
#define SP r[4].v
#define BP r[5].v
#define SI r[6].v
#define DI r[7].v

#define AL r[0].b.l
#define AH r[0].b.h
#define CL r[1].b.l

#define ES seg[0]
#define CS seg[1]
#define SS seg[2]
#define DS seg[3]

#define PREFIX_SS 1
#define PREFIX_DS 2
#define PREFIX_ES 4
#define PREFIX_CS 8
#define PREFIX_REP 0x10
#define PREFIX_REPN 0x20
#define ANY_REP (prefix & (PREFIX_REP|PREFIX_REPN))

class modrm
{
public:
	modrm(x8086& x, bool call_lea = true) : cpu(x)
	{
		m = cpu.read((cpu.CS<<4) + cpu.pc); cpu.pc++;
		if( call_lea ) lea();
	}
	
	u16 reg16() { return cpu.r[(m>>3)&7].v; }
	void reg16(u16 v) { cpu.r[(m>>3)&7].v = v; }
	u8 reg8() { return (m&0x20)? cpu.r[(m>>3)&3].b.h : cpu.r[(m>>3)&3].b.l; }	
	void reg8(u8 v) { if( m & 0x20 ) { cpu.r[(m>>3)&3].b.h = v; } else { cpu.r[(m>>3)&3].b.l = v; } }

	u16 rm16()
	{
		if( (m & 0xc0) == 0xc0 ) return cpu.r[m&7].v;
		
		u16 res = cpu.read((lea_seg<<4) + lea_off);
		res |= cpu.read((lea_seg<<4) + ((lea_off+1)&0xffff))<<8;
		return res;
	}
	
	void rm16(u16 v)
	{
		if( (m & 0xc0) == 0xc0 )
		{
			cpu.r[m&7].v = v;
			return;
		}
		
		cpu.write((lea_seg<<4) + lea_off, v);
		cpu.write((lea_seg<<4) + ((lea_off+1)&0xffff), v>>8);
	}
	
	u8 rm8()
	{
		if( (m & 0xc0) == 0xc0 )
		{
			return (m&4)? cpu.r[m&3].b.h : cpu.r[m&3].b.l;
		}
		return cpu.read((lea_seg<<4) + lea_off);
	}
	
	void rm8(u8 v)
	{
		if( (m & 0xc0) == 0xc0 )
		{
			if( m & 4 ) { cpu.r[m&3].b.h = v; } else { cpu.r[m&3].b.l = v; }
			return;
		}
		cpu.write((lea_seg<<4) + lea_off, v);
	}
	
	void lea()
	{
		if( (m&0xc0) == 0xc0 ) return;
		lea_seg = cpu.DS;
		is_ss = false;
		const u8 r = m&7;
		if( r == 2 || r == 3 || (r == 6 && (m&0xc0)!=0) )
		{
			lea_seg = cpu.SS;
			is_ss = true;
		}
		u16 disp = 0;
		if( (m&0xc0) == 0x40 ) 
		{
			disp = (s16)(s8) cpu.read((cpu.CS<<4) + cpu.pc++);
		} else if( (m&0xc0) == 0x80 || (r == 6 && (m&0xc0)==0) ) {
			disp = cpu.read((cpu.CS<<4) + cpu.pc++);
			disp |= cpu.read((cpu.CS<<4) + cpu.pc++)<<8;
		}
		
		switch( r )
		{
		case 0: lea_off = cpu.BX + cpu.SI + disp; break;
		case 1:	lea_off = cpu.BX + cpu.DI + disp; break;
		case 2:	lea_off = cpu.BP + cpu.SI + disp; break;
		case 3:	lea_off = cpu.BP + cpu.DI + disp; break;
		case 4:	lea_off = cpu.SI + disp; break;
		case 5:	lea_off = cpu.DI + disp; break;
		case 6:	if( (m&0xc0) == 0 ) lea_off = disp;
			else lea_off = cpu.BP + disp;
			break;
		case 7:	lea_off = cpu.BX + disp; break;		
		}
		
		if( cpu.prefix & PREFIX_DS ) { lea_seg = cpu.DS; is_ss = false; }
		else if( cpu.prefix & PREFIX_ES ) { lea_seg = cpu.ES; is_ss = false; }
		else if( cpu.prefix & PREFIX_SS ) { lea_seg = cpu.SS; is_ss = true; }
		else if( cpu.prefix & PREFIX_CS ) { lea_seg = cpu.CS; is_ss = false; }
	}

	x8086& cpu;
	u8 m;
	u16 lea_off, lea_seg;
	bool is_ss;
};

void x8086::setSZP8(u8 v)
{
	F.b.Z = (v==0)?1:0;
	F.b.S = (v&0x80)?1:0;
	F.b.P = (std::popcount(v)&1)?0:1;
}

void x8086::setSZP16(u16 v)
{
	F.b.Z = (v==0)?1:0;
	F.b.S = (v&0x8000)?1:0;
	F.b.P = (std::popcount(v)&1)?0:1;
}

u8 x8086::add8(u8 a, u8 b, u8 c)
{
	u16 res = a;
	res += b;
	res += c;
	F.b.C = res>>8;
	F.b.O = ((res^a) & (res^b) & 0x80)?1:0;
	F.b.A = (((a&0xf) + (b&0xf) + c)>0xf)?1:0;
	setSZP8(res);
	return res;
}

u16 x8086::add16(u16 a, u16 b, u16 c)
{
	u32 res = a;
	res += b;
	res += c;
	F.b.C = res>>16;
	F.b.O = ((res^a) & (res^b) & 0x8000)?1:0;
	F.b.A = (((a&0xff) + (b&0xff) + c)>0xff)?1:0;
	setSZP16(res);
	return res;
}

u64 x8086::exec(u8 opc)
{
	u8 temp8 = 0;
	u16 temp16 = 0;
	switch( opc )
	{
	case 0x00:{
		modrm m(*this);
		temp8 = add8(m.rm8(), m.reg8(), 0);
		m.rm8(temp8);
		}break;
	case 0x01:{
		modrm m(*this);
		temp16 = add16(m.rm16(), m.reg16(), 0);
		m.rm16(temp16);
		}break;
	case 0x02:{
		modrm m(*this);
		temp8 = add8(m.reg8(), m.rm8(), 0);
		m.reg8(temp8);
		}break;
	case 0x03:{
		modrm m(*this);
		temp16 = add16(m.reg16(), m.rm16(), 0);
		m.reg16(temp16);
		}break;
	case 0x04:
		AL = add8(AL, read((CS<<4)+pc), 0); pc++;
		break;
	
	case 0x06: push16(ES); break;
	case 0x07: ES = pop16(); break;		
	case 0x08:{
		modrm m(*this);
		temp8 = m.rm8();
		temp8 |= m.reg8();
		setSZP8(temp8);
		m.rm8(temp8);
		F.b.C = F.b.O = 0;	
		}break;
	case 0x09:{
		modrm m(*this);
		temp8 = m.rm8();
		temp8 |= m.reg8();
		setSZP8(temp8);
		m.reg8(temp8);
		F.b.C = F.b.O = 0;	
		}break;
	case 0x0A:{
		modrm m(*this);
		temp8 = m.reg8();
		temp8 |= m.rm8();
		setSZP8(temp8);
		m.reg8(temp8);
		F.b.C = F.b.O = 0;	
		}break;
		
	case 0x0C:{
		AL |= read((CS<<4)+pc); pc++;
		setSZP8(AL);
		}break;
	case 0x0D:{
		AX |= read16(CS, pc); pc+=2;
		setSZP16(AX);
		}break;		
	case 0x0E: push16(CS); break;
	
	case 0x10:{
		modrm m(*this);
		m.rm8(add8(m.rm8(), m.reg8(), F.b.C));
		}break;
	case 0x11:{
		modrm m(*this);
		m.rm16(add16(m.rm16(), m.reg16(), F.b.C));
		}break;
	
	case 0x1E: push16(DS); break;
	case 0x1F: DS = pop16(); break;
	
	case 0x24:
		AL &= read((CS<<4)+pc); pc++;
		setSZP8(AL);
		break;
	
	case 0x29:{
		modrm m(*this);
		m.rm16(add16(m.rm16(), m.reg16()^0xffff, 1));
		F.b.C ^= 1;
		F.b.A ^= 1;	
		}break;
		
	case 0x2D:{
		temp16 = read16(CS, pc); pc+=2;
		AX = add16(AX, temp16^0xffff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;	
		}break;
		
	case 0x30:{
		modrm m(*this);
		temp8 = m.rm8();
		temp8 ^= m.reg8();
		setSZP8(temp8);
		F.b.C = F.b.O = 0;
		m.rm8(temp8);	
		}break;		
	case 0x31:{
		modrm m(*this);
		temp16 = m.rm16();
		temp16 ^= m.reg16();
		setSZP16(temp16);
		F.b.C = F.b.O = 0;
		m.rm16(temp16);	
		}break;
	
	case 0x38:{
		modrm m(*this);
		add8(m.rm8(), m.reg8()^0xff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;	
		}break;	
	case 0x39:{
		modrm m(*this);
		add16(m.rm16(), m.reg16()^0xffff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;	
		}break;
	case 0x3A:{
		modrm m(*this);
		add8(m.reg8(), m.rm8()^0xff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		}break;
	case 0x3B:{
		modrm m(*this);
		add16(m.reg16(), m.rm16()^0xffff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		}break;
	case 0x3C:{
		temp8 = read((CS<<4)+pc); pc++;
		add8(AL, temp8^0xff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		}break;
	case 0x3D:{
		temp16 = read16(CS, pc); pc += 2;
		add16(AX, temp16^0xffff, 1);
		F.b.C ^= 1;
		F.b.A ^= 1;
		}break;
		
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47: // inc r16  (todo: A flag)
		temp16 = r[opc&7].v;
		setSZP16(temp16+1);
		F.b.O = (temp16==0x7fff)?1:0;
		r[opc&7].v += 1;	
		break;
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4E:
	case 0x4F: // dec r16  (todo: A flag)
		temp16 = r[opc&7].v;
		setSZP16(temp16-1);
		F.b.O = (temp16==0x8000)?1:0;
		r[opc&7].v -= 1;	
		break;		
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
		push16(r[opc&7].v);
		break;
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5E:
	case 0x5F:
		r[opc&7].v = pop16();
		break;
	
	case 0x70: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.O )
		{
			pc += (s8)temp8;
		}
		break;	
	case 0x71: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.O == 0 )
		{
			pc += (s8)temp8;
		}
		break;
	case 0x72: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.C )
		{
			pc += (s8)temp8;
		}
		break;
	case 0x73: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.C == 0 )
		{
			pc += (s8)temp8;
		}
		break;
	case 0x74: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.Z == 1 ) 
		{
			pc += (s8)temp8;
		}
		break;
	case 0x75: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.Z == 0 ) 
		{
			pc += (s8)temp8;
		}
		break;
	case 0x76:
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.Z || F.b.C ) 
		{
			pc += (s8)temp8;
		}
		break;
	case 0x77:
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.Z == 0 && F.b.C == 0 ) 
		{
			pc += (s8)temp8;
		}
		break;
	case 0x78: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.S ) 
		{
			pc += (s8)temp8;
		}
		break;
	case 0x7A: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.P ) 
		{
			pc += (s8)temp8;
		}
		break;
	case 0x7B: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.P == 0 ) 
		{
			pc += (s8)temp8;
		}
		break;
	case 0x7C: 
		temp8 = read((CS<<4)+pc); pc++;
		if( F.b.S != F.b.O ) 
		{
			pc += (s8)temp8;
		}
		break;
	case 0x80:{
		modrm m(*this);
		temp8 = read((CS<<4)+pc); pc++;
		switch( (m.m>>3)&7 )
		{
		case 0: m.rm8(add8(m.rm8(), temp8, 0)); break;
		case 1: temp8 |= m.rm8(); m.rm8(temp8); setSZP8(temp8); break;
		case 2: m.rm8(add8(m.rm8(), temp8, F.b.C)); break;
		case 4: temp8 &= m.rm8(); m.rm8(temp8); setSZP8(temp8); break;
		case 7: add8(m.rm8(), temp8^0xff, 1); F.b.C ^= 1; F.b.A ^= 1; break;
		default: printf("unimpl 0x80 subcode %i\n", (m.m>>3)&7); exit(1);		
		}	
		}break;
		
	case 0x81:{
		modrm m(*this);
		temp16 = read16(CS, pc); pc+=2;
		switch( (m.m>>3)&7 )
		{
		case 0: m.rm16(add16(m.rm16(), temp16, 0)); break;
		case 2: m.rm16(add16(m.rm16(), temp16, F.b.C)); break;
		case 7: add16(m.rm16(), temp16^0xffff, 1); F.b.C ^= 1; F.b.A ^= 1; break;
		default:
			printf("Unimpl 0x81 subcode %i\n", (m.m>>3)&7);
			exit(1);
		}	
		}break;
		
		
	case 0x83:{
		modrm m(*this);
		temp16 = (s16)(s8) read((CS<<4)+pc); pc++;
		switch( (m.m>>3)&7 )
		{
		case 0: m.rm16(add16(m.rm16(), temp16, 0)); break;
		case 3: m.rm16(add16(m.rm16(), temp16^0xffff, F.b.C^1)); F.b.C ^= 1; F.b.A ^= 1; break;
		case 4: temp16 &= m.rm16(); m.rm16(temp16); setSZP16(temp16); break;
		case 7: add16(m.rm16(), temp16^0xffff, 1); F.b.C ^= 1; F.b.A ^= 1; break;
		default: printf("unimpl 0x83 subcode %i\n", (m.m>>3)&7); exit(1);		
		}	
		}break;
		
	case 0x86:{
		modrm m(*this);
		temp8 = m.rm8();
		m.rm8(m.reg8());
		m.reg8(temp8);		
		}break;
	case 0x87:{
		modrm m(*this);
		temp16 = m.rm16();
		m.rm16(m.reg16());
		m.reg16(temp16);		
		}break;		
	case 0x88:{
		modrm m(*this);
		m.rm8(m.reg8());	
		}break;
	case 0x89:{
		modrm m(*this);
		m.rm16(m.reg16());	
		}break;
	case 0x8A:{
		modrm m(*this);
		m.reg8(m.rm8());
		}break;
	case 0x8B:{
		modrm m(*this);
		m.reg16(m.rm16());
		}break;
		
	case 0x8C:{
		modrm m(*this);
		m.rm16(seg[(m.m>>3)&3]);		
		}break;
	case 0x8D:{
		modrm m(*this);
		m.reg16(m.lea_off);
		}break;
	case 0x8E:{
		modrm m(*this);
		seg[(m.m>>3)&3] = m.rm16();
		}break;
		
	case 0x90: break;
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x97:
		temp16 = AX;
		AX = r[opc&7].v;
		r[opc&7].v = temp16;
		break;
		
	case 0x9C: push16(F.v); break;
	case 0x9D: F.v = pop16(); break;
	case 0xA0:{
		temp16 = read16(CS, pc); pc+=2;
		AL = read((seg_override(DS)<<4) + temp16);
		}break;
	case 0xA1:{
		temp16 = read16(CS, pc); pc+=2;
		AX = read16(seg_override(DS), temp16);
		}break;		
	case 0xA2:{
		temp16 = read16(CS, pc); pc+=2;
		write((seg_override(DS)<<4)+temp16, AL);		
		}break;
	case 0xA3:{
		temp16 = read16(CS, pc); pc+=2;
		write16(seg_override(DS), temp16, AX);		
		}break;
		
	case 0xA5:
		temp16 = read16(seg_override(DS), SI);
		write((ES<<4)+DI, temp16);
		write((ES<<4)+((DI+1)&0xffff), temp16>>8);
		if( F.b.D )
		{
			DI -= 2;
			SI -= 2;
		} else {
			DI += 2;
			SI += 2;
		}
		break;
		
	case 0xA8:
		temp8 = read((CS<<4)+pc); pc++;
		setSZP8(temp8 & AL);
		break;
	case 0xA9:
		temp16 = read16(CS, pc); pc+=2;
		setSZP16(temp16 & AX);
		break;
	case 0xAA:
		if( ANY_REP && CX == 0 ) break;
		write((ES<<4) + DI, AL);
	 	DI += (F.b.D ? -1 : 1);
		if( ANY_REP )
		{
			CX--;
			CS = start_cs;
			pc = start_ip;
		}
		break;	
	case 0xAB:
		if( ANY_REP && CX == 0 ) break;
		write((ES<<4) + DI, AL);
	 	write((ES<<4) + ((DI+1)&0xffff), AH);
		DI += (F.b.D ? -2 : 2);
		if( ANY_REP )
		{
			CX--;
			CS = start_cs;
			pc = start_ip;
		}
		break;
	case 0xAC:
		AL = read((seg_override(DS)<<4) + SI);
		SI += (F.b.D ? -1 : 1);
		break;	
	case 0xAD:
		AX = read16(seg_override(DS), SI);
		SI += (F.b.D ? -2 : 2);
		break;

	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7:
		if( opc & 4 )
		{
			r[opc&3].b.h = read((CS<<4)+pc); pc++;
		} else {
			r[opc&3].b.l = read((CS<<4)+pc); pc++;
		}
		break;	
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBE:
	case 0xBF:
		r[opc&7].v = read16(CS, pc); pc += 2;
		break;
		
	case 0xC3: pc = pop16(); break;
	case 0xC4:{ //TODO: LES, check it
		modrm m(*this);
		u16 o = read16(m.lea_seg, m.lea_off);
		u16 s = read16(m.lea_seg, m.lea_off+2);
		m.reg16(o);
		ES = s;
		}break;
	case 0xC5:{ //TODO: LDS, check it
		modrm m(*this);
		u16 o = read16(m.lea_seg, m.lea_off);
		u16 s = read16(m.lea_seg, m.lea_off+2);
		m.reg16(o);
		DS = s;
		}break;
	case 0xC6:{
		modrm m(*this);
		m.rm8(read((CS<<4)+pc)); pc++;
		}break;
	case 0xC7:{
		modrm m(*this);
		m.rm16(read16(CS, pc)); pc += 2;	
		}break;
		
	case 0xCA: 
		temp16 = read16(CS, pc);
		pc = pop16();
		CS = pop16();
		SP += temp16;
		break;
		
	case 0xCD:{
		printf("$%X:$%X: ", CS, pc-1);		
		u32 vec = read((CS<<4)+pc)*4; pc++;
		push16(F.v);
		push16(CS);
		push16(pc);
		u16 o = read16(0, vec);
		u16 s = read16(0, vec+2);
		CS = s;
		pc = o;
		printf("software interrupt $%02X, to $%X:$%X\n", vec>>2, CS, pc);
		}break;
		
	case 0xCF:
		pc = pop16();
		CS = pop16();
		F.v = pop16();
		break;
	case 0xD0:{
		modrm m(*this);
		
		switch( (m.m>>3)&7 )
		{
		case 0:{
			temp8 = m.rm8();
			F.b.C = temp8>>7;
			temp8 = (temp8>>7)|(temp8<<1);
			setSZP8(temp8);
			m.rm8(temp8);
			}break;
		case 1:{
			temp8 = m.rm8();
			u8 oldC = F.b.C;
			F.b.C = temp8&1;
			temp8 = (oldC<<7)|(temp8>>1);
			setSZP8(temp8);
			m.rm8(temp8);
			}break;
		case 3:{
			temp8 = m.rm8();
			F.b.C = temp8&1;
			temp8 = (temp8<<7)|(temp8>>1);
			setSZP8(temp8);
			m.rm8(temp8);
			}break;			
		case 4:
			temp8 = m.rm8();
			F.b.C = temp8>>7;
			temp8 <<= 1;
			setSZP8(temp8);
			m.rm8(temp8);
			break;
		case 5:
			temp8 = m.rm8();
			F.b.C = temp8&1;
			temp8 >>= 1;
			setSZP8(temp8);
			m.rm8(temp8);
			break;
		default: 
			printf("Unimpl D0 shift %i\n", (m.m>>3)&7);
			exit(1);
		}	
		}break;
	case 0xD1:{
		modrm m(*this);
		
		switch( (m.m>>3)&7 )
		{
		case 0:
			temp16 = m.rm16();
			F.b.C = temp16>>15;
			temp16 = (temp16<<1)|(temp16>>15);
			setSZP16(temp16);
			m.rm16(temp16);
			break;
		case 4:
			temp16 = m.rm16();
			F.b.C = temp16>>15;
			temp16 <<= 1;
			setSZP16(temp16);
			m.rm16(temp16);
			break;
		case 5:
			temp16 = m.rm16();
			F.b.C = temp16 & 1;
			temp16 >>= 1;
			setSZP16(temp16);
			m.rm16(temp16);
			break;
		default: 
			printf("Unimpl D1 shift %i\n", (m.m>>3)&7);
			exit(1);
		}	
		}break;	
	case 0xD2:{
		modrm m(*this);
		
		switch( (m.m>>3)&7 )
		{
		case 4:
			temp8 = m.rm8();
			for(u32 i = 0; i < (CL&0x1F); ++i)
			{
				F.b.C = temp8>>7;
				temp8 <<= 1;
			}
			setSZP8(temp8);
			m.rm8(temp8);
			break;
		case 5:
			temp8 = m.rm8();
			for(u32 i = 0; i < (CL&0x1F); ++i)
			{
				F.b.C = temp8&1;
				temp8 >>= 1;
			}
			setSZP8(temp8);
			m.rm8(temp8);
			break;
		default: 
			printf("Unimpl D2 shift %i\n", (m.m>>3)&7);
			exit(1);
		}	
		}break;
	case 0xD3:{
		modrm m(*this);
		
		switch( (m.m>>3)&7 )
		{
		case 4:
			temp16 = m.rm16();
			for(u32 i = 0; i < (CL&0x1F); ++i)
			{
				F.b.C = temp16>>15;
				temp16 <<= 1;
			}
			setSZP16(temp16);
			m.rm16(temp16);
			break;
		default: 
			printf("Unimpl D3 shift %i\n", (m.m>>3)&7);
			exit(1);
		}	
		}break;
		
	case 0xD5: //todo: AAD
		temp8 = read((CS<<4)+pc); pc++;
		printf("AAD $%X\n", temp8);
		break;
		
	case 0xD8: // fpu
	case 0xD9:
	case 0xDA:
	case 0xDB:
	case 0xDC:
	case 0xDD:
	case 0xDE:
	case 0xDF:
		pc++;
		break;
		
	case 0xE2: temp8 = read((CS<<4)+pc); pc++; CX--; if( CX ) pc += (s8)temp8; break;
	
	case 0xE4: AL = port_in(read((CS<<4)+pc), 8); pc++; break;
	
	case 0xE6: port_out(read((CS<<4)+pc), AL, 8); pc++; break;
		
	case 0xE8:
		temp16 = read16(CS, pc); 
		pc += 2;
		push16(pc);
		pc += temp16;
		//printf("$%X, call\n", pc);
		break;
	case 0xE9:
		temp16 = read16(CS, pc); 
		pc += 2 + temp16;
		break;	
	case 0xEA:{
		u16 o = read16(CS, pc); pc += 2;
		u16 s = read16(CS, pc);
		CS = s;
		pc = o;
		}break;
	case 0xEB:{
		temp8 = read((CS<<4)+pc); pc++;
		pc += (s8) temp8;
		}break;
	case 0xEC: AL = port_in(DX, 8); break;
	case 0xED: AX = port_in(DX, 16); break;
	case 0xEE: port_out(DX, AL, 8); break;
	case 0xEF: port_out(DX, AX, 16); break;
	
	case 0xF4:
		if( F.b.I )
		{
			printf("$%X:$%X: System halted with interrupts enabled\n", CS, pc-1);
		} else {
			printf("$%X:$%X: System halted with disabled interrupts\n", CS, pc-1);
		}
		exit(1);
		break;
	case 0xF5: F.b.C ^= 1; break;
	case 0xF6:{
		modrm m(*this);
		switch( (m.m>>3)&7 )
		{
		case 0: 
			temp8 = read((CS<<4)+pc); pc++;
			setSZP8(temp8 & m.rm8());
			break;
		case 4:
			AX = AL;
			AX *= m.rm8();
			break;
		/*case 2: m.rm16(m.rm16()^0xffff); break;
		case 4:{
			u32 t = AX;
			u32 RM = m.rm16();
			t *= RM;
			DX = t>>16;
			AX = t;
			F.b.C = F.b.O = ((DX==0)?0:1);
			}break;
		case 6:{
			u32 t = (DX<<16)|AX;
			AX = t / m.rm16();
			DX = t % m.rm16();
			//todo: flags & exception
			}break;
		*/
		default: printf("unimpl 0xF6 subcode %i\n", (m.m>>3)&7); exit(1);		
		}	
		}break;		
	case 0xF7:{
		modrm m(*this);
		switch( (m.m>>3)&7 )
		{
		case 2: m.rm16(m.rm16()^0xffff); break;
		case 4:{
			u32 t = AX;
			u32 RM = m.rm16();
			t *= RM;
			DX = t>>16;
			AX = t;
			F.b.C = F.b.O = ((DX==0)?0:1);
			}break;
		case 6:{
			u32 t = (DX<<16)|AX;
			AX = t / m.rm16();
			DX = t % m.rm16();
			//todo: flags & exception
			}break;
		default: printf("unimpl 0xF7 subcode %i\n", (m.m>>3)&7); exit(1);		
		}	
		}break;
	case 0xF8: F.b.C = 0; break;
	case 0xF9: F.b.C = 1; break;	
	case 0xFA: F.b.I = 0; break;
	case 0xFB: F.b.I = 1; irq_blocked = true; printf("Interrupts Enabled\n"); break;
	case 0xFC: F.b.D = 0; break;
	case 0xFD: F.b.D = 1; break;	
	case 0xFE:{
		modrm m(*this);
		switch( (m.m>>3)&7 )
		{
		case 0:
			temp8 = m.rm8();
			F.b.O = (temp8==0x7F)?1:0;
			F.b.A = ((temp8&0xf)==0xf)?1:0;
			temp8++;
			setSZP8(temp8);
			m.rm8(temp8);
			break;
		case 1:
			temp8 = m.rm8();
			F.b.O = (temp8==0x80)?1:0;
			F.b.A = ((temp8&0xf)==0)?1:0;
			temp8--;
			setSZP8(temp8);
			m.rm8(temp8);
			break;
		default: printf("Unimpl 0xFE subcode %i\n", (m.m>>3)&7); exit(1);
		}
		}break;
	case 0xFF:{
		modrm m(*this);
		switch( (m.m>>3)&7 )
		{
		case 2:
			temp16 = m.rm16();
			push16(pc);
			pc = temp16;
			break;
		case 4:
			pc = m.rm16();
			break;
		default: printf("Unimpl 0xFF subcode %i\n", (m.m>>3)&7); exit(1);
		}
		}break;
	default:
		printf("$%04X:$%04X: Unimpl. opc = $%02X\n", CS, pc-1, opc);
		exit(1);
	}

	return 5;
}

u64 x8086::step()
{
	//printf("PC = $%X\n", pc);
	u8 opc = 0;
	prefix = 0;
	pic.imr = 0;
	if( F.b.I && !irq_blocked && (~pic.imr&pic.irr) )
	{
		int b = std::countr_zero(u8(~pic.imr&pic.irr));
		printf("b = %i, isr = top set bit is bit%i\n", b, std::countr_zero(pic.isr));
		if( b < std::countr_zero(pic.isr) )
		{
			pic.isr |= (1<<b);
			pic.irr &=~(1<<b);
			u32 vec = (/*pic.icw[1]*/8 + b)*4;
			printf("irq #%i\n", vec>>2);
			push16(F.v);
			push16(CS);
			push16(pc);
			u16 o = read16(0, vec);
			u16 s = read16(0, vec+2);
			CS = s;
			pc = o;
			F.b.I = 0;
		}
	}
	irq_blocked = false;
	
	start_ip = pc;
	start_cs = CS;
	
	do {
		opc = read((CS<<4) + pc++);
	} while( is_prefix(opc) );
	
	exec(opc);
	return 5;
}

bool x8086::is_prefix(u8 p)
{
	switch( p )
	{
	case 0xF3: prefix &= ~PREFIX_REPN; prefix |= PREFIX_REP; return true;
	case 0xF2: prefix &= ~PREFIX_REP;  prefix |= PREFIX_REPN; return true;
	case 0x2E: prefix &= ~0xf; prefix |= PREFIX_CS; return true;	
	case 0x36: prefix &= ~0xf; prefix |= PREFIX_SS; return true;
	case 0x3E: prefix &= ~0xf; prefix |= PREFIX_DS; return true;
	case 0x26: prefix &= ~0xf; prefix |= PREFIX_ES; return true;
	default: return false;
	}
	return false;
}

u16 x8086::seg_override(u16 S)
{
	if( prefix & PREFIX_CS ) return CS;
	if( prefix & PREFIX_DS ) return DS;
	if( prefix & PREFIX_SS ) return SS;
	if( prefix & PREFIX_ES ) return ES;
	return S;
}

void x8086::push16(u16 v)
{
	SP--;
	write((SS<<4)+SP, v>>8);
	SP--;
	write((SS<<4)+SP, v);
}

u16 x8086::pop16()
{
	u16 res = read((SS<<4)+SP); SP++;
	res |= read((SS<<4)+SP)<<8; SP++;
	return res;
}

void x8086::reset()
{
	CS = 0xffff;
	pc = 0;
	AL = 0xfe;
	pic.isr = 0;
	pic.irr = 0;
	return;
}

