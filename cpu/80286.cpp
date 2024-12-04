#include <cstdio>
#include <cstdlib>
#include "80286.h"
#include "modrm.h"

#define PREFIX_ES 1
#define PREFIX_CS 2
#define PREFIX_SS 3
#define PREFIX_DS 4
#define PREFIX_REP 0x10
#define PREFIX_REPN 0x20
#define ANY_REP (prefix&0x30)

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

u64 c80286::step()
{
	u8 temp8 = 0;
	u16 temp16 = 0;
	
	//pic.imr = 0;
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
			push(F.v);
			push(CS);
			push(IP);
			u16 o = read16(0, vec);
			u16 s = read16(0, vec+2);
			CS = s;
			IP = o;
			F.b.I = 0;
			halted = false;
		}
	}
	irq_blocked = false;
	
	if( halted ) return 1;
	
	start_cs = CS;
	start_ip = IP;
	prefix = 0;
	
	//printf("$%X:$%X ", CS, IP);
	u8 opc = 0;
	do {
		opc = read_next();
	} while( is_prefix(opc) );
	//printf("$%X\n", opc);
	
	switch( opc )
	{
	case 0x00:{
		modrm m(this, read_next());
		temp8 = add8(m.rm8(), m.reg8(), 0);
		m.rm8(temp8);	
		}break;
	case 0x01:{
		modrm m(this, read_next());
		temp16 = add16(m.rm16(), m.reg16(), 0);
		m.rm16(temp16);	
		}break;
	case 0x02:{
		modrm m(this, read_next());
		temp8 = add8(m.rm8(), m.reg8(), 0);
		m.reg8(temp8);	
		}break;
	case 0x03:{
		modrm m(this, read_next());
		temp16 = add16(m.rm16(), m.reg16(), 0);
		m.reg16(temp16);	
		}break;
	case 0x04:
		AL = add8(AL, read_next(), 0);
		break;
	case 0x05:
		AX = add16(AX, read_next16(), 0);
		break;
		
	case 0x06: push(ES); break;
	case 0x07: ES = pop(); break;
	case 0x08:{
		modrm m(this, read_next());
		temp8 = m.rm8();
		temp8 |= m.reg8();
		setSZP8(temp8);
		m.rm8(temp8);
		F.b.O = F.b.C = 0;
		}break;
	case 0x09:{
		modrm m(this, read_next());
		temp16 = m.rm16();
		temp16 |= m.reg16();
		setSZP16(temp16);
		m.rm16(temp16);
		F.b.O = F.b.C = 0;
		}break;
	case 0x0A:{
		modrm m(this, read_next());
		temp8 = m.rm8();
		temp8 |= m.reg8();
		setSZP8(temp8);
		m.reg8(temp8);
		F.b.O = F.b.C = 0;
		}break;
	case 0x0B:{
		modrm m(this, read_next());
		temp16 = m.rm16();
		temp16 |= m.reg16();
		setSZP16(temp16);
		m.reg16(temp16);
		F.b.O = F.b.C = 0;
		}break;
	case 0x0C:
		AL |= read_next();
		setSZP8(AL);
		F.b.O = F.b.C = 0;
		break;
	case 0x0D:
		AX |= read_next16();
		setSZP16(AX);
		F.b.O = F.b.C = 0;
		break;
	case 0x0E: push(CS); break;
	
	case 0x10:{
		modrm m(this, read_next());
		temp8 = add8(m.rm8(), m.reg8(), F.b.C);
		m.rm8(temp8);	
		}break;
	case 0x11:{
		modrm m(this, read_next());
		temp16 = add16(m.rm16(), m.reg16(), F.b.C);
		m.rm16(temp16);	
		}break;	
	case 0x12:{
		modrm m(this, read_next());
		temp8 = add8(m.rm8(), m.reg8(), F.b.C);
		m.reg8(temp8);	
		}break;
	case 0x13:{
		modrm m(this, read_next());
		temp16 = add16(m.rm16(), m.reg16(), F.b.C);
		m.reg16(temp16);	
		}break;	

	case 0x14: AL = add8(AL, read_next(), F.b.C); break;
	case 0x15: AX = add16(AX, read_next16(), F.b.C); break;
	case 0x16: push(SS); break;
	case 0x17: SS = pop(); break;
	case 0x18:{
		modrm m(this, read_next());
		temp8 = sub8(m.rm8(), m.reg8(), F.b.C);
		m.rm8(temp8);	
		}break;
	case 0x19:{
		modrm m(this, read_next());
		temp16 = sub16(m.rm16(), m.reg16(), F.b.C);
		m.rm16(temp16);	
		}break;
	case 0x1A:{
		modrm m(this, read_next());
		temp8 = sub8(m.reg8(), m.rm8(), F.b.C);
		m.reg8(temp8);	
		}break;	
	case 0x1B:{
		modrm m(this, read_next());
		temp16 = sub16(m.reg16(), m.rm16(), F.b.C);
		m.reg16(temp16);	
		}break;
	case 0x1C:
		AL = sub8(AL, read_next(), F.b.C);
		break;
	case 0x1D:
		AX = sub16(AX, read_next16(), F.b.C);
		break;
	case 0x1E: push(DS); break;
	case 0x1F: DS = pop(); break;
	case 0x20:{
		modrm m(this, read_next());
		temp8 = m.rm8();
		temp8 &= m.reg8();
		m.rm8(temp8);
		setSZP8(temp8);
		F.b.C = F.b.O = 0;
		}break;
	case 0x21:{
		modrm m(this, read_next());
		temp16 = m.rm16();
		temp16 &= m.reg16();
		m.rm16(temp16);
		setSZP16(temp16);
		F.b.C = F.b.O = 0;
		}break;
	case 0x22:{
		modrm m(this, read_next());
		temp8 = m.rm8();
		temp8 &= m.reg8();
		m.reg8(temp8);
		setSZP8(temp8);
		F.b.C = F.b.O = 0;
		}break;			
	case 0x23:{
		modrm m(this, read_next());
		temp16 = m.rm16();
		temp16 &= m.reg16();
		m.reg16(temp16);
		setSZP16(temp16);
		F.b.C = F.b.O = 0;
		}break;
	case 0x24:
		AL &= read_next();
		setSZP8(AL);
		F.b.C = F.b.O = 0;
		break;		
	case 0x25:
		AX &= read_next16();
		setSZP16(AX);
		F.b.C = F.b.O = 0;
		break;
		
	case 0x27: // daa
		temp8 = AL;
		if( F.b.A || (AL&0xf)>9 )
		{
			AL += 6;
			F.b.A = 1;
		}
		if( temp8 > 0x99 || F.b.C )
		{
			AL += 0x60;
			F.b.C = 1;
		}
		setSZP8(AL);
		break;
	case 0x28:{
		modrm m(this, read_next());
		temp8 = sub8(m.rm8(), m.reg8(), 0);
		m.rm8(temp8);
		}break;		
	case 0x29:{
		modrm m(this, read_next());
		temp16 = sub16(m.rm16(), m.reg16(), 0);
		m.rm16(temp16);
		}break;
	case 0x2A:{
		modrm m(this, read_next());
		temp8 = sub8(m.reg8(), m.rm8(), 0);
		m.reg8(temp8);
		}break;
	case 0x2B:{
		modrm m(this, read_next());
		temp16 = sub16(m.reg16(), m.rm16(), 0);
		m.reg16(temp16);
		}break;
	case 0x2C:
		temp8 = read_next();
		AL = sub8(AL, temp8, 0);
		break;
	case 0x2D:
		temp16 = read_next16();
		AX = sub16(AX, temp16, 0);
		break;
	
	case 0x2F:{ //todo: das
		u8 oldAL = AL;
		u8 oldCF = F.b.C;
		F.b.C = 0;
		if( (AL & 0xf) > 9 || F.b.A )
		{
			AL -= 6;
			F.b.C = oldCF | ((AL&0xf0)!=(oldAL&0xf0));
			F.b.A = 1;
		} else {
			F.b.A = 0;
		}
		if( oldAL > 0x99 || oldCF )
		{
			AL -= 0x60;
			F.b.C = 1;
		}
		setSZP8(AL);
		}break;
	case 0x30:{
		modrm m(this, read_next());
		temp8 = m.rm8();
		temp8 ^= m.reg8();
		m.rm8(temp8);
		setSZP8(temp8);
		F.b.C = F.b.O = 0;
		}break;
	case 0x31:{
		modrm m(this, read_next());
		temp16 = m.rm16();
		temp16 ^= m.reg16();
		m.rm16(temp16);
		setSZP16(temp16);
		F.b.C = F.b.O = 0;
		}break;
	case 0x32:{
		modrm m(this, read_next());
		temp8 = m.rm8();
		temp8 ^= m.reg8();
		m.reg8(temp8);
		setSZP8(temp8);
		F.b.C = F.b.O = 0;
		}break;
	case 0x33:{
		modrm m(this, read_next());
		temp16 = m.rm16();
		temp16 ^= m.reg16();
		m.reg16(temp16);
		setSZP16(temp16);
		F.b.C = F.b.O = 0;
		}break;	

	case 0x34:
		AL ^= read_next();
		setSZP8(AL);
		F.b.C = F.b.O = 0;
		break;		
	case 0x35:
		AX ^= read_next16();
		setSZP16(AX);
		F.b.C = F.b.O = 0;
		break;
		
		
	case 0x38:{
		modrm m(this, read_next());
		sub8(m.rm8(), m.reg8(), 0);	
		}break;
	case 0x39:{
		modrm m(this, read_next());
		sub16(m.rm16(), m.reg16(), 0);	
		}break;
	case 0x3A:{
		modrm m(this, read_next());
		sub8(m.reg8(), m.rm8(), 0);	
		}break;
	case 0x3B:{
		modrm m(this, read_next());
		sub16(m.reg16(), m.rm16(), 0);	
		}break;
	case 0x3C:
		temp8 = read_next();
		sub8(AL, temp8, 0);
		break;
	case 0x3D:
		temp16 = read_next16();
		sub16(AX, temp16, 0);
		break;
		
	case 0x3F:
		if( F.b.A || (AL & 0xf) > 9 )
		{
			AL -= 6;
			AH -= 1;
			AL &= 0xf;
			F.b.A = F.b.C = 1;
		} else {
			AL &= 0xf;
			F.b.A = F.b.C = 0;
		}
		setSZP8(AL);
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
		temp16 = r[opc&7].v;
		F.b.O = (temp16==0x7fff)?1:0;
		F.b.A = ((temp16&0xf)==0xf)?1:0;
		temp16++;
		setSZP16(temp16);
		r[opc&7].v = temp16;
		break;		
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4E:
	case 0x4F:
		temp16 = r[opc&7].v;
		F.b.O = (temp16==0x8000)?1:0;
		F.b.A = ((temp16&0xf)==0)?1:0;
		temp16--;
		setSZP16(temp16);
		r[opc&7].v = temp16;
		break;
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x55:
	case 0x56:
	case 0x57:
		push(r[opc&7].v);
		break;
	case 0x54:
		push(SP-2);
		break;
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5E:
	case 0x5F:
		r[opc&7].v = pop();
		break;		
	//case 0x60:
	case 0x70:
		temp8 = read_next();
		if( F.b.O )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x61:
	case 0x71:
		temp8 = read_next();
		if( F.b.O == 0 )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x62:
	case 0x72:
		temp8 = read_next();
		if( F.b.C )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x63:
	case 0x73:
		temp8 = read_next();
		if( F.b.C == 0 )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x64:
	case 0x74:
		temp8 = read_next();
		if( F.b.Z == 1 )
		{
			IP += (s8)temp8;
		}
		break;	
	//case 0x65:
	case 0x75:
		temp8 = read_next();
		if( F.b.Z == 0 )
		{
			IP += (s8)temp8;
		}
		break;		
	//case 0x66:
	case 0x76:
		temp8 = read_next();
		if( F.b.Z || F.b.C )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x67:
	case 0x77:
		temp8 = read_next();
		if( !F.b.Z && !F.b.C )
		{
			IP += (s8)temp8;
		}
		break;
	case 0x68:
		push(read16(CS, IP));
		IP+=2;
		break;
	case 0x78:
		temp8 = read_next();
		if( F.b.S )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x69:
	case 0x79:
		temp8 = read_next();
		if( F.b.S == 0 )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x6A:
	case 0x7A:
		temp8 = read_next();
		if( F.b.P )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x6B:
	case 0x7B:
		temp8 = read_next();
		if( F.b.P == 0 )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x6C:
	case 0x7C:
		temp8 = read_next();
		if( F.b.S != F.b.O )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x6D:
	case 0x7D:
		temp8 = read_next();
		if( F.b.S == F.b.O )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x6E:
	case 0x7E:
		temp8 = read_next();
		if( F.b.Z || (F.b.S != F.b.O) )
		{
			IP += (s8)temp8;
		}
		break;
	//case 0x6F:
	case 0x7F:
		temp8 = read_next();
		if( F.b.Z==0 && (F.b.S == F.b.O) )
		{
			IP += (s8)temp8;
		}
		break;	
		
		
	case 0x81:{
		modrm m(this, read_next());
		temp16 = read_next16();
		switch( (m.m>>3)&7 )
		{
		case 0: temp16 = add16(m.rm16(), temp16, 0); m.rm16(temp16); break;
		case 1: temp16 |= m.rm16(); m.rm16(temp16); setSZP16(temp16); F.b.O = F.b.C = 0; break;
		case 2: temp16 = add16(m.rm16(), temp16, F.b.C); m.rm16(temp16); break;
		case 3: temp16 = sub16(m.rm16(), temp16, F.b.C); m.rm16(temp16); break;
		case 4: temp16 &= m.rm16(); m.rm16(temp16); setSZP16(temp16); F.b.O = F.b.C = 0; break;
		case 5: temp16 = sub16(m.rm16(), temp16, 0); m.rm16(temp16); break;
		case 6: temp16 ^= m.rm16(); m.rm16(temp16); setSZP16(temp16); F.b.O = F.b.C = 0; break;
		case 7: sub16(m.rm16(), temp16, 0); break;
		default: break;
		}	
		}break;
		
	case 0x80:
	case 0x82:{
		modrm m(this, read_next());
		temp8 = read_next();
		switch( (m.m>>3)&7 )
		{
		case 0: temp8 = add8(m.rm8(), temp8, 0); m.rm8(temp8); break;
		case 1: temp8 |= m.rm8(); m.rm8(temp8); setSZP8(temp8); F.b.O = F.b.C = 0; break;
		case 2: temp8 = add8(m.rm8(), temp8, F.b.C); m.rm8(temp8); break;
		case 3: temp8 = sub8(m.rm8(), temp8, F.b.C); m.rm8(temp8); break;
		case 4: temp8 &= m.rm8(); m.rm8(temp8); setSZP8(temp8); F.b.O = F.b.C = 0; break;
		case 5: temp8 = sub8(m.rm8(), temp8, 0); m.rm8(temp8); break;
		case 6: temp8 ^= m.rm8(); m.rm8(temp8); setSZP8(temp8); F.b.O = F.b.C = 0; break;
		case 7: sub8(m.rm8(), temp8, 0); break;
		default: break;
		}	
		}break;
	case 0x83:{
		modrm m(this, read_next());
		temp16 = (s16)(s8)read_next();
		switch( (m.m>>3)&7 )
		{
		case 0: temp16 = add16(m.rm16(), temp16, 0); m.rm16(temp16); break;
		case 1: temp16 |= m.rm16(); m.rm16(temp16); setSZP16(temp16); F.b.O = F.b.C = 0; break;
		case 2: temp16 = add16(m.rm16(), temp16, F.b.C); m.rm16(temp16); break;
		case 3: temp16 = sub16(m.rm16(), temp16, F.b.C); m.rm16(temp16); break;
		case 4: temp16 &= m.rm16(); m.rm16(temp16); setSZP16(temp16); F.b.O = F.b.C = 0; break;
		case 5: temp16 = sub16(m.rm16(), temp16, 0); m.rm16(temp16); break;
		case 6: temp16 ^= m.rm16(); m.rm16(temp16); setSZP16(temp16); F.b.O = F.b.C = 0; break;
		case 7: sub16(m.rm16(), temp16, 0); break;
		default: break;
		}	
		}break;
	case 0x84:{
		modrm m(this, read_next());
		temp8 = m.rm8();
		temp8 &= m.reg8();
		setSZP8(temp8);
		F.b.C = F.b.O = 0;
		}break;	
	case 0x85:{
		modrm m(this, read_next());
		temp16 = m.rm16();
		temp16 &= m.reg16();
		setSZP16(temp16);	
		F.b.C = F.b.O = 0;
		}break;
	case 0x86:{
		modrm m(this, read_next());
		temp8 = m.reg8();
		u8 t2 = m.rm8();
		m.reg8(t2);
		m.rm8(temp8);	
		}break;	
	case 0x87:{
		modrm m(this, read_next());
		temp16 = m.reg16();
		u16 t2 = m.rm16();
		m.reg16(t2);
		m.rm16(temp16);	
		}break;
	case 0x88:{
		modrm m(this, read_next());
		temp8 = m.reg8();
		m.rm8(temp8);
		}break;
	case 0x89:{
		modrm m(this, read_next());
		temp16 = m.reg16();
		m.rm16(temp16);
		}break;
	case 0x8A:{
		modrm m(this, read_next());
		temp8 = m.rm8();
		m.reg8(temp8);
		}break;
	case 0x8B:{
		modrm m(this, read_next());
		temp16 = m.rm16();
		m.reg16(temp16);
		}break;
	case 0x8C:{
		modrm m(this, read_next());
		m.rm16(seg[(m.m>>3)&3]);
		}break;
	case 0x8D:{
		modrm m(this, read_next());
		m.reg16(m.ea_off);	
		}break;
	case 0x8E:{
		modrm m(this, read_next());
		seg[(m.m>>3)&3] = m.rm16();
		}break;	
	case 0x8F:{
		modrm m(this, read_next());
		temp16 = pop();
		m.rm16(temp16);
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
	case 0x98:
		AH = (AL&0x80) ? 0xff : 0;
		break;
	case 0x99:
		DX = (AX&0x8000) ? 0xffff : 0;
		break;
	case 0x9A:{
		u16 o = read_next16();
		u16 s = read_next16();
		push(CS);
		push(IP);
		CS = s;
		IP = o;	
		}break;
		
	case 0x9C:
		push(F.v);
		break;
	case 0x9D:
		F.v = pop();
		F.b.top4 = 0xf;
		F.b.pad1 = F.b.pad2 = 0;
		F.b.pad0 = 1;
		break;
	case 0x9E:
		F.v &= 0xff00;
		F.v |= AH;
		F.b.pad1 = F.b.pad2 = 0;
		F.b.pad0 = 1;
		break;		
	case 0x9F:
		AH = F.v;
		break;
	case 0xA0:
		temp16 = read_next16();
		AL = read((seg[seg_override(3)]<<4) + temp16);	
		break;		
	case 0xA1:
		temp16 = read_next16();
		AX = read16(seg[seg_override(3)], temp16);	
		break;
	case 0xA2:{
		temp16 = read_next16();
		u8 segind = seg_override(3);
		write((seg[segind]<<4)+temp16, AL);
		}break;
	case 0xA3:{
		temp16 = read_next16();
		u8 segind = seg_override(3);
		write16(seg[segind], temp16, AX);
		}break;
	case 0xA4: // movsb
		if( ANY_REP && CX == 0 ) break;
		temp8 = read((seg[seg_override(3)]<<4) + SI);
		write((ES<<4) + DI, temp8);
		//printf("%X:%X = $%X from %X:%X, CX=$%X\n", ES, DI, temp8, cpu.seg[segind], SI, CX);
		SI += (F.b.D ? -1 : 1);
		DI += (F.b.D ? -1 : 1);
		if( ANY_REP )
		{
			CX--;
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xA5: // movsw
		if( ANY_REP && CX == 0 ) break;
		temp16 = read16(seg[seg_override(3)], SI);
		write16(ES, DI, temp16);
		//printf("$%X from %X:%X to %X:%X\n", temp16, cpu.seg[segind], SI, ES, DI);
		SI += (F.b.D ? -2 : 2);
		DI += (F.b.D ? -2 : 2);
		if( ANY_REP )
		{
			CX--;
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xA6:{ // cmpsb
		if( ANY_REP && CX == 0 ) break;
		u8 x = read((ES<<4) + DI);
		u8 y = read((seg[seg_override(3)]<<4) + SI);
		sub8(y, x, 0);
		SI += (F.b.D ? -1 : 1);
		DI += (F.b.D ? -1 : 1);
		if( ANY_REP ) CX--;
		if( ((prefix & PREFIX_REP) && F.b.Z==1) || ((prefix & PREFIX_REPN) && F.b.Z==0) )
		{
			CS = start_cs;
			IP = start_ip;
		}
		}
		break;
	case 0xA7: // cmpsw
		if( ANY_REP && CX == 0 ) break;
		sub16(read16(seg[seg_override(3)], SI), read16(ES, DI), 0);
		SI += (F.b.D ? -2 : 2);
		DI += (F.b.D ? -2 : 2);
		if( ANY_REP ) CX--;
		if( ((prefix & PREFIX_REP) && F.b.Z==1) || ((prefix & PREFIX_REPN) && F.b.Z==0) )
		{
			CS = start_cs;
			IP = start_ip;
		}
		break;	
	case 0xA8:
		temp8 = AL & read_next();
		setSZP8(temp8);
		F.b.O = F.b.C = 0;
		break;	
	case 0xA9:
		temp16 = AX & read_next16();
		setSZP16(temp16);
		F.b.O = F.b.C = 0;
		break;
	case 0xAA: // stosb
		if( ANY_REP && CX == 0 ) break;
	 	write((ES<<4) + DI, AL);
		DI += (F.b.D ? -1 : 1);
		if( ANY_REP )
		{
			CX--;
			CS = start_cs;
			IP = start_ip;
		}		
		break;	
	case 0xAB: // stosw
		if( ANY_REP && CX == 0 ) break;
	 	write16(ES, DI, AX);
		DI += (F.b.D ? -2 : 2);
		if( ANY_REP )
		{
			CX--;
			CS = start_cs;
			IP = start_ip;
		}		
		break;	
	case 0xAC: // lodsb
		if( ANY_REP && CX == 0 ) break;
		AL = read((seg[seg_override(3)]<<4) + SI);
		SI += (F.b.D ? -1 : 1);
		if( ANY_REP )
		{
			CX--;
			CS = start_cs;
			IP = start_ip;
		}		
		break;
	case 0xAD: // lodsw
		if( ANY_REP && CX == 0 ) break;
	 	AX = read16(seg[seg_override(3)], SI);
	 	//printf("[$%X] is $%X\n", (seg[seg_override(3)]<<4) + SI, AX);
		SI += (F.b.D ? -2 : 2);
		if( ANY_REP )
		{
			CX--;
			CS = start_cs;
			IP = start_ip;
		}		
		break;
	case 0xAE: // scasb
		if( ANY_REP && CX == 0 ) break;
		sub8(AL, read((ES<<4) + DI), 0);
		DI += (F.b.D ? -1 : 1);
		if( ANY_REP ) CX--;
		if( ((prefix & PREFIX_REP) && F.b.Z==1) || ((prefix & PREFIX_REPN) && F.b.Z==0) )
		{
			CS = start_cs;
			IP = start_ip;
		}
		break;
	case 0xAF: // scasw
		if( ANY_REP && CX == 0 ) break;
		sub16(AX, read16(ES, DI), 0);
		DI += (F.b.D ? -2 : 2);
		if( ANY_REP ) CX--;
		if( ((prefix & PREFIX_REP) && F.b.Z==1) || ((prefix & PREFIX_REPN) && F.b.Z==0) )
		{
			CS = start_cs;
			IP = start_ip;
		}
		break;		
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
		r[opc&3].b.l = read_next();
		break;
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7:
		r[opc&3].b.h = read_next();	
		break;	
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBE:
	case 0xBF:
		r[opc&7].v = read_next16();	
		break;
	case 0xC0:
		temp16 = read_next16();
		IP = pop();
		SP += temp16;
		break;		
	case 0xC1:
		IP = pop();
		break;
	case 0xC2:{
		temp16 = read_next16();
		IP = pop();
		SP += temp16;	
		}break;
	case 0xC3:
		IP = pop();
		break;
	case 0xC4:{
		modrm m(this, read_next());
		u16 o = read16(m.ea_seg, m.ea_off);
		u16 s = read16(m.ea_seg, m.ea_off+2);
		ES = s;
		m.reg16(o);	
		}break;		
	case 0xC5:{
		modrm m(this, read_next());
		u16 o = read16(m.ea_seg, m.ea_off);
		u16 s = read16(m.ea_seg, m.ea_off+2);
		DS = s;
		m.reg16(o);	
		}break;
	case 0xC6:{
		modrm m(this, read_next());
		temp8 = read_next();
		m.rm8(temp8);
		}break;
	case 0xC7:{
		modrm m(this, read_next());
		temp16 = read_next16();
		m.rm16(temp16);
		}break;
	case 0xC8:
		temp16 = read_next16();
		IP = pop();
		CS = pop();
		SP += temp16;
		break;
	case 0xC9:
		IP = pop();
		CS = pop();
		break;
	case 0xCA:
		temp16 = read_next16();
		IP = pop();
		CS = pop();
		SP += temp16;
		break;		
	case 0xCB:
		IP = pop();
		CS = pop();
		break;
	case 0xCC:{
		u32 vec = 3*4;
		u16 o = read16(0, vec);
		u16 s = read16(0, vec+2);
		push(F.v);
		push(CS);
		push(IP);
		CS = s;
		IP = o;	
		}break;
	case 0xCD:{
		u32 vec = read_next()*4;
		u16 o = read16(0, vec);
		u16 s = read16(0, vec+2);
		push(F.v);
		push(CS);
		push(IP);
		CS = s;
		IP = o;	
		}break;
	case 0xCE:
		if( F.b.O )
		{
			u32 vec = 4*4;
			u16 o = read16(0, vec);
			u16 s = read16(0, vec+2);
			push(F.v);
			push(CS);
			push(IP);
			CS = s;
			IP = o;
		}
		break;		
	case 0xCF:
		IP = pop();
		CS = pop();
		F.v = pop();
		F.b.top4 = 0xf;
		F.b.pad1 = F.b.pad2 = 0;
		F.b.pad0 = 1;
		break;
		
	case 0xD0:{
		modrm m(this, read_next());
		u8 c = F.b.C;
		switch( (m.m>>3)&7 )
		{
		case 0:
			temp8 = m.rm8();
			F.b.C = temp8>>7;
			temp8 = (temp8<<1)|(temp8>>7);
			m.rm8(temp8);
			F.b.O = F.b.C ^ (temp8>>7);
			break;
		case 1:
			temp8 = m.rm8();
			F.b.C = temp8 & 1;
			temp8 = (temp8>>1)|(temp8<<7);
			m.rm8(temp8);
			F.b.O = ((temp8>>6)&1) ^ (temp8>>7);
			break;
		case 2:
			temp8 = m.rm8();
			F.b.C = temp8>>7;
			temp8 = (temp8<<1)|c;
			m.rm8(temp8);
			F.b.O = F.b.C ^ (temp8>>7);
			break;
		case 3:
			temp8 = m.rm8();
			F.b.C = temp8 & 1;
			temp8 = (temp8>>1)|(c<<7);
			m.rm8(temp8);
			F.b.O = ((temp8>>6)&1) ^ (temp8>>7);
			break;
		case 4:
			temp8 = m.rm8();
			F.b.C = temp8>>7;
			temp8 <<= 1;
			F.b.O = F.b.C ^ (temp8>>7);
			setSZP8(temp8);
			m.rm8(temp8);
			break;
		case 5:
			temp8 = m.rm8();
			F.b.O = temp8>>7;
			F.b.C = temp8&1;
			temp8 >>= 1;
			setSZP8(temp8);
			m.rm8(temp8);
			break;
		case 6: // setmo, might be duplicate of case 4 on later cpus?
			m.rm8(0xff);
			F.b.C = F.b.O = F.b.A = F.b.Z = 0;
			F.b.P = F.b.S = 1;
			break;
		case 7:
			temp8 = m.rm8();
			F.b.C = temp8 & 1;
			temp8 = (temp8&0x80)|(temp8>>1);
			setSZP8(temp8);
			F.b.O = 0;
			m.rm8(temp8);
			break;
		default: printf("unimpl D0 subcode %i\n", (m.m>>3)&7); exit(1);
		}		
		}break;	
	case 0xD1:{
		modrm m(this, read_next());
		u8 c = F.b.C;
		switch( (m.m>>3)&7 )
		{
		case 0:
			temp16 = m.rm16();
			F.b.C = temp16>>15;
			temp16 = (temp16<<1)|(temp16>>15);
			m.rm16(temp16);
			F.b.O = F.b.C ^ (temp16>>15);
			break;
		case 1:
			temp16 = m.rm16();
			F.b.C = temp16 & 1;
			temp16 = (temp16>>1)|(temp16<<15);
			m.rm16(temp16);
			F.b.O = ((temp16>>14)&1) ^ (temp16>>15);
			break;
		case 2:
			temp16 = m.rm16();
			F.b.C = temp16>>15;
			temp16 = (temp16<<1)|c;
			m.rm16(temp16);
			F.b.O = F.b.C ^ (temp16>>15);
			break;
		case 3:
			temp16 = m.rm16();
			F.b.C = temp16 & 1;
			temp16 = (temp16>>1)|(c<<15);
			m.rm16(temp16);
			F.b.O = ((temp16>>14)&1) ^ (temp16>>15);
			break;
		case 4:
			temp16 = m.rm16();
			F.b.C = temp16>>15;
			temp16 <<= 1;
			F.b.O = F.b.C ^ (temp16>>15);
			setSZP16(temp16);
			m.rm16(temp16);
			break;	
		case 5:
			temp16 = m.rm16();
			F.b.O = temp16>>15;
			F.b.C = temp16&1;
			temp16 >>= 1;
			setSZP16(temp16);
			m.rm16(temp16);
			break;
		case 6: // setmo, might be duplicate of case 4 on later cpus?
			m.rm16(0xffff);
			F.b.C = F.b.O = F.b.A = F.b.Z = 0;
			F.b.P = F.b.S = 1;
			break;
		case 7:
			temp16 = m.rm16();
			F.b.C = temp16 & 1;
			temp16 = (temp16&0x8000)|(temp16>>1);
			setSZP16(temp16);
			F.b.O = 0;
			m.rm16(temp16);
			break;
		default: printf("unimpl D1 subcode %i\n", (m.m>>3)&7); exit(1);
		}		
		}break;
		
	case 0xD2:{
		modrm m(this, read_next());
		switch( (m.m>>3)&7 )
		{
		case 0: 
			temp8 = m.rm8();
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp8 >> 7;
				temp8 = (temp8<<1)|(temp8>>7);
			}
			if( CL != 0 )
			{
				if( CL == 1 ) F.b.O = (temp8>>7) ^ F.b.C;
				m.rm8(temp8);
			}
			break;
		case 1: 
			temp8 = m.rm8();
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp8 & 1;
				temp8 = (temp8>>1)|(temp8<<7);
			}
			if( CL != 0 )
			{
				if( CL == 1 ) F.b.O =  ((temp8>>6)&1) ^ (temp8>>7);
				m.rm8(temp8);
			}
			break;
		case 2: 
			temp8 = m.rm8();
			for(u32 i = 0; i < CL; ++i)
			{
				u8 c = F.b.C;
				F.b.C = temp8 >> 7;
				temp8 <<= 1;
				temp8 |= c;
			}
			if( CL != 0 )
			{
				if( CL == 1 ) F.b.O = (temp8>>7) ^ F.b.C;
				m.rm8(temp8);
			}
			break;
		case 3: 
			temp8 = m.rm8();
			for(u32 i = 0; i < CL; ++i)
			{
				u8 c = F.b.C;
				F.b.C = temp8 & 1;
				temp8 >>= 1;
				temp8 |= c<<7;
			}
			if( CL != 0 )
			{
				if( CL == 1 ) F.b.O = (temp8>>7) ^ ((temp8>>6)&1);
				m.rm8(temp8);
			}
			break;
		case 4: 
			temp8 = m.rm8();
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp8>>7;
				temp8 <<= 1;
			}
			if( CL != 0 )
			{
				setSZP8(temp8);
				if( CL == 1 ) F.b.O = ((temp8>>7)==F.b.C)?0:1;
				m.rm8(temp8);
			}			
			break;
		case 5: 
			temp8 = m.rm8();
			if( CL == 1 ) F.b.O = temp8>>7;
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp8 & 1;
				temp8 >>= 1;
			}
			if( CL != 0 )
			{
				setSZP8(temp8);
				m.rm8(temp8);
			}			
			break;
		case 6: // setmoc, might be duplicate of case 4 on later cpus?
			if( CL )
			{
				m.rm8(0xff);
				F.b.C = F.b.O = F.b.A = F.b.Z = 0;
				F.b.P = F.b.S = 1;
			}
			break;
		case 7:
			temp8 = m.rm8();
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp8 & 1;
				temp8 = (temp8&0x80)|(temp8>>1);
			}
			if( CL != 0 )
			{
				setSZP8(temp8);
				if( CL == 1 ) F.b.O = 0;
				m.rm8(temp8);
			}
			break;
		default: printf("Unimpl D2 subcode %i\n", (m.m>>3)&7); exit(1);
		}	
		}break;		
	case 0xD3:{
		modrm m(this, read_next());
		switch( (m.m>>3)&7 )
		{
		case 0: 
			temp16 = m.rm16();
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp16 >> 15;
				temp16 = (temp16<<1)|(temp16>>15);
			}
			if( CL != 0 )
			{
				if( CL == 1 ) F.b.O = (temp16>>15) ^ F.b.C;
				m.rm16(temp16);
			}
			break;
		case 1: 
			temp16 = m.rm16();
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp16 & 1;
				temp16 = (temp16>>1)|(temp16<<15);
			}
			if( CL != 0 )
			{
				if( CL == 1 ) F.b.O = ((temp16>>14)&1) ^ (temp16>>15);
				m.rm16(temp16);
			}
			break;
		case 2: 
			temp16 = m.rm16();
			for(u32 i = 0; i < CL; ++i)
			{
				u8 c = F.b.C;
				F.b.C = temp16 >> 15;
				temp16 <<= 1;
				temp16 |= c;
			}
			if( CL != 0 )
			{
				if( CL == 1 ) F.b.O = (temp16>>15) ^ F.b.C;
				m.rm16(temp16);
			}
			break;
		case 3: 
			temp16 = m.rm16();
			for(u32 i = 0; i < CL; ++i)
			{
				u8 c = F.b.C;
				F.b.C = temp16 & 1;
				temp16 >>= 1;
				temp16 |= c<<15;
			}
			if( CL != 0 )
			{
				if( CL == 1 ) F.b.O = (temp16>>15) ^ ((temp16>>14)&1);
				m.rm16(temp16);
			}
			break;
		case 4: 
			temp16 = m.rm16();
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp16>>15;
				temp16 <<= 1;
			}
			if( CL != 0 )
			{
				setSZP16(temp16);
				if( CL == 1 ) F.b.O = ((temp16>>15)==F.b.C)?0:1;
				m.rm16(temp16);
			}			
			break;
		case 5: 
			temp16 = m.rm16();
			if( CL == 1 ) F.b.O = temp16>>15;
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp16 & 1;
				temp16 >>= 1;
			}
			if( CL != 0 )
			{
				setSZP16(temp16);
				m.rm16(temp16);
			}			
			break;
		case 6: // setmoc, might be duplicate of case 4 on later cpus?
			if( CL )
			{
				m.rm16(0xffff);
				F.b.C = F.b.O = F.b.A = F.b.Z = 0;
				F.b.P = F.b.S = 1;
			}
			break;
		case 7:
			temp16 = m.rm16();
			for(u32 i = 0; i < CL; ++i)
			{
				F.b.C = temp16 & 1;
				temp16 = (temp16&0x8000)|(temp16>>1);
			}
			if( CL != 0 )
			{
				setSZP16(temp16);
				if( CL == 1 ) F.b.O = 0;
				m.rm16(temp16);
			}
			break;
		default: printf("Unimpl D3 subcode %i\n", (m.m>>3)&7); exit(1);
		}	
		}break;
	case 0xD4: //todo: aam	
		break;		
	case 0xD5:
		temp8 = read_next();
		AL = (AL + (AH * temp8));
		AH = 0;
		setSZP8(AL);
		break;
	case 0xD6:
		AL = F.b.C ? 0xff : 0;
		break;
	case 0xD7:
		temp16 = BX + AL;
		AL = read((seg[seg_override(3)]<<4) + temp16);	
		break;
	case 0xD8:
	case 0xD9:
	case 0xDA:
	case 0xDB:
	case 0xDC:
	case 0xDD:
	case 0xDE:
	case 0xDF:{  // stub non-existent fpu
		modrm m(this, read_next());
		}break;
	case 0xE0:
		temp8 = read_next();
		CX--;
		if( CX && F.b.Z == 0 ) IP += (s8)temp8;
		break;
	case 0xE1:
		temp8 = read_next();
		CX--;
		if( CX && F.b.Z == 1 ) IP += (s8)temp8;
		break;
	case 0xE2:
		temp8 = read_next();
		CX--;
		if( CX ) IP += (s8)temp8;
		break;	
	case 0xE3:
		temp8 = read_next();
		if( CX == 0 ) IP += (s8)temp8;
		break;
	case 0xE4:
		temp8 = read_next();
		AL = port_in(temp8, 8);
		break;
	case 0xE5:
		temp8 = read_next();
		AX = port_in(temp8, 16);
		break;	
	case 0xE6:
		temp8 = read_next();
		port_out(temp8, AL, 8);	
		break;
	case 0xE7:
		temp8 = read_next();
		port_out(temp8, AX, 16);
		break;
	case 0xE8:
		temp16 = read_next16();
		push(IP);
		IP += temp16;	
		break;
	case 0xE9:
		temp16 = read_next16();
		IP += temp16;	
		break;
	case 0xEA:{
		u16 o = read_next16();
		u16 s = read_next16();
		IP = o;
		CS = s;	
		}break;
	case 0xEB:
		temp8 = read_next();
		IP += (s8)temp8;
		break;
	case 0xEC: AL = port_in(DX, 8); break;
	case 0xED: AX = port_in(DX, 16); break;
	case 0xEE: port_out(DX, AL, 8); break;		
	case 0xEF: port_out(DX, AX, 16); break;
		
	case 0xF4: halted = true; break;
	case 0xF5: F.b.C ^= 1; break;
	case 0xF6:{
		modrm m(this, read_next());
		switch( (m.m>>3)&7 )
		{
		case 0:
		case 1:
			temp8 = read_next();
			temp8 &= m.rm8();
			setSZP8(temp8);
			F.b.C = F.b.O = 0;		
			break;
		case 2:
			temp8 = m.rm8();
			m.rm8(~temp8);
			break;
		case 3:
			temp8 = sub8(0, m.rm8(), 0);
			m.rm8(temp8);
			break;
		case 4:{
			temp16 = AL;
			temp16 *= u16(m.rm8());
			AX = temp16;
			setSZP8(AH);
			F.b.C = F.b.O = (AH ? 1:0);
			}break;
		case 5:{
			s16 a = s16(s8(AL));
			s16 b = s16(s8(m.rm8()));
			AX = a * b;
			setSZP8(AH);
			F.b.C = F.b.O = ((AX!=(u16)s16(s8(AL)))?1:0);
			}break;
		default: printf("F6 unimpl subcode %i\n", (m.m>>3)&7); exit(1);
		}
		}break;
	case 0xF7:{
		modrm m(this, read_next());
		switch( (m.m>>3)&7 )
		{
		case 0:
		case 1:
			temp16 = read_next16();
			temp16 &= m.rm16();
			setSZP16(temp16);
			F.b.C = F.b.O = 0;
			break;
		case 2:
			temp16 = m.rm16();
			m.rm16(~temp16);
			break;
		case 3:
			temp16 = sub16(0, m.rm16(), 0);
			m.rm16(temp16);
			break;
		case 4:{
			u32 t = AX;
			t *= u32(m.rm16());
			DX = t>>16;
			AX = t;
			setSZP16(DX);
			F.b.C = F.b.O = (DX?1:0);
			}break;
		case 5:{
			s32 a = s32(s16(AX));
			s32 b = a * s32(s16(m.rm16()));
			DX = b>>16;
			AX = b;
			setSZP16(DX);
			F.b.C = F.b.O = ((b!=s32(s16(b)))?1:0);
			}break;	
		case 6:{
			u32 t = (DX<<16)|AX;
			AX = t / m.rm16();
			DX = t % m.rm16();
			//todo: flags & exception
			}break;
		default: printf("Unimpl F7 subcode %i\n", (m.m>>3)&7); exit(1);
		}	
		}break;	
	case 0xF8: F.b.C = 0; break;
	case 0xF9: F.b.C = 1; break;
	case 0xFA: F.b.I = 0; break;
	case 0xFB: F.b.I = 1; irq_blocked = true; break;
	case 0xFC: F.b.D = 0; break;
	case 0xFD: F.b.D = 1; break;
	case 0xFE:{
		modrm m(this, read_next());
		switch( (m.m>>3)&7 )
		{
		case 0:
			temp8 = m.rm8();
			F.b.O = (temp8==0x7f)?1:0;
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
		default: printf("FE unimpl subcode %i\n", (m.m>>3)&7); exit(1);
		}	
		}break;			
	case 0xFF:{
		modrm m(this, read_next());
		switch( (m.m>>3)&7 )
		{
		case 0:
			temp16 = m.rm16();
			F.b.O = (temp16==0x7fff)?1:0;
			F.b.A = ((temp16&0xf)==0xf)?1:0;
			temp16++;
			m.rm16(temp16);
			setSZP16(temp16);			
			break;
		case 1:
			temp16 = m.rm16();
			F.b.O = (temp16==0x8000)?1:0;
			F.b.A = ((temp16&0xf)==0)?1:0;
			temp16--;
			m.rm16(temp16);
			setSZP16(temp16);			
			break;
		case 2:
			temp16 = m.rm16();
			push(IP);
			IP = temp16;
			break;
		case 3:{
			u16 no = read16(m.ea_seg, m.ea_off);
			u16 ns = read16(m.ea_seg, m.ea_off+2);
			push(CS);
			push(IP);
			IP = no;
			CS = ns;		
			}break;
		case 4:{
			IP = m.rm16();
			}break;	
		case 5:{
			u16 no = read16(m.ea_seg, m.ea_off);
			u16 ns = read16(m.ea_seg, m.ea_off+2);
			IP = no;
			CS = ns;		
			}break;
		case 6:
		case 7:
			temp16 = m.rm16();
			push(temp16);
			break;
		default: printf("FF unimpl subcode %i\n", (m.m>>3)&7); exit(1);
		}	
		}break;		
	default:
		printf("$%X:$%X: Unimpl instruction $%02X\n", CS, IP-1, opc);
		exit(1);
	}
	return 0;
}

