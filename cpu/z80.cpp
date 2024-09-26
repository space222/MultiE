#include <cstdio>
#include <cstdlib>
#include <utility>
#include "z80.h"

#define B r[0]
#define C r[1]
#define D r[2]
#define E r[3]
#define H r[4]
#define L r[5]
#define F fl
#define A r[7]
#define IXL r[8]
#define IXH r[9]
#define IYL r[10]
#define IYH r[11]

#define BC ((B<<8)|C)
#define DE ((D<<8)|E)
#define HL ((H<<8)|L)
#define AF ((A<<8)|F.v)
#define IX ((IXH<<8)|IXL)
#define IY ((IYH<<8)|IYL)


int cyctable[256] = {  4,  10, 7, 6, 4, 4, 7, 4, 4, 11, 7, 6, 4, 4, 7, 4,
		       8,  10, 7, 6, 4, 4, 7, 4, 12, 11, 7, 6, 4, 4, 7, 4,
		       7,  10, 16, 6, 4,4, 7, 4, 7, 11,  16, 6,4, 4, 7, 4,
		       7,  10, 13, 6, 11, 11,10, 4, 7, 11, 13, 6, 4, 4, 7,
		       4,4, 4,4,4,4,4,7,4, 4, 4,4,4,4,4,7,4,4,4,4,4,4,4,7,
		       4,4,4,4,4,4,4,7,4,4,4,4,4,4,4,7,4,4,4,4,4,4,4,7,4,
		       7,7, 7, 7, 7, 7, 4,7,4,4,4,4,4,4,7,4,4,4,4,4,4,4,
			7,4, 4,4,4,4,4,4,7,4,4,4,4,4,4,4,7,4,4,4,4,4,4,
			4, 7, 4,4,4,4,4,4,4,7,4,4,4,4,4,4,4,7,4,
			4,4,4,4,4,4,7,4,4,4,4,4, 4, 4, 7, 4,
		       5, 10, 10,10,10,11,7,11,5,10,10,0,10,17,7,11,
			5, 10,10,11,10,11, 7,11,5,4,10,11,10,0,7, 11,
			5, 10,10,19,10,11, 7,11,5,4,10,4, 10,0,7, 11,
		 	5,  10,10,4, 10, 11, 7, 11,5,6,10,4,10,0,7,11
		    };


