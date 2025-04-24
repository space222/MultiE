#include <print>
#include "rca1802.h"
#define PC R[P]

u64 rca1802::step()
{
	u8 temp = 0;
	u8 opc = read(PC);
	PC += 1;
	switch( opc )
	{
	case 0: waiting = true; return 2; //TODO: this is wait for irq, not nop
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	case 0x08:
	case 0x09:
	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F: D = read(R[opc&15]); return 2;
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
	case 0x1F: ++R[opc&15]; return 2;
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
	case 0x2F: --R[opc&15]; return 2;	
	case 0x30: temp = read(PC); PC = (PC&0xff00)|temp; return 2;
	case 0x31: temp = read(PC); if( Q ) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x32: temp = read(PC); if(!D ) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x33: temp = read(PC); if(DF ) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x34: temp = read(PC); if(EF[0]) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x35: temp = read(PC); if(EF[1]) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x36: temp = read(PC); if(EF[2]) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x37: temp = read(PC); if(EF[3]) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x38: PC += 1; return 2; // SKP ??
	case 0x39: temp = read(PC); if(!Q ) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x3A: temp = read(PC); if( D ) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x3B: temp = read(PC); if(!DF) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x3C: temp = read(PC); if(!EF[0]) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x3D: temp = read(PC); if(!EF[1]) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x3E: temp = read(PC); if(!EF[2]) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
	case 0x3F: temp = read(PC); if(!EF[3]) { PC = (PC&0xff00)|temp; } else { PC+=1; } return 2;
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
	case 0x4F: D = read(R[opc&15]); ++R[opc&15]; return 2;
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
	case 0x5F: write(R[opc&15], D); return 2;
	case 0x60: ++R[X&15]; return 2;
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x64:
	case 0x65:
	case 0x66:
	case 0x67: out(read(R[X&15]), opc&7); R[X&15]+=1; return 2;
	//case 0x68: undef
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6E:
	case 0x6F: write(R[X&15], D = in(opc&7)); return 2;
	case 0x70: temp = X&15; X = read(R[temp]); P = X&15; X>>=4; R[temp]+=1; IE = 1; return 2; //2?? 
	case 0x71: temp = X&15; X = read(R[temp]); P = X&15; X>>=4; R[temp]+=1; IE = 0; return 2;
	case 0x72: D = read(R[X&15]); ++R[X&15]; return 2;
	case 0x73: write(R[X&15], D); --R[X&15]; return 2;
	case 0x74:{ // ADC
		u16 res = D;
		res += read(R[X&15]);
		res += DF;
		D = res;
		DF = ((res&0x100)?1:0);	
		}return 2;
	case 0x75:{ // SDB
		u16 res = read(R[X&15]);
		res += 0xff ^ D;
		res += DF;
		D = res;
		DF = ((res&0x100)?1:0);	
		}return 2;
	case 0x76: temp = DF; DF = D&1; D>>=1; D |= (temp?0x80:0); return 2;
	case 0x77:{
		u16 res = D;
		res += 0xff ^ read(R[X&15]);
		res += DF;
		D = res;
		DF = ((res&0x100)?1:0);	
		}return 2;
	case 0x78: write(R[X&15], T); return 2;
	case 0x79: T = (X<<4)|P; write(R[2], T); X = P; R[2]-=1; return 2;		
	case 0x7A: Q = 0; return 2;
	case 0x7B: Q = 1; return 2;	
	case 0x7C:{ // ADC imm
		u16 res = D;
		res += read(PC);
		PC += 1;
		res += DF;
		D = res;
		DF = ((res&0x100)?1:0);	
		}return 2;
	case 0x7D:{ // SDB
		u16 res = read(PC);
		PC += 1;
		res += 0xff ^ D;
		res += DF;
		D = res;
		DF = ((res&0x100)?1:0);	
		}return 2;	
	case 0x7E: temp = DF; DF = D>>7; D<<=1; D |= (temp?1:0); return 2;
	case 0x7F:{
		u16 res = D;
		res += 0xff ^ read(PC);
		PC += 1;
		res += DF;
		D = res;
		DF = ((res&0x100)?1:0);	
		}return 2;	
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
	case 0x8F: D = R[opc&15]; return 2;
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
	case 0x9F: D = R[opc&15]>>8; return 2;
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
	case 0xAF: R[opc&15] &= 0xff00; R[opc&15] |= D; return 2;	
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
	case 0xBF: R[opc&15] &= 0xff; R[opc&15] |= D<<8; return 2;
	case 0xC0:{
		u16 t = read(PC)<<8;
		t |= read(PC+1);
		PC = t;	
		} return 3; // first 3 cycle instr?
	case 0xC1:
		if( Q )
		{
			u16 t = read(PC)<<8;
			t |= read(PC);
			PC = t;
		} else {
			PC += 2;
		}
		return 3; // ??
	case 0xC2:
		if( D == 0 )
		{
			u16 t = read(PC)<<8;
			t |= read(PC);
			PC = t;
		} else {
			PC += 2;
		}
		return 3; // ??
	case 0xC3:
		if( DF )
		{
			u16 t = read(PC)<<8;
			t |= read(PC);
			PC = t;
		} else {
			PC += 2;
		}
		return 3; // ??
	case 0xC4: return 3; // official nop, all 0xCN instructions use 3 cycles
	case 0xC5: if( Q == 0 ) { PC += 2; } return 3;
	case 0xC6: if( D != 0 ) { PC += 2; } return 3;
	case 0xC7: if( DF== 0 ) { PC += 2; } return 3;

		
	case 0xC8: PC += 2; return 3;
	case 0xC9:
		if( Q == 0 )
		{
			u16 t = read(PC)<<8;
			t |= read(PC);
			PC = t;
		} else {
			PC += 2;
		}
		return 3; // ??
	case 0xCA:
		if( D != 0 )
		{
			u16 t = read(PC)<<8;
			t |= read(PC);
			PC = t;
		} else {
			PC += 2;
		}
		return 3; // ??	
	case 0xCB:
		if( DF == 0 )
		{
			u16 t = read(PC)<<8;
			t |= read(PC);
			PC = t;
		} else {
			PC += 2;
		}
		return 3; // ??
	case 0xCC: if( IE ) { PC += 2; } return 3;
	case 0xCD: if( Q ) { PC += 2; } return 3;
	case 0xCE: if( D == 0 ) { PC += 2; } return 3;
	case 0xCF: if( DF ) { PC += 2; } return 3;
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
	case 0xDF: P = opc&15; return 2;
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
	case 0xEF: X = opc&15; return 2;

	case 0xF1: D |= read(R[X&15]); return 2;
	case 0xF2: D &= read(R[X&15]); return 2;
	case 0xF3: D ^= read(R[X&15]); return 2;
	case 0xF4:{
		u16 res = D;
		res += read(R[X&15]);
		D = res;
		DF = res>>8;	
		}return 2;
	case 0xF5:{
		u16 res = read(R[X&15]);
		res += 0xff^D;
		res += 1;
		D = res;
		DF = res>>8;	
		}return 2;
	case 0xF6: DF = D&1; D>>=1; return 2;
	case 0xF7:{
		u16 res = D;
		res += 0xff^read(R[X&15]);
		res += 1;
		D = res;
		DF = res>>8;	
		}return 2;	
	case 0xF8: D = read(PC); PC += 1; return 2;
	case 0xF9: D |= read(PC); PC += 1; return 2;
	case 0xFA: D &= read(PC); PC += 1; return 2;
	case 0xFB: D ^= read(PC); PC += 1; return 2;
	case 0xFC:{
		u16 res = D;
		res += read(PC);
		PC += 1;
		D = res;
		DF = res>>8;	
		}return 2;
	case 0xFD:{
		u16 res = D;
		res += 0xff^read(PC);
		PC += 1;
		res += 1;
		D = res;
		DF = res>>8;	
		}return 2;	
	case 0xFE: DF = D>>7; D<<=1; return 2;
	case 0xFF:{
		u16 res = read(PC);
		res += 0xff^D;
		PC += 1;
		res += 1;
		D = res;
		DF = res>>8;	
		}return 2;
	}
	return 0;
}

void rca1802::reset()
{
	waiting = false;
	X = P = Q = R[0] = 0; // cpu starts with R0 as PC, with value=zero
	IE = 1;
}