void c80286::push(u16 v)
{
	SP -= 2;
	write16(SS, SP, v);
}

u16 c80286::pop()
{
	u16 res = read16(SS, SP);
	SP += 2;
	return res;
}

u8 c80286::add8(u8 a, u8 b, u8 c)
{
	u16 res = a;
	res += b;
	res += c;
	F.b.O = ((res^b) & (res^a) & 0x80)?1:0;
	F.b.A = (((a&0xf) + (b&0xf) + c)>0xf)?1:0;
	F.b.C = res>>8;
	setSZP8(u8(res));
	return res;
}

u8 c80286::sub8(u8 a, u8 b, u8 c)
{
	u8 res = add8(a, b^0xff, c^1);
	F.b.A ^= 1;
	F.b.C ^= 1;
	return res;
}

u16 c80286::add16(u16 a, u16 b, u16 c)
{
	u32 res = a;
	res += b;
	res += c;
	F.b.O = ((res^b) & (res^a) & 0x8000)?1:0;
	F.b.A = (((a&0xf) + (b&0xf) + c)>0xf)?1:0;
	F.b.C = res>>16;
	setSZP16(u16(res));
	return res;
}

u16 c80286::sub16(u16 a, u16 b, u16 c)
{
	u16 res = add16(a, b^0xffff, c^1);
	F.b.A ^= 1;
	F.b.C ^= 1;
	return res;
}

void c80286::reset()
{
	IP = 0;
	CS = 0xffff;
}

bool c80286::is_prefix(u8 p)
{
	switch( p )
	{
	case 0xF0: return true; //todo: lock
	case 0xF2: prefix |= PREFIX_REPN; return true;
	case 0xF3: prefix |= PREFIX_REP; return true;
	case 0x2E: prefix &=~0xf; prefix |= PREFIX_CS; return true;
	case 0x36: prefix &=~0xf; prefix |= PREFIX_SS; return true;
	case 0x3E: prefix &=~0xf; prefix |= PREFIX_DS; return true;
	case 0x26: prefix &=~0xf; prefix |= PREFIX_ES; return true;
	default: break;
	}
	return false;
}

u8 c80286::seg_override(u8 index)
{
	if( prefix & 0xf )
	{
		return (prefix & 0xf)-1;
	}
	return index;
}