u64 z80::step()
{
	u8 temp = 0;
	prefix = icycles = 0;
	R = (R&0x80)|((R+1)&0x7f);
	
	if( nmi_line )
	{
		push(pc);
		pc = 0x66;
		nmi_line = 0;
		iff1 = 0;
		halted = false;
	} else if( iff1 && irq_line && !intr_blocked ) {
		push(pc);
		pc = 0x38;
		//irq_line = 0;
		iff1 = 0;
		halted = false;
	}
	intr_blocked = false;
	
	if( halted ) return 4;
	
	u8 opc = read(pc++);
	
	while( opc == 0xFD || opc == 0xDD )
	{
		prefix = opc;
		opc = read(pc++);
		icycles += 4;
	}
	
	switch( opc )
	{
	case 0: break; //nop
	case 1: setBC(read16(pc)); pc += 2; break;
	case 2: write(BC, A); break;
	case 3: setBC(BC+1); break;
	case 4: B = inc(B); break;
	case 5: B = dec(B); break;
	case 6: B = read(pc++); break;
	case 7: rcla(); break;
	case 8: std::swap(r[7], r[7+12]); std::swap(F.v, r[6+12]); break;
	case 9: setHL(add16(getHL(), BC)); break;
	case 0x0A: A = read(BC); break;
	case 0x0B: setBC(BC-1); break;
	case 0x0C: C = inc(C); break;
	case 0x0D: C = dec(C); break;
	case 0x0E: C = read(pc++); break;
	case 0x0F: rrca(); break;
	
	case 0x10: temp = read(pc++); B--; if( B ) { pc += (s8)temp; icycles += 5; } break;
	case 0x11: setDE(read16(pc)); pc += 2; break;
	case 0x12: write(DE, A); break;
	case 0x13: setDE(DE+1); break;
	case 0x14: D = inc(D); break;
	case 0x15: D = dec(D); break;
	case 0x16: D = read(pc++); break;
	case 0x17: rla(); break;
	case 0x18: temp = read(pc++); pc += (s8)temp; break;
	case 0x19: setHL(add16(getHL(), DE)); break;
	case 0x1A: A = read(DE); break;
	case 0x1B: setDE(DE-1); break;
	case 0x1C: E = inc(E); break;
	case 0x1D: E = dec(E); break;
	case 0x1E: E = read(pc++); break;
	case 0x1F: rra(); break;
	
	case 0x20: temp = read(pc++); if( F.b.Z == 0 ) { pc += (s8)temp; icycles += 5; } break;
	case 0x21: setHL(read16(pc)); pc+=2; break;
	case 0x22: write16(read16(pc), getHL()); pc+=2; break;
	case 0x23: setHL(getHL()+1); break;
	case 0x24: if( prefix == 0xFD ) IYH = inc(IYH);
		   else if( prefix == 0xDD ) IXH = inc(IXH);
		   else H = inc(H);
		   break;
	case 0x25: if( prefix == 0xFD ) IYH = dec(IYH);
		   else if( prefix == 0xDD ) IXH = dec(IXH);
		   else H = dec(H);
		   break;
	case 0x26: if( prefix == 0xFD ) IYH = read(pc++);
		   else if( prefix == 0xDD ) IXH = read(pc++);
		   else H = read(pc++);
		   break;
	case 0x27: daa(); break;
	case 0x28: temp = read(pc++); if( F.b.Z ) { pc += (s8)temp; icycles += 5; } break;
	case 0x29: setHL(add16(getHL(), getHL())); break;
	case 0x2A: setHL(read16(read16(pc))); pc += 2; break;
	case 0x2B: setHL(getHL()-1); break;
	case 0x2C: if( prefix == 0xFD ) IYL = inc(IYL);
		   else if( prefix == 0xDD ) IXL = inc(IXL);
		   else L = inc(L);
		   break;
	case 0x2D: if( prefix == 0xFD ) IYL = dec(IYL);
		   else if( prefix == 0xDD ) IXL = dec(IXL);
		   else L = dec(L);
		   break;
	case 0x2E: if( prefix == 0xFD ) IYL = read(pc++);
		   else if( prefix == 0xDD ) IXL = read(pc++);
		   else L = read(pc++);
		   break;
	case 0x2F: A = ~A; F.b.N = F.b.fH = 1; set35(A); break;
	
	case 0x30: temp = read(pc++); if( F.b.fC == 0 ) { pc += (s8)temp; icycles += 5; } break;
	case 0x31: sp = read16(pc); pc += 2; break;
	case 0x32: write(read16(pc), A); pc += 2; break;
	case 0x33: sp++; break;
	case 0x34: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	u8 v = read(IY+(s8)temp);
		   	write(IY+(s8)temp, inc(v));
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	u8 v = read(IX+(s8)temp);
		   	write(IX+(s8)temp, inc(v));		   
		   } else {
		   	write(HL, inc(read(HL)));
		   }
		   break;
	case 0x35: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	u8 v = read(IY+(s8)temp);
		   	write(IY+(s8)temp, dec(v));
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	u8 v = read(IX+(s8)temp);
		   	write(IX+(s8)temp, dec(v));		   
		   } else {
		   	write(HL, dec(read(HL)));
		   }
		   break;
	case 0x36: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	write(IY+(s8)temp, read(pc++));
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	write(IX+(s8)temp, read(pc++));
		   } else {
		   	write(HL, read(pc++));
		   }
		   break;
	case 0x37: F.b.fC = 1; F.b.N = F.b.fH = 0; set35(F.v|A); break;
	case 0x38: temp = read(pc++); if( F.b.fC ) { pc += (s8)temp; icycles += 5; } break;
	case 0x39: setHL(add16(getHL(), sp)); break;
	case 0x3A: A = read(read16(pc)); pc += 2; break;
	case 0x3B: sp--; break;
	case 0x3C: A = inc(A); break;
	case 0x3D: A = dec(A); break;
	case 0x3E: A = read(pc++); break;
	case 0x3F: F.b.fH = F.b.fC; F.b.fC = F.b.fC^1; F.b.N = 0; set35(F.v|A); break;
	
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
	case 0x45:
	case 0x47: B = getreg(opc&7); break;	
	case 0x46: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	B = read(IY+(s8)temp);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	B = read(IX+(s8)temp);
		   } else B = read(HL);
		   break;
	case 0x48:
	case 0x49:
	case 0x4A:
	case 0x4B:
	case 0x4C:
	case 0x4D:
	case 0x4F: C = getreg(opc&7); break;	
	case 0x4E: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	C = read(IY+(s8)temp);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	C = read(IX+(s8)temp);
		   } else C = read(HL);
		   break;

	case 0x50:
	case 0x51:
	case 0x52:
	case 0x53:
	case 0x54:
	case 0x55:
	case 0x57: D = getreg(opc&7); break;	
	case 0x56: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	D = read(IY+(s8)temp);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	D = read(IX+(s8)temp);
		   } else D = read(HL);
		   break;
	case 0x58:
	case 0x59:
	case 0x5A:
	case 0x5B:
	case 0x5C:
	case 0x5D:
	case 0x5F: E = getreg(opc&7); break;	
	case 0x5E: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	E = read(IY+(s8)temp);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	E = read(IX+(s8)temp);
		   } else E = read(HL);
		   break;
		   
	case 0x60:
	case 0x61:
	case 0x62:
	case 0x63:
	case 0x64:
	case 0x65:
	case 0x67: if( prefix == 0xFD ) IYH = r[opc&7];
		   else if( prefix == 0xDD ) IXH = r[opc&7];
		   else H = getreg(opc&7);
		   break;
	case 0x66: if( prefix == 0xFD )
		   { 
		   	s8 d = read(pc++);
		   	H = read(IY+d);
		   } else if( prefix == 0xDD ) {
		   	s8 d = read(pc++);
		   	H = read(IX+d);
		   } else H = read(HL);
		   break;
	case 0x68:
	case 0x69:
	case 0x6A:
	case 0x6B:
	case 0x6C:
	case 0x6D:
	case 0x6F: if( prefix == 0xFD ) IYL = r[opc&7];
		   else if( prefix == 0xDD ) IXL = r[opc&7];
		   else L = getreg(opc&7);
		   break;
	case 0x6E: if( prefix == 0xFD )
		   { 
		   	s8 d = read(pc++);
		   	L = read(IY+d);
		   } else if( prefix == 0xDD ) {
		   	s8 d = read(pc++);
		   	L = read(IX+d);
		   } else L = read(HL);
		   break;
		   
	case 0x76: halted = true; break;
	
	case 0x70:
	case 0x71:
	case 0x72:
	case 0x73:
	case 0x74:
	case 0x75:
	case 0x77: if( prefix == 0xFD )
		   {
		   	s8 d = read(pc++);
		   	write(IY+d, r[opc&7]);
		   } else if( prefix == 0xDD ) {
		   	s8 d = read(pc++);
		   	write(IX+d, r[opc&7]);
		   } else {
		   	write(HL, r[opc&7]);
		   }
		   break;
		      
	case 0x78:
	case 0x79:
	case 0x7A:
	case 0x7B:
	case 0x7C:
	case 0x7D:
	case 0x7F: A = getreg(opc&7); break;	
	case 0x7E: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	A = read(IY+(s8)temp);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	A = read(IX+(s8)temp);
		   } else A = read(HL);
		   break;
		   
	case 0x80:
	case 0x81:
	case 0x82:
	case 0x83:
	case 0x84:
	case 0x85:
	case 0x87: A = adc(A, getreg(opc&7), 0); set35(A); break;	
	case 0x86: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	A = adc(A, read(IY+(s8)temp), 0);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	A = adc(A, read(IX+(s8)temp), 0);
		   } else A = adc(A, read(HL), 0); 
		   set35(A);
		   break;
	case 0x88:
	case 0x89:
	case 0x8A:
	case 0x8B:
	case 0x8C:
	case 0x8D:
	case 0x8F: A = adc(A, getreg(opc&7), F.b.fC); set35(A); break;	
	case 0x8E: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	A = adc(A, read(IY+(s8)temp), F.b.fC);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	A = adc(A, read(IX+(s8)temp), F.b.fC);
		   } else A = adc(A, read(HL), F.b.fC); 
		   set35(A);
		   break;
	case 0x90:
	case 0x91:
	case 0x92:
	case 0x93:
	case 0x94:
	case 0x95:
	case 0x97: A = adc(A, getreg(opc&7)^0xff, 1); F.b.fC^=1; F.b.fH^=1; set35(A); F.b.N=1; break;	
	case 0x96: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	A = adc(A, read(IY+(s8)temp)^0xff, 1);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	A = adc(A, read(IX+(s8)temp)^0xff, 1);
		   } else A = adc(A, read(HL)^0xff, 1);
		   F.b.fC^=1; F.b.fH^=1; F.b.N=1; 
		   set35(A);
		   break;
	case 0x98:
	case 0x99:
	case 0x9A:
	case 0x9B:
	case 0x9C:
	case 0x9D:
	case 0x9F: A = adc(A, getreg(opc&7)^0xff, F.b.fC^1); F.b.fC^=1; F.b.fH^=1; F.b.N=1; set35(A); break;	
	case 0x9E: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	A = adc(A, read(IY+(s8)temp)^0xff, F.b.fC^1);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	A = adc(A, read(IX+(s8)temp)^0xff, F.b.fC^1);
		   } else A = adc(A, read(HL)^0xff, F.b.fC^1);
		   F.b.fC^=1; F.b.fH^=1; F.b.N=1;
		   set35(A);
		   break;	   

	case 0xA0:
	case 0xA1:
	case 0xA2:
	case 0xA3:
	case 0xA4:
	case 0xA5:
	case 0xA7: A &= getreg(opc&7); set35(A); setSZP(A); F.b.fH = 1; F.b.fC = F.b.N = 0; break;	
	case 0xA6: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	A &= read(IY+(s8)temp);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	A &= read(IX+(s8)temp);
		   } else A &= read(HL);
		   setSZP(A);
		   F.b.fH = 1;
		   F.b.fC = F.b.N = 0;
		   set35(A);
		   break;
	case 0xA8:
	case 0xA9:
	case 0xAA:
	case 0xAB:
	case 0xAC:
	case 0xAD:
	case 0xAF: A ^= getreg(opc&7); setSZP(A); set35(A); F.b.fH = F.b.fC = F.b.N = 0; break;
	case 0xAE: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	A ^= read(IY+(s8)temp);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	A ^= read(IX+(s8)temp);
		   } else A ^= read(HL);
		   setSZP(A);
		   F.b.fH = F.b.fC = F.b.N = 0;
		   set35(A);
		   break;
	
	case 0xB0:
	case 0xB1:
	case 0xB2:
	case 0xB3:
	case 0xB4:
	case 0xB5:
	case 0xB7: A |= getreg(opc&7); setSZP(A); set35(A); F.b.fH = F.b.fC = F.b.N = 0; break;
	case 0xB6: if( prefix == 0xFD )
		   {
		   	temp = read(pc++);
		   	A |= read(IY+(s8)temp);
		   } else if( prefix == 0xDD ) {
		   	temp = read(pc++);
		   	A |= read(IX+(s8)temp);
		   } else A |= read(HL);
		   setSZP(A);
		   F.b.fH = F.b.fC = F.b.N = 0;
		   set35(A);
		   break;
		   
	case 0xB8:
	case 0xB9:
	case 0xBA:
	case 0xBB:
	case 0xBC:
	case 0xBD:
	case 0xBF: temp = getreg(opc&7); adc(A, temp^0xff, 1); F.b.fC^=1; F.b.fH^=1; F.b.N=1; set35(temp); break;	
	case 0xBE: if( prefix == 0xFD )
		   {
		   	s8 d = read(pc++);
		   	temp = read(IY+d);
		   	adc(A, temp^0xff, 1);
		   } else if( prefix == 0xDD ) {
		   	s8 d = read(pc++);
		   	temp = read(IX+d);
		   	adc(A, temp^0xff, 1);
		   } else {
		   	temp = read(HL);
		   	adc(A, temp^0xff, 1);
		   }
		   F.b.fC^=1; F.b.fH^=1; F.b.N=1;
		   set35(temp);
		   break;
		   
	case 0xC0: if( F.b.Z == 0 ) { ret(); icycles += 6; } break;		   
	case 0xD0: if( F.b.fC == 0 ) { ret(); icycles += 6; } break;
	case 0xE0: if( F.b.V == 0 ) { ret(); icycles += 6; } break;
	case 0xF0: if( F.b.S == 0 ) { ret(); icycles += 6; } break;
	case 0xC8: if( F.b.Z == 1 ) { ret(); icycles += 6; } break;		   
	case 0xD8: if( F.b.fC == 1 ) { ret(); icycles += 6; } break;
	case 0xE8: if( F.b.V == 1 ) { ret(); icycles += 6; } break;
	case 0xF8: if( F.b.S == 1 ) { ret(); icycles += 6; } break;
	
	case 0xC1: setBC(pop()); break;
	case 0xD1: setDE(pop()); break;
	case 0xE1: setHL(pop()); break;
	case 0xF1: setAF(pop()); break;
	case 0xC5: push(BC); break;
	case 0xD5: push(DE); break;
	case 0xE5: push(getHL()); break;
	case 0xF5: push(AF); break;
	
	case 0xC2: if( F.b.Z == 0 ) { pc = read16(pc); } else { pc += 2; } break;
	case 0xD2: if( F.b.fC == 0 ) { pc = read16(pc); } else { pc += 2; } break;
	case 0xE2: if( F.b.V == 0 ) { pc = read16(pc); } else { pc += 2; } break;
	case 0xF2: if( F.b.S == 0 ) { pc = read16(pc); } else { pc += 2; } break;
	case 0xCA: if( F.b.Z == 1 ) { pc = read16(pc); } else { pc += 2; } break;
	case 0xDA: if( F.b.fC == 1 ) { pc = read16(pc); } else { pc += 2; } break;
	case 0xEA: if( F.b.V == 1 ) { pc = read16(pc); } else { pc += 2; } break;
	case 0xFA: if( F.b.S == 1 ) { pc = read16(pc); } else { pc += 2; } break;
	
	case 0xC7: call(0x00); break;
	case 0xD7: call(0x10); break;
	case 0xE7: call(0x20); break;
	case 0xF7: call(0x30); break;
	case 0xCF: call(0x08); break;
	case 0xDF: call(0x18); break;
	case 0xEF: call(0x28); break;
	case 0xFF: call(0x38); break;
	
	case 0xC4: if( F.b.Z == 0 ) { u16 a = read16(pc); pc+=2; call(a); icycles += 7; } else { pc += 2; } break;
	case 0xD4: if( F.b.fC == 0 ) { u16 a = read16(pc); pc+=2; call(a); icycles += 7; } else { pc += 2; } break;
	case 0xE4: if( F.b.V == 0 ) { u16 a = read16(pc); pc+=2; call(a); icycles += 7; } else { pc += 2; } break;
	case 0xF4: if( F.b.S == 0 ) { u16 a = read16(pc); pc+=2; call(a); icycles += 7; } else { pc += 2; } break;
	case 0xCC: if( F.b.Z == 1 ) { u16 a = read16(pc); pc+=2; call(a); icycles += 7; } else { pc += 2; } break;
	case 0xDC: if( F.b.fC == 1 ) { u16 a = read16(pc); pc+=2; call(a); icycles += 7; } else { pc += 2; } break;
	case 0xEC: if( F.b.V == 1 ) { u16 a = read16(pc); pc+=2; call(a); icycles += 7; } else { pc += 2; } break;
	case 0xFC: if( F.b.S == 1 ) { u16 a = read16(pc); pc+=2; call(a); icycles += 7; } else { pc += 2; } break;
	case 0xCD: { u16 a = read16(pc); pc+=2; call(a); } break;
	
	case 0xC6: A = adc(A, read(pc++), 0); set35(A); break;
	case 0xD6: A = adc(A, read(pc++)^0xff, 1); F.b.fC^=1; F.b.fH^=1; set35(A); F.b.N=1; break;
	case 0xE6: A &= read(pc++); setSZP(A); set35(A); F.b.fH = 1; F.b.fC = F.b.N = 0; break;
	case 0xF6: A |= read(pc++); setSZP(A); set35(A); F.b.fH = F.b.fC = F.b.N = 0; break;
	case 0xCE: A = adc(A, read(pc++), F.b.fC); set35(A); break;
	case 0xDE: A = adc(A, read(pc++)^0xff, F.b.fC^1); F.b.fC^=1; F.b.fH^=1; set35(A); F.b.N=1; break;
	case 0xEE: A ^= read(pc++); setSZP(A); set35(A); F.b.fH = F.b.fC = F.b.N = 0; break;
	case 0xFE: temp = read(pc++); adc(A, temp^0xff, 1); F.b.fH^=1; F.b.fC^=1; F.b.N=1; set35(temp); break;
	
	case 0xC3: pc = read16(pc); break;
	case 0xD3: out((A<<8)|read(pc++), A); break;
	case 0xE3: {u16 v = getHL();
		   setHL(read16(sp));
		   write16(sp, v);
		   }break;
	case 0xF3: iff1 = iff2 = 0; break;
	
	case 0xC9: ret(); break;
	case 0xD9: std::swap(r[0], r[0+12]); std::swap(r[1], r[1+12]);
		   std::swap(r[2], r[2+12]); std::swap(r[3], r[3+12]);
		   std::swap(r[4], r[4+12]); std::swap(r[5], r[5+12]);
		   break;
	case 0xE9: pc = getHL(); break;
	case 0xF9: sp = getHL(); break;
	
	case 0xDB: A = in((A<<8)|read(pc++)); break;
	case 0xEB: { u16 t = HL; H=D; L=E; setDE(t); } break;
	case 0xFB: iff1 = iff2 = 1; intr_blocked = true; break;
	
	case 0xCB: icycles += step_cb(); break;
	case 0xED: icycles += step_ed(); break;
	}
	
	icycles += cyctable[opc];
	
	return icycles;
}

