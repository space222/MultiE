#include <cstdlib>
#include <cstdio>
#include <utility>
#include "f8.h"

u64 f8::step()
{
	u8 temp = 0;
	u8 opc = read(pc0);
	//printf("F8:$%X: opc $%X\n", pc0, opc);
	pc0 += 1;
	
	switch( opc )
	{
	case 0:
	case 1:
	case 2:
	case 3:
		A = r[12 + (opc&3)];
		return 4;
	case 4:
	case 5:
	case 6:
	case 7:
		r[12 + (opc&3)] = A;
		return 4;
	case 8:
		r[12] = pc1>>8;
		r[13] = pc1;
		return 16;
	case 9:
		pc1 = r[12]<<8;
		pc1 |= r[13];
		return 16;
	case 0x0A:
		A = isar;
		return 4;
	case 0x0B:
		isar = A & 0x3F;
		return 4;		
	case 0x0C:
		pc1 = pc0;
		pc0 = r[12]<<8;
		pc0 |= r[13];	
		return 16;	
	case 0x0D:
		pc0 = r[14]<<8;
		pc0 |= r[15];	
		return 16;
	case 0xE:
		r[14] = dc0>>8;
		r[15] = dc0;
		return 16;
	case 0xF:
		dc0 = r[14]<<8;
		dc0 |= r[15];
		return 16;		
	case 0x10:
		dc0 = r[10]<<8;
		dc0 |= r[11];	
		return 16;
	case 0x11:
		r[10] = dc0>>8;
		r[11] = dc0;
		return 16;		
	case 0x12:
		A >>= 1;
		F.b.Z = (A==0 ? 1:0);
		F.b.C = F.b.O = 0;
		F.b.S = 1;	
		return 4;
	case 0x13:
		F.b.S = (A>>7)^1;
		A <<= 1;
		F.b.Z = (A==0 ? 1:0);
		F.b.C = F.b.O = 0;
		return 4;
	case 0x14:
		A >>= 4;
		F.b.Z = (A==0 ? 1:0);
		F.b.C = F.b.O = 0;
		F.b.S = 1;	
		return 4;
	case 0x15:
		F.b.S = (A&BIT(3))?0:1;
		A <<= 4;
		F.b.Z = (A==0 ? 1:0);
		F.b.C = F.b.O = 0;
		return 4;
	case 0x16:
		A = read(dc0);
		dc0 += 1;	
		return 10;
	case 0x17:
		write(dc0, A);
		dc0 += 1;	
		return 10;
	case 0x18:
		A ^= 0xff;
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.C = F.b.O = 0;
		return 4;
	case 0x19: // add carry
		if( F.b.C )
		{
			F.b.O = ((A==0x7f)?1:0);
			F.b.C = ((A==0xff)?1:0);
			A += 1;
		} else {
			F.b.O = F.b.C = 0;
		}
		F.b.Z = ((A==0)?1:0);
		F.b.S = ((A&0x80)?0:1);	
		return 4;
	case 0x1A: // DI
		F.b.I = 0;
		return 8;
	case 0x1B: // EI
		F.b.I = 1;
		return 8;
	case 0x1C: // pop (ret)
		pc0 = pc1;	
		return 8;
	case 0x1D: // W = r9
		F.v = r[9];
		F.b.pad = 0;
		return 8;
	case 0x1E: // r9 = W
		F.b.pad = 0;
		r[9] = F.v;
		return 8;
	case 0x1F: // inc A
		F.b.O = (A==0x7f ? 1 : 0);
		F.b.C = (A==0xff ? 1 : 0);
		A += 1;
		F.b.Z = (A==0 ? 1 : 0);
		F.b.S = ((A&0x80) ? 0 : 1);	
		return 4;		
	case 0x20:
		A = read(pc0);
		pc0 += 1;
		return 10;
	case 0x21:
		A &= read(pc0);
		pc0 += 1;
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.O = F.b.C = 0;
		return 10;
	case 0x22:
		A |= read(pc0);
		pc0 += 1;
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.O = F.b.C = 0;
		return 10;
	case 0x23:
		A ^= read(pc0);
		pc0 += 1;
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.O = F.b.C = 0;
		return 10;
	case 0x24:{ //add A, #imm	
		temp = read(pc0); pc0 += 1;
		u16 res = A;
		res += temp;
		F.b.C = res>>8;
		F.b.Z = ((res&0xff)==0 ? 1 : 0);
		F.b.S = ((res&0x80) ? 0 : 1);
		F.b.O = (((res^temp) & (res^A) & 0x80) ? 1 : 0);
		A = res;
		}return 10;	
	case 0x25:{ // cmp #imm
		temp = read(pc0); pc0 += 1;
		u16 res = A^0xff;
		res += temp;
		res += 1;
		F.b.C = res>>8;
		F.b.Z = ((res&0xff)==0 ? 1 : 0);
		F.b.S = ((res&0x80) ? 0 : 1);
		F.b.O = (((res^temp) & (res^(A^0xff)) & 0x80) ? 1 : 0);	
		}return 10;		
	case 0x26: // in
		A = in(read(pc0));
		pc0 += 1;	
		return 16;
	case 0x27: // out 
		out(read(pc0), A);
		pc0 += 1;
		return 16;
	case 0x28: // pi (call)
		A = read(pc0);
		temp = read(pc0+1);
		pc1 = pc0 + 2;
		pc0 = A<<8;
		pc0 |= temp;	
		return 26;
	case 0x29: // jmp
		A = read(pc0);
		pc0 += 1;
		temp = read(pc0);
		pc0 = A<<8;
		pc0 |= temp;	
		return 22;
	case 0x2A:
		dc0 = read(pc0)<<8;
		pc0 += 1;
		dc0 |= read(pc0);
		pc0 += 1;	
		return 24;
	case 0x2B: return 4; // NOP
	case 0x2C:
		std::swap(dc0, dc1);
		return 8;
	
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
	case 0x3B:
	case 0x3C:
	case 0x3D:
	case 0x3E:
	case 0x3F:
		temp = scratch(opc&15);
		F.b.O = (r[temp]==0x80 ? 1 : 0);
		F.b.C = (r[temp]==0 ? 1 : 0);
		r[temp] -= 1;
		F.b.Z = (r[temp]==0 ? 1 : 0);
		F.b.S = ((r[temp]&0x80) ? 0 : 1);
		return 6;
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
	case 0x4C:
	case 0x4D:
	case 0x4E:
	case 0x4F:
		A = r[scratch(opc&15)];
		return 4;
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
		r[scratch(opc&15)] = A;
		return 4;
	
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x64:
	case 0x65:
	case 0x66:
	case 0x67:
		isar &= ~070;
		isar |= (opc & 7)<<3;
		return 4;
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6E:
	case 0x6F:
		isar &= ~7;
		isar |= (opc & 7);
		return 4;
		
	case 0x70:
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
		A = opc&0xf;
		return 4;		
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x86:
	case 0x87:
		pc0 += (( F.v & (opc&7) ) ? ((s8)read(pc0)) : 1);
		return 12 + (( F.v & (opc&7) ) ? 14 : 0);
	case 0x88:{ //add A, [dc0+]	
		temp = read(dc0); dc0 += 1;
		u16 res = A;
		res += temp;
		F.b.C = res>>8;
		F.b.Z = ((res&0xff)==0 ? 1 : 0);
		F.b.S = ((res&0x80) ? 0 : 1);
		F.b.O = (((res^temp) & (res^A) & 0x80) ? 1 : 0);
		A = res;
		}return 4;
	case 0x89:{ //add A, [dc0+] (BCD)
		temp = read(dc0); dc0 += 1;
		u16 normsum = A + temp;
		u8 L = normsum & 0xf;
		u8 H = normsum >> 4;
		u8 ic = (A&0xf) + (temp&0xf);
		if( !(ic&BIT(4)) )  L = (L&15) + 10;
		if( !(normsum & BIT(8)) ) H = (H&15) + 10;
		F.b.C = H>>4;
		//printf("F8:$%X:add-bcd: $%X + $%X = ", pc0-1, u8(A), u8(temp));
		u8 res = ((H&15)<<4)|(L&15);
		//printf("$%X\n", res);
		F.b.Z = (res==0 ? 1 : 0);
		F.b.S = ((res&0x80)? 0 : 1);	
		F.b.O = (((res^temp) & (res^A) & 0x80) ? 1 : 0);
		A = res;
		}return 8;
	case 0x8A:
		A &= read(dc0);
		dc0 += 1;
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.O = F.b.C = 0;
		return 10;
	case 0x8B:
		A |= read(dc0);
		dc0 += 1;
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.O = F.b.C = 0;
		return 10;
	case 0x8C:
		A ^= read(dc0);
		dc0 += 1;
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.O = F.b.C = 0;
		return 10;
	case 0x8D:{ // cmp [dc0+]
		temp = read(dc0); dc0 += 1;
		u16 res = A^0xff;
		res += temp;
		res += 1;
		F.b.C = res>>8;
		F.b.Z = ((res&0xff)==0 ? 1 : 0);
		F.b.S = ((res&0x80) ? 0 : 1);
		F.b.O = (((res^temp) & (res^(A^0xff)) & 0x80) ? 1 : 0);	
		}return 10;
	case 0x8E:
		dc0 += s8(A);
		return 10;
	case 0x8F:
		pc0 += (((isar&7)!=7) ? s8(read(pc0)) : 1);
		return ((isar&7)!=7) ? 10 : 8;
	case 0x90:
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x96:
	case 0x97:
	case 0x98:
	case 0x99:
	case 0x9A:
	case 0x9B:
	case 0x9C:
	case 0x9D:
	case 0x9E:
	case 0x9F:
		pc0 += ( (F.v&(opc&15))==0 ? ((s8)read(pc0)) : 1);
		return 14 +( (F.v&(opc&15))==0 ? 12 : 0);  // BF have reverse cycles taken/not taken from BT?	
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
	case 0xAF:
		temp = opc&15;
		A = in(temp);
		return (temp<2)?8:16;	
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
		temp = opc&15;
		out(temp, A);
		return (temp<2)?8:16;
	case 0xC0:
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
	case 0xCF:{ //add A, r	
		temp = r[scratch(opc&15)];
		u16 res = A;
		res += temp;
		F.b.C = res>>8;
		F.b.Z = ((res&0xff)==0 ? 1 : 0);
		F.b.S = ((res&0x80) ? 0 : 1);
		F.b.O = (((res^temp) & (res^A) & 0x80) ? 1 : 0);
		A = res;
		}return 4;
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
	case 0xDE:
	case 0xDF:{ //add A, r (BCD)
		temp = r[scratch(opc&15)];
		u16 normsum = A + temp;
		u8 L = normsum & 0xf;
		u8 H = normsum >> 4;
		u8 ic = (A&0xf) + (temp&0xf);
		if( !(ic&BIT(4)) )  L = (L&15) + 10;
		if( !(normsum & BIT(8)) ) H = (H&15) + 10;
		F.b.C = H>>4;
		//printf("F8:$%X:add-bcd: $%X + $%X = ", pc0-1, u8(A), u8(temp));
		u8 res = ((H&15)<<4)|(L&15);
		//printf("$%X\n", res);
		F.b.Z = (res==0 ? 1 : 0);
		F.b.S = ((res&0x80)? 0 : 1);	
		F.b.O = (((res^temp) & (res^A) & 0x80) ? 1 : 0);
		A = res;
		}return 8;		
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
	case 0xEE:
	case 0xEF:
		A ^= r[scratch(opc&7)];
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.O = F.b.C = 0;
		return 4;
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
	case 0xFF:
		A &= r[scratch(opc&7)];
		F.b.Z = (A==0?1:0);
		F.b.S = ((A&0x80)?0:1);
		F.b.O = F.b.C = 0;
		return 4;	
	default:
		printf("F8:$%X: Unimpl. opcode = $%X\n", pc0-1, opc);
		exit(1);
	}

	return 4;
}

void f8::reset()
{
	pc1 = pc0 = 0;
}

u8 f8::scratch(u8 p)
{
	if( p == 12 ) return isar;
	if( p == 13 )
	{
		u8 v = isar;
		isar = (isar&070)|((isar+1)&7);
		return v;	
	}
	if( p == 14 )
	{
		u8 v = isar;
		isar = (isar&070)|((isar-1)&7);
		return v;	
	}
	return p;
}


