#include <print>
#include <cstdlib>
#include "snes.h"

#define check14(rn) if( (rn) == 14 ) gsu.rombuf = gread(gsu.romb, gsu.r[14])
#define gread(b, a) gsu_read(b, a)
#define gwrite(a, v) gsu_write(a, v)
#define ALT1 gsu.F.b.a1
#define ALT2 gsu.F.b.a2
#define ALT3 (gsu.F.b.a1 && gsu.F.b.a2)
#define FROM gsu.from_reg
#define TO gsu.to_reg
#define setsz(V) gsu_setSZ(V)

void snes::gsu_exec(u8 opc)
{
	u16 t = 0;
	switch( opc )
	{
	case 0x00: 
		gsu.F.b.GO = 0;
		gsu.F.b.IRQ = 1;
		cpu.irq_line = true;
		break;
	case 0x01: break; /*nop*/
	case 0x02: break; /*cache*/
	case 0x03: // lsr
		gsu.F.b.C = gsu.r[FROM]&1;
		gsu.r[TO] = setsz(gsu.r[FROM]>>1);
		gsu.F.b.V = 0; //??
		check14(TO);
		break;
	case 0x04: // rol
		t = gsu.F.b.C;
		gsu.F.b.C = gsu.r[FROM]>>15;
		gsu.r[TO] = setsz( (gsu.r[FROM]<<1)|t );
		break;	
	case 0x05: // bra
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		gsu.r[15] += s8(t) - 1;
		gsu.stamp += 1;
		break;
	case 0x06: // bge
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.S == gsu.F.b.V )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x07: // blt
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.S != gsu.F.b.V )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x08: // bne
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.Z == 0 )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x09: // beq
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.Z == 1 )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x0A: // bpl
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.S == 0 )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x0B: // bmi
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.S == 1 )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x0C: // bcc
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.C == 0 )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x0D: // bcs
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.C == 1 )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x0E: // bvc
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.V == 0 )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x0F: // bvs
		gsu.stamp += 1;
		t = gread(gsu.pb, gsu.r[15]); gsu.r[15] += 1;
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		if( gsu.F.b.V == 1 )
		{
			gsu.r[15] += s8(t) - 1;
		}
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x15:
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x1E:
	case 0x1F:
		gsu.to_reg = opc&15;
		if( gsu.F.b.B )
		{
			gsu.r[TO] = gsu.r[FROM];
			check14(TO);
			break;
		}
		return;
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x27:
	case 0x28:
	case 0x29:
	case 0x2A:
	case 0x2B:
	case 0x2C:
	case 0x2D:
	case 0x2E:
	case 0x2F:
		gsu.to_reg = gsu.from_reg = opc&15;
		gsu.F.b.B = 1;
		return;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3B: // stb / stw
		if( ALT1 )
		{
			gwrite(gsu.r[opc&15], gsu.r[FROM]);
		} else {
			gwrite(gsu.r[opc&15], gsu.r[FROM]);
			gwrite(gsu.r[opc&15]+1, gsu.r[FROM]>>8);
		}
		break;
	case 0x3C: 
		gsu.r[12] -= 1; 
		setsz(gsu.r[12]);
		if( gsu.F.b.Z==0 ) 
		{ 
			gsu.r[15]=gsu.r[13]-1; 
		} 
		break;
		
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x46:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
		if( ALT1 )
		{
			gsu.r[TO] = (s8)gread(gsu.ramb, gsu.r[opc&15]); //todo: sign extend??
			gsu.stamp += 7;
		} else {
			gsu.r[TO] = gread(gsu.ramb, gsu.r[opc&15]);
			gsu.r[TO] |= gread(gsu.ramb, gsu.r[opc&15]^1)<<8;
			gsu.stamp += 10;
		}
		break;
	case 0x4C: // plot / rpix
		if( ALT1 )
		{
			std::println("superfx: rpix todo");
			//exit(1);
		} else {
			gsu_plot();
		}
		break;
	case 0x4D: 	
		gsu.r[TO] = setsz((gsu.r[FROM]<<8)|(gsu.r[FROM]>>8)); 
		check14(TO);
		break;
	case 0x4E: 
		if( ALT1 )
		{
			gsu.plotopt = gsu.r[FROM];
		} else {
			gsu.color = gsu.r[FROM];
		}
		break;
	case 0x4F: 			
		gsu.r[TO] = setsz(~gsu.r[FROM]); 
		check14(TO);
		break;
	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x56:
	case 0x57:
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5E:
	case 0x5F:
		gsu.r[TO] = gsu_add(gsu.r[FROM], (ALT2 ? (opc&15) : gsu.r[opc&15]), (ALT1 ? gsu.F.b.C : 0));
		check14(TO);
		break;
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x64:
	case 0x65:
	case 0x66:
	case 0x67:
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6E:
	case 0x6F:
		if( ALT3 )
		{
			gsu_add(gsu.r[FROM], ~gsu.r[opc&15], 1);
			return;
		}
		gsu.r[TO] = gsu_add(gsu.r[FROM], ~(ALT2 ? (opc&15) : gsu.r[opc&15]), (ALT1 ? gsu.F.b.C : 1));
		check14(TO);
		break;
	case 0x70:
		gsu.F.b.S = (gsu.r[7]|gsu.r[8])>>15;
		gsu.F.b.V = (gsu.r[7]|gsu.r[8]|(gsu.r[7]<<1)|(gsu.r[8]<<1))>>15;
		gsu.F.b.C = (gsu.r[7]|gsu.r[8]|(gsu.r[7]<<1)|(gsu.r[8]<<1)|(gsu.r[7]<<2)|(gsu.r[8]<<2))>>15;
		gsu.F.b.Z = (gsu.r[7]|gsu.r[8]|(gsu.r[7]<<1)|(gsu.r[8]<<1)|(gsu.r[7]<<2)|(gsu.r[8]<<2)|(gsu.r[7]<<3)|(gsu.r[8]<<3))>>15;
		gsu.r[TO] = (gsu.r[7]&0xff00)|(gsu.r[8]>>8);
		check14(TO);
		break;
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x76:
	case 0x77:
	case 0x78:
	case 0x79:
	case 0x7A:
	case 0x7B:
	case 0x7C:
	case 0x7D:
	case 0x7E:
	case 0x7F:
		t = (ALT2 ? (opc&15) : gsu.r[opc&15]);
		if( ALT1 ) t = ~t;
		gsu.r[TO] = setsz(gsu.r[FROM] & t);
		check14(TO);
		gsu.F.b.V = gsu.F.b.C = 0;
		break;
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
	case 0x8D:
	case 0x8E:
	case 0x8F:
		if( ALT3 )
		{
			gsu.r[TO] = (gsu.r[FROM]&0xff)*(opc&15);
		} else if( ALT2 ) {
			gsu.r[TO] = s8(gsu.r[FROM])*(opc&15);
		} else if( ALT1 ) {
			gsu.r[TO] = (gsu.r[FROM]&0xff)*(gsu.r[opc&15]&0xff);
		} else {
			gsu.r[TO] = s8(gsu.r[FROM]) * s8(gsu.r[opc&15]);
		}
		setsz(gsu.r[TO]);
		check14(TO);
		break;
	case 0x90: // sbk
		gwrite(gsu.last_rdaddr, gsu.r[FROM]);
		gwrite(gsu.last_rdaddr^1, gsu.r[FROM]>>8);	
		break;
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94: // link #n
		gsu.r[11] = gsu.r[15] + (opc&15);
		break;
	case 0x95: 	
		gsu.r[TO] = setsz(s8(gsu.r[FROM]));
		check14(TO);
		break;
	case 0x96: // asr / div2
		gsu.F.b.C = gsu.r[FROM]&1;
		if( ALT1 && gsu.r[FROM] == 0xffff )
		{
			gsu.r[TO] = setsz(0);
		} else {
			gsu.r[TO] = setsz(s16(gsu.r[FROM])>>1);
		}
		gsu.F.b.V = 0;  // ??
		check14(TO);
		break;
	case 0x97: // ror
		t = gsu.F.b.C;
		gsu.F.b.C = gsu.r[FROM]&1;
		gsu.r[TO] = setsz( (gsu.r[FROM]>>1)|(t<<15) );
		check14(TO);
		break;
	case 0x98:
	case 0x99:
	case 0x9A:
	case 0x9B:
	case 0x9C:
	case 0x9D:
		if( ALT1 )
		{
			gsu.pb = gsu.r[opc&15];
			gsu.cache_base = gsu.r[FROM] & 0xfff0;
			//todo: doc says "cache flags cleared"
			gsu.r[15] = gsu.r[FROM];
		} else {
			gsu.r[15] = gsu.r[opc&15];
		}
		break;
	case 0x9E: // lob
		gsu.r[TO] = setsz( gsu.r[FROM] & 0xff );
		gsu.F.b.S = (gsu.r[TO]>>7)&1;
		check14(TO);
		break;
	case 0x9F:
		if( ALT1 )
		{ // lmult
			s32 mres = s16(gsu.r[6]);
			mres *= s16(gsu.r[FROM]);
			if( gsu.to_reg != 4 )
			{
				gsu.r[TO] = mres>>16;
			}
			gsu.r[4] = mres;
			gsu.F.b.C = ((mres&BIT(15)) ? 1:0);
			setsz(gsu.r[TO]); //todo: this or r4?
			check14(TO);
			gsu.stamp += 3;
		} else { //fmult
			s32 mres = s16(gsu.r[6]);
			mres *= s16(gsu.r[FROM]);
			if( gsu.to_reg != 4 )
			{
				gsu.r[TO] = mres>>16;
			}
			gsu.F.b.C = ((mres&BIT(15)) ? 1:0);
			setsz(gsu.r[TO]);
			check14(TO);
			gsu.stamp += 3;
		}
		break;		
	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	case 0xA4:
	case 0xA5:
	case 0xA6:
	case 0xA7:
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAE:
	case 0xAF: // ibt / lms
		if( ALT1 )
		{
			t = gread(gsu.pb, gsu.r[15]++)<<1;
			gsu.stamp += 12;
			gsu.r[opc&15] = gread(gsu.ramb, t++);
			gsu.r[opc&15] |= gread(gsu.ramb, t)<<8;
			check14(opc&15);
		} else if( ALT2 ) {
			t = gread(gsu.pb, gsu.r[15]++)<<1;
			gsu.stamp += 3;
			gwrite(t++, gsu.r[opc&15]);
			gwrite(t, gsu.r[opc&15]>>8);
		} else {
			gsu.stamp += 1;
			gsu.r[opc&15] = (s8)gread(gsu.pb, gsu.r[15]++);
			check14(opc&15);
		}
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		break;
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB6:
	case 0xB7:
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBE:
	case 0xBF:
		gsu.from_reg = opc&15;
		if( gsu.F.b.B )
		{
			gsu.r[TO] = setsz(gsu.r[FROM]);
			gsu.F.b.V = (gsu.r[TO]>>7)&1;
			check14(TO);
			break;
		}
		return;
	case 0xC0:
		gsu.r[TO] = setsz(gsu.r[FROM]>>8);
		gsu.F.b.S = (gsu.r[TO]>>7)&1;
		gsu.F.b.C = gsu.F.b.V = 0;
		check14(TO);
		break;
	case 0xC1:
	case 0xC2:
	case 0xC3:
	case 0xC4:
	case 0xC5:
	case 0xC6:
	case 0xC7:
	case 0xC8:
	case 0xC9:
	case 0xCA:
	case 0xCB:
	case 0xCC:
	case 0xCD:
	case 0xCE:
	case 0xCF:
		if( ALT1 )
		{
			gsu.r[TO] = setsz(gsu.r[FROM] ^ (ALT2 ? (opc&15) : gsu.r[opc&15]));
		} else {
			gsu.r[TO] = setsz(gsu.r[FROM] | (ALT2 ? (opc&15) : gsu.r[opc&15]));
		}
		check14(TO);
		break;
	case 0xD0:
	case 0xD1:
	case 0xD2:
	case 0xD3:
	case 0xD4:
	case 0xD5:
	case 0xD6:
	case 0xD7:
	case 0xD8:
	case 0xD9:
	case 0xDA:
	case 0xDB:
	case 0xDC:
	case 0xDD:
	case 0xDE: // inc Rn
		gsu.r[opc&15] = setsz(gsu.r[opc&15]+1);
		check14(opc&15);
		break;		
	case 0xDF: // getc
		if( ALT3 ) {
			gsu.romb = gsu.r[FROM];
		} else if( ALT2 ) {
			gsu.ramb = 0x70 | (gsu.r[FROM]&1);
		} else {
			gsu.color = gsu.rombuf;
		}
		break;
	case 0xE0:
	case 0xE1:
	case 0xE2:
	case 0xE3:
	case 0xE4:
	case 0xE5:
	case 0xE6:
	case 0xE7:
	case 0xE8:
	case 0xE9:
	case 0xEA:
	case 0xEB:
	case 0xEC:
	case 0xED:
	case 0xEE: // dec Rn
		gsu.r[opc&15] = setsz(gsu.r[opc&15]-1);
		check14(opc&15);
		break;
	case 0xEF: // getb/bh/bl/bs
		if( ALT3 )
		{ // getbs
			gsu.r[TO] = (s8)gsu.rombuf;
		} else if( ALT2 ) {
			// getbl
			gsu.r[TO] = (gsu.r[FROM]&0xff00)|gsu.rombuf;
		} else if( ALT1 ) {
			// getbh
			gsu.r[TO] = (gsu.r[FROM]&0xff)|(gsu.rombuf<<8);		
		} else { // getb
			gsu.r[TO] = gsu.rombuf;
		}
		check14(TO);
		break;
	case 0xF0:
	case 0xF1:
	case 0xF2:
	case 0xF3:
	case 0xF4:
	case 0xF5:
	case 0xF6:
	case 0xF7:
	case 0xF8:
	case 0xF9:
	case 0xFA:
	case 0xFB:
	case 0xFC:
	case 0xFD:
	case 0xFE:
	case 0xFF: // iwt  / lm
		if( ALT1 )
		{
			t = gread(gsu.pb, gsu.r[15]++);
			t |= gread(gsu.pb, gsu.r[15]++)<<8;
			gsu.r[opc&15] = gread(gsu.ramb, t++);
			gsu.r[opc&15] |= gread(gsu.ramb, t)<<8;
			gsu.stamp += 13;
			check14(opc&15);
		} else if( ALT2 ) {
			t = gread(gsu.pb, gsu.r[15]++);
			t |= gread(gsu.pb, gsu.r[15]++)<<8;
			gwrite(t++, gsu.r[opc&15]);
			gwrite(t, gsu.r[opc&15]>>8);
			gsu.stamp += 2;
		} else {
			t = gread(gsu.pb, gsu.r[15]++);
			t |= gread(gsu.pb, gsu.r[15]++)<<8;
			gsu.r[opc&15] = t;
			gsu.stamp += 2;
			check14(opc&15);
		}
		gsu.fetch = gread(gsu.pb, gsu.r[15]);
		break;	
	case 0x3D: gsu.F.b.a1 = 1; return;
	case 0x3E: gsu.F.b.a2 = 1; return;
	case 0x3F: gsu.F.b.a1 = gsu.F.b.a2 = 1; return;
	default:
		std::println("GSU Unimpl opc = ${:X}", opc);
		exit(1);
	}
	gsu.F.b.B = gsu.F.b.a1 = gsu.F.b.a2 = 0;
	gsu.to_reg = gsu.from_reg = 0;
}