u64 z80::step_cb()
{
	s8 d = prefix ? read(pc++) : 0;
	u8 opc = read(pc++);
	u8 reg = opc&7;
	u8 oldC = F.b.fC;
	
	if( opc < 0x08 )
	{
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.N = F.b.fH = 0;
		val = (val<<1)|(val>>7);
		F.b.fC = val&1;
		setSZP(val);
		set35(val);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;
	} else if( opc < 0x10 ) {
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.N = F.b.fH = 0;
		val = (val<<7)|(val>>1);
		F.b.fC = val>>7;
		setSZP(val);
		set35(val);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;	
	} else if( opc < 0x18 ) {
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.N = F.b.fH = 0;
		F.b.fC = val>>7;
		val = (val<<1)|oldC;
		setSZP(val);
		set35(val);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;	
	} else if( opc < 0x20 ) {
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.N = F.b.fH = 0;
		F.b.fC = val&1;
		val = (val>>1)|(oldC<<7);
		setSZP(val);
		set35(val);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;	
	} else if( opc < 0x28 ) {
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.N = F.b.fH = 0;
		F.b.fC = val>>7;
		val <<= 1;
		setSZP(val);
		set35(val);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;	
	} else if( opc < 0x30 ) {
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.N = F.b.fH = 0;
		F.b.fC = val&1;
		val = (val&0x80)|(val>>1);
		setSZP(val);
		set35(val);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;	
	} else if( opc < 0x38 ) {
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.N = F.b.fH = 0;
		F.b.fC = val>>7;
		val <<= 1;
		val |= 1;
		setSZP(val);
		set35(val);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;
	} else if( opc < 0x40 ) {
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.N = F.b.fH = 0;
		F.b.fC = val&1;
		val >>= 1;
		setSZP(val);
		set35(val);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;
	}
	
	if( opc < 0x80 )
	{
		const u8 bit = ((opc>>3)&7);
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		F.b.fH = 1;
		F.b.N = 0;
		set35(val);
		val &= 1u<<bit;
		F.b.V = F.b.Z = val==0;
		F.b.S = val>>7;
		return 4;
	}
	
	if( opc < 0xC0 )
	{
		const u8 bit = ((opc>>3)&7);
		u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
		val &= ~(1u<<bit);
		if( prefix == 0xFD )
		{
			write(IY+d, val);
			if( reg != 6 ) r[reg] = val;
		} else if( prefix == 0xDD ) {
			write(IX+d, val);
			if( reg != 6 ) r[reg] = val;
		} else {
			setreg(reg, val);
		}
		return 4;
	}
	
	const u8 bit = ((opc>>3)&7);
	u8 val = (prefix == 0xFD ? read(IY+d) : (prefix == 0xDD ? read(IX+d) : getreg(reg)));
	val |= (1u<<bit);
	if( prefix == 0xFD )
	{
		write(IY+d, val);
		if( reg != 6 ) r[reg] = val;
	} else if( prefix == 0xDD ) {
		write(IX+d, val);
		if( reg != 6 ) r[reg] = val;
	} else {
		setreg(reg, val);
	}
	
	return 4;
}

u64 z80::step_ed()
{
	prefix = 0;
	u8 opc = read(pc++);
	u8 temp = 0;
	u8 oldC = F.b.fC;
	
	switch( opc )
	{
	case 0x46: break;
	case 0x56: break; 
	case 0x5E: break; // im 0/1/2 instructions not supported
	case 0x4D: ret(); break; // reti (??)
	case 0x45: ret(); iff1 = iff2; break; // retn
	
	case 0x47: Ibase = A; break;
	case 0x57: A = Ibase; setSZP(A); break; //todo: flags
	
	case 0x4F: R = A; break;
	case 0x5F: A = R; F.b.N = F.b.fH = 0; setSZP(A); set35(A); F.b.V = iff2; break;
	
	case 0x40: B = in(BC); setSZP(B); set35(B); F.b.N = F.b.fH = 0; break;
	case 0x50: D = in(BC); setSZP(D); set35(D); F.b.N = F.b.fH = 0; break;	
	case 0x60: H = in(BC); setSZP(H); set35(H); F.b.N = F.b.fH = 0; break;	
	case 0x70: temp = in(BC); setSZP(temp); set35(temp); F.b.N = F.b.fH = 0; break;
	case 0x48: C = in(BC); setSZP(C); set35(C); F.b.N = F.b.fH = 0; break;
	case 0x58: E = in(BC); setSZP(E); set35(E); F.b.N = F.b.fH = 0; break;	
	case 0x68: L = in(BC); setSZP(L); set35(L); F.b.N = F.b.fH = 0; break;	
	case 0x78: A = in(BC); setSZP(A); set35(A); F.b.N = F.b.fH = 0; break;	
	
	case 0x41: out(BC, B); break;
	case 0x51: out(BC, D); break;
	case 0x61: out(BC, H); break;	
	case 0x71: out(BC, 0); break;
	case 0x49: out(BC, C); break;
	case 0x59: out(BC, E); break;
	case 0x69: out(BC, L); break;
	case 0x79: out(BC, A); break;
		
	case 0x42: setHL(add16f(HL, BC^0xffff, F.b.fC^1)); F.b.fC^=1; F.b.fH^=1; F.b.N=1; set35(H); break;
	case 0x43: write16(read16(pc), BC); pc += 2; break;
	case 0x44: A = adc(0, A^0xff, 1); F.b.fC^=1; F.b.fH^=1; set35(A); F.b.N=1; break;
	
	case 0x4A: setHL(add16f(HL, BC, F.b.fC)); set35(H); F.b.N=0; break;
	case 0x4B: setBC(read16(read16(pc))); pc += 2; break;
	
	case 0x52: setHL(add16f(HL, DE^0xffff, F.b.fC^1)); F.b.fC^=1; F.b.fH^=1; F.b.N=1; set35(H); break;
	case 0x53: write16(read16(pc), DE); pc += 2; break;

	case 0x5A: setHL(add16f(HL, DE, F.b.fC)); set35(H); F.b.N=0; break;
	case 0x5B: setDE(read16(read16(pc))); pc += 2; break;
	
	case 0x62: setHL(add16f(HL, HL^0xffff, F.b.fC^1)); F.b.fC^=1; F.b.fH^=1; F.b.N=1; set35(H); break;
	
	case 0x67:{ //rrd
		temp = read(HL);
		u8 n = A;
		A = (A&0xf0)|(temp&0xf);
		temp = (temp>>4)|(n<<4);
		write(HL, temp);
		setSZP(A);
		set35(A);
		F.b.N = F.b.fH = 0;
		}break;
	
	case 0x6A: setHL(add16f(HL, HL, F.b.fC)); set35(H); F.b.N=0; break;

	case 0x6F:{ //rld
		temp = read(HL);
		u8 n = A;
		A = (A&0xf0)|(temp>>4);
		temp = (temp<<4)|(n&0xf);
		write(HL, temp);
		setSZP(A);
		set35(A);
		F.b.N = F.b.fH = 0;
		}break;

	case 0x72: setHL(add16f(HL, sp^0xffff, F.b.fC^1)); F.b.fC^=1; F.b.fH^=1; F.b.N=1; set35(H); break;
	
	case 0x7A: setHL(add16f(HL, sp, F.b.fC)); set35(H); F.b.N=0; break;
	
	
	case 0x73: write16(read16(pc), sp); pc += 2; break;
	
	case 0x7B: sp = read16(read16(pc)); pc += 2; break;

	case 0xA0:
	case 0xB0: //ldi/r
		temp = read(HL);
		write(DE, temp);
		setHL(HL+1);
		setDE(DE+1);
		F.b.fH = F.b.N = 0;
		temp += A;
		set35(((temp<<4)&0x20)|(temp&8));
		setBC(BC-1);
		if( BC )
		{
			F.b.V = 1;
			if( opc == 0xB0 ) pc -= 2;
		} else {
			F.b.V = 0;
		}
		break;
	case 0xA8:
	case 0xB8: //ldd/r
		temp = read(HL);
		write(DE, temp);
		setHL(HL-1);
		setDE(DE-1);
		F.b.fH = F.b.N = 0;
		temp += A;
		set35(((temp<<4)&0x20)|(temp&8));
		setBC(BC-1);
		if( BC )
		{
			F.b.V = 1;
			if( opc == 0xB8 ) pc -= 2;
		} else {
			F.b.V = 0;
		}
		break;

	case 0xA1:
	case 0xA9: //cpi/d
		temp = read(HL);
		adc(A, temp^0xff, 1);
		F.b.fH ^= 1;
		if( opc == 0xA9 )
		{
			setHL(HL-1);
		} else {
			setHL(HL+1);
		}
		temp = A - temp - F.b.fH;
		set35(((temp<<4)&0x20)|(temp&8));
		F.b.N = 1;
		F.b.fC = oldC;
		setBC(BC-1);
		F.b.V = (BC==0)?0:1;		
		break;
	case 0xB1:
	case 0xB9: //cpi/dr
		temp = read(HL);
		adc(A, temp^0xff, 1);
		F.b.fH ^= 1;
		if( opc == 0xB9 )
		{
			setHL(HL-1);
		} else {
			setHL(HL+1);
		}
		temp = A - temp - F.b.fH;
		set35(((temp<<4)&0x20)|(temp&8));
		F.b.N = 1;
		F.b.fC = oldC;
		setBC(BC-1);
		F.b.V = (BC==0)?0:1;
		if( BC && F.b.Z == 0 ) pc -= 2;	
		break;
		
	case 0xA2:
	case 0xB2: // ini/inir //flags todo
		temp = in(BC);
		write(HL, temp);
		B--;
		setHL(HL+1);
		if( opc == 0xB2 && B ) pc -= 2;	
		F.b.Z = (B==0)?1:0;
		F.b.N = 0;
		break;
		
	case 0xA3:
	case 0xB3: //outi/otir
		temp = read(HL);
		out(BC, temp);
		B--;
		setHL(HL+1);
		if( opc == 0xB3 && B ) pc -= 2;	
		F.b.Z = (B==0)?1:0;
		F.b.N = 0;
		break;
		
	case 0xAB:
	case 0xBB: //outd/otdr
		temp = read(HL);
		out(BC, temp);
		B--;
		setHL(HL-1);
		if( opc == 0xBB && B ) pc -= 2;
		F.b.Z = (B==0)?1:0;
		F.b.N = 0;
		break;
	
	default:
		printf("ED opcode $%X\n", opc);
		exit(1);
	}
	return 4;
}