void snes::gsu_run()
{
	if( gsu.F.b.GO )
	{
		while( gsu.F.b.GO && gsu.stamp < master_stamp )
		{
			u8 opc = gsu.fetch;
			//std::println("GSU: opc ${:X}, next fetch ${:X}:${:X}", opc, gsu.pb, gsu.r[15]);
			gsu.fetch = gread(gsu.pb, gsu.r[15]);
			//std::println("fetch got ${:X}\n----", gsu.fetch);
			gsu_exec(opc);
			gsu.r[15] += 1;
			gsu.stamp += 1;
		}
	} else {
		gsu.stamp = master_stamp;
	}
}

u16 snes::gsu_add(u16 a, u16 b, u16 c)
{
	u32 res = a;
	res += b;
	res += c;
	
	gsu.F.b.C = (res>>16)&1;
	gsu.F.b.Z = (((res&0xffff)==0)? 1:0);
	gsu.F.b.V = ((res^a)&(res^b))>>15;
	gsu.F.b.S = (res>>15)&1;
	return res;
}

u8 snes::gsu_read(u8 bank, u16 addr)
{
	if( bank == 0x70 || bank == 0x71 )
	{
		gsu.last_rdaddr = (bank << 16)|addr;
		u8 v = extram[(((bank&1)<<16)|addr) % cart.ext_size];
		return v;
	}

	if( bank < 0x40 )
	{
		return ROM[(((bank&0x3F)*32_KB) | (addr&0x7fff)) % ROM.size()];
	}
	
	if( bank >= 0x40 && bank < 0x60 )
	{
		return ROM[(((bank-0x40)*64_KB) | addr) % ROM.size()];
	}
	
	std::println("GSU: Unimpl read ${:X}:${:X}", bank, addr);
	return 0;
}