void z80::setreg(u8 reg, u8 v)
{
	// only used in cb and ed when prefix is not needed/already handled
	if( reg == 6 ) write(HL, v);
	else r[reg] = v;
}

u8 z80::getreg(u8 v)
{
	if( v == 4 )
	{
		if( prefix == 0xFD ) return IYH;
		if( prefix == 0xDD ) return IXH;
		return H;
	}
	if( v == 5 )
	{
		if( prefix == 0xFD ) return IYL;
		if( prefix == 0xDD ) return IXL;
		return L;
	}
	if( v == 6 )
	{
		if( prefix )
		{
			printf("This should have been handled elsewhere\n");
			exit(1);
		}
		return read(HL);
	}
	return r[v];
}

u16 z80::getHL()
{
	if( prefix == 0xFD ) return IY;
	if( prefix == 0xDD ) return IX;
	return HL;
}

void z80::setHL(u16 v)
{
	if( prefix == 0xFD ) { IYH = v>>8; IYL = v; }
	else if( prefix == 0xDD ) { IXH = v>>8; IXL = v; }
	else {
		H = v>>8; L = v;
	}
}

void z80::setDE(u16 v) { D = v>>8; E = v; }
void z80::setBC(u16 v) { B = v>>8; C = v; }

u8 z80::adc(u8 a, u8 b, u8 c)
{
	u16 V = a;
	V += b;
	V += c;
	
	F.b.fH = ((a&0xf) + (b&0xf) + c)>0xf;
	F.b.fC = V>>8;
	F.b.N = 0;
	F.b.V = ((a^V) & (b^V))>>7;
	F.b.Z = ((V&0xff)==0);
	F.b.S = V>>7;
	return V;
}