void snes::gsu_write(u16 addr, u8 v)
{
	if( gsu.ramb != 0x70 && gsu.ramb != 0x71 )
	{
		std::println("GSU: Unimpl write ${:X}:${:X}", gsu.ramb, addr);
		return;
	}
	extram[(((gsu.ramb&1)<<16)|addr) % cart.ext_size] = v;
}

u16 snes::gsu_setSZ(u16 v)
{
	gsu.F.b.Z = ((v==0) ? 1:0);
	gsu.F.b.S = ((v>>15)&1);
	return v;
}

void snes::gsu_plot()
{
	u32 base = gsu.scb<<10;
	
	u32 Y = gsu.r[2];
	u32 X = gsu.r[1]++;
	
	//std::println("PLOT ({}, {})", X, Y);
	
	u32 htmode = ((gsu.scm>>2)&1)|((gsu.scm>>4)&2);
	u32 cn = 0;
	switch( htmode )
	{
	case 0: // 128
		cn = (X/8)*0x10 + (Y/8);
		break;
	case 1: // 164
		cn = (X/8)*0x14 + (Y/8);
		break;
	case 2: // 192
		cn = (X/8)*0x18 + (Y/8);
		break;
	case 3: // obj mode
		cn = ((Y>>7)&1)*0x200 + ((X>>7)&1)*0x100 + ((Y&0x7F)>>3)*0x10 + ((Y&0x7F)>>3);
		break;
	}

	u32 mode = 0;
	switch( gsu.scm&3 ) 
	{
	case 0:
		mode = 2;
		base += (cn<<4) + (Y&7)*2;
		break;
	case 1:
		mode = 4;
		base += (cn<<5) + (Y&7)*2;
		break;
	case 2:
	case 3:
		mode = 8;
		base += (cn<<6) + (Y&7)*2;
		break;
	}

	u8 b = (X&7)^7;
	extram[base] &= ~BIT(b);
	extram[base] |= BIT(b)*(gsu.color&1);
	extram[base+1] &= ~BIT(b);
	extram[base+1] |= BIT(b)*((gsu.color>>1)&1);	

	if( mode >= 4 )
	{
		extram[base+16] &= ~BIT(b);
		extram[base+16] |= BIT(b)*((gsu.color>>2)&1);	
		extram[base+17] &= ~BIT(b);
		extram[base+17] |= BIT(b)*((gsu.color>>3)&1);	
	}
	if( mode == 8 )
	{
		extram[base+32] &= ~BIT(b);
		extram[base+32] |= BIT(b)*((gsu.color>>4)&1);	
		extram[base+33] &= ~BIT(b);
		extram[base+33] |= BIT(b)*((gsu.color>>5)&1);	
		extram[base+48] &= ~BIT(b);
		extram[base+48] |= BIT(b)*((gsu.color>>6)&1);	
		extram[base+49] &= ~BIT(b);
		extram[base+49] |= BIT(b)*((gsu.color>>7)&1);	
	}
}