u16 z80::add16(u16 a, u16 b, u16 c)
{
	u32 V = a;
	V += b;
	V += c;
	
	F.b.fC = V>>16;
	F.b.fH = ((a&0xfff)+(b&0xfff)+c)>0xfff;
	F.b.N = 0;
	set35(V>>8);
	//F.b.Z = (V&0xffff)==0;
	//F.b.S = V>>15;
	//F.b.V = ((a^V)&(b^V))>>15;
	return V;
}

u16 z80::add16f(u16 a, u16 b, u16 c)
{
	u32 V = a;
	V += b;
	V += c;
	
	F.b.fC = V>>16;
	F.b.fH = ((a&0xfff)+(b&0xfff)+c)>0xfff;
	F.b.N = 0;
	set35(V>>8);
	F.b.Z = (V&0xffff)==0;
	F.b.S = V>>15;
	F.b.V = ((a^V)&(b^V))>>15;
	return V;
}

u8 z80::inc(u8 v)
{
	F.b.V = (v==0x7F);
	v++;
	F.b.Z = (v==0);
	F.b.S = (v>>7);
	F.b.N = 0;
	F.b.fH = ((v&0xf)==0);
	set35(v);
	return v;
}

u8 z80::dec(u8 v)
{
	F.b.V = (v==0x80);
	v--;
	F.b.Z = (v==0);
	F.b.S = (v>>7);
	F.b.N = 1;
	F.b.fH = ((v&0xf)==0xf);
	set35(v);
	return v;
}

void z80::rcla()
{
	F.b.fH = F.b.N = 0;
	A = (A<<1)|(A>>7);
	F.b.fC = A&1;
	set35(A);
}

void z80::rrca()
{
	F.b.fH = F.b.N = 0;
	A = (A<<7)|(A>>1);
	F.b.fC = A>>7;
	set35(A);
}

void z80::rra()
{
	F.b.fH = F.b.N = 0;
	u8 oldC = F.b.fC;
	F.b.fC = A&1;
	A >>= 1;
	A |= oldC<<7;
	set35(A);
}

void z80::rla()
{
	F.b.fH = F.b.N = 0;
	u8 oldC = F.b.fC;
	F.b.fC = A>>7;
	A <<= 1;
	A |= oldC;
	set35(A);
}

void z80::daa()
{
	u8 dif = 0;
	if( F.b.fC )
	{
		dif = 0x60;
		if( (A & 0xf) > 9 || (F.b.fH) ) dif |= 6;	
	} else if( (F.b.fH) && (A & 0xf) < 0xA ) {
		dif = 6;
		if( (A>>4) > 9 ) dif |= 0x60;	
	} else if( (A & 0xf) > 9 ) {
		dif = 6;
		if( (A >> 4) > 8 ) dif |= 0x60;
	} else if( (A >> 4) > 9 ) {
		dif = 0x60;
	}
	
	if( F.b.fC == 0 )
	{
		if( ((A&0xf) > 9 && (A>>4) > 8)
		  ||((A&0xf) < 0xA && (A>>4) > 9) )
		  	F.b.fC = 1;
	}
	
	if( F.b.N )
	{
		if( F.b.fH )
		{
			if( (A&0xf) > 5 ) F.b.fH = 0;
			else 		  F.b.fH = 1;
		}
		A -= dif;
	} else {
		if( (A&0xf) < 0xA ) F.b.fH = 0;
		else		       F.b.fH = 1;
		A += dif;
	}
	
	setSZP(A);
	set35(A);
}

void z80::reset()
{
	pc = 0;
	sp = 0xdff0;
	irq_line = 0;
	iff1 = iff2 = 0;
	intr_blocked = halted = false;
}





