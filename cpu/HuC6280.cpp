#include <print>
#include <string>
#include "HuC6280.h"

u64 HuC6280::step()
{
	if( continueXfer == 0 )
	{
		if( !irq_blocked && F.b.I==0 && irq_line && !(irq_enable&BIT(1)) )
		{
			push(pc>>8);
			push(pc);
			F.b.T = nextT;
			nextT = 0;
			push(F.v);
			F.b.I = 1;
			pc = read(0xFFF8);
			pc |= read(0xFFF9)<<8;
			irq_blocked = false;
			return 7;
		}
		irq_blocked = false;
	}

	u8 opc = (continueXfer ? continueXfer : fetch());
	u16 addr = 0;
	u8 t = 0;
	int cycles = 2;
	F.b.T = nextT;
	nextT = 0;		

	u8 oldC = F.b.C;
	switch( opc )
	{
	case 0xE3: // tia
		if( continueXfer==0 )
		{
			xfer_src = fetch16();
			xfer_dst = fetch16();
			xfer_len = fetch16();
			xfer_alt = 0;
			continueXfer = opc;
			return 17;
		}
		write(xfer_dst + xfer_alt, read(xfer_src));			
		xfer_alt ^= 1;
		xfer_src += 1;
		xfer_len -= 1;
		if( xfer_len == 0 ) { continueXfer = 0; }
		return 6;	
	case 0xF3: // tai
		if( continueXfer==0 )
		{
			xfer_src = fetch16();
			xfer_dst = fetch16();
			xfer_len = fetch16();
			xfer_alt = 0;
			continueXfer = opc;
			return 17;
		}
		write(xfer_dst, read(xfer_src + xfer_alt));			
		xfer_alt ^= 1;
		xfer_dst += 1;
		xfer_len -= 1;
		if( xfer_len == 0 ) { continueXfer = 0; }
		return 6;

	case 0xC3: // tdd
		if( continueXfer==0 )
		{
			xfer_src = fetch16();
			xfer_dst = fetch16();
			xfer_len = fetch16();
			continueXfer = opc;
			return 17;
		}
		write(xfer_dst, read(xfer_src));			
		xfer_dst -= 1;
		xfer_src -= 1;
		xfer_len -= 1;
		if( xfer_len == 0 ) { continueXfer = 0; }
		return 6;	
	case 0x73: // tii
		if( continueXfer==0 )
		{
			xfer_src = fetch16();
			xfer_dst = fetch16();
			xfer_len = fetch16();
			continueXfer = opc;
			return 17;
		}
		write(xfer_dst, read(xfer_src));			
		xfer_dst += 1;
		xfer_src += 1;
		xfer_len -= 1;
		if( xfer_len == 0 ) { continueXfer = 0; }
		return 6;
	case 0xD3: // tin
		if( continueXfer==0 )
		{
			xfer_src = fetch16();
			xfer_dst = fetch16();
			xfer_len = fetch16();
			continueXfer = opc;
			return 17;
		}
		write(xfer_dst, read(xfer_src));			
		xfer_src += 1;
		xfer_len -= 1;
		if( xfer_len == 0 ) { continueXfer = 0; }
		return 6;	
	case 0x48: push(A); return 3; // pha
	case 0xDA: push(X); return 3; // phx
	case 0x5A: push(Y); return 3; // phy
	case 0x08: F.b.T=0; push(F.v|0x10); return 3; // php
	case 0x68: setnz(A=pop()); return 4; // pla
	case 0x28: F.v = pop(); nextT=F.b.T; return 4; // plp
	case 0xFA: setnz(X=pop()); return 4; // plx
	case 0x7A: setnz(Y=pop()); return 4; // ply
	
	case 0x22: std::swap(A, X); return 3; // sax
	case 0x42: std::swap(A, Y); return 3; // say
	case 0x02: std::swap(X, Y); return 3; // sxy
	case 0xAA: setnz(X=A); return 2; // tax
	case 0xA8: setnz(Y=A); return 2; // tay
	case 0xBA: setnz(X=S); return 2; // tsx
	case 0x8A: setnz(A=X); return 2; // txa
	case 0x98: setnz(A=Y); return 2; // tya
	case 0x9A: S = X; return 2;   // txs
	
	case 0x78: F.b.I=1; return 2; // sei (todo: block irqs?)
	case 0xF8: F.b.D=1; return 2; // sed
	case 0x38: F.b.C=1; return 2; // sec
	case 0xF4: nextT=1; return 2; // set
	case 0x18: F.b.C=0; return 2; // clc
	case 0xD8: F.b.D=0; return 2; // cld
	case 0x58: F.b.I=0; return 2; // cli (todo: block irqs for an instruction)
	case 0xB8: F.b.V=0; return 2; // clv
	
	
	case 0x62: A=0; return 2; // cla
	case 0xC2: Y=0; return 2; // cly
	case 0x82: X=0; return 2; // clx
	
	case 0xCA: X-=1; setnz(X); return 2; // dex
	case 0xE8: X+=1; setnz(X); return 2; // inx
	case 0x88: Y-=1; setnz(Y); return 2; // dey
	case 0xC8: Y+=1; setnz(Y); return 2; // iny
	
	case 0x4C: pc=fetch16(); return 4; // jmp abs
	case 0x6C: addr=fetch16(); pc=read(addr); pc|=read(addr+1)<<8; return 7; // jmp (abs)
	case 0x7C: addr=fetch16()+X; pc=read(addr); pc|=read(addr+1)<<8; return 7; // jmp (abs,x)

	case 0x0F: case 0x1F: case 0x2F: case 0x3F:
	case 0x4F: case 0x5F: case 0x6F: case 0x7F: // bbr
		t = read(0x2000+fetch());
		addr = fetch();
		if( !(t & BIT((opc>>4)&7)) )
		{
			pc += (s8)addr;
		}
		return 6; //todo: check cycles
	case 0x8F: case 0x9F: case 0xAF: case 0xBF:
	case 0xCF: case 0xDF: case 0xEF: case 0xFF: // bbs
		t = read(0x2000+fetch());
		addr = fetch();
		if( t & BIT((opc>>4)&7) )
		{
			pc += (s8)addr;
		}
		return 6; //todo: check cycles
	
	case 0x80: // bra (todo: check cycles)
		t=fetch(); 
		cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
		pc += (s8)t; 
		return cycles;
	case 0x70: // bvs
		t=fetch(); 
		if(F.b.V==1) 
		{
			cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
			pc += (s8)t; 
		}
		return cycles;
	case 0x50: // bvc
		t=fetch(); 
		if(F.b.V==0) 
		{
			cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
			pc += (s8)t; 
		}
		return cycles;
	case 0x30: // bmi
		t=fetch(); 
		if(F.b.N==1) 
		{
			cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
			pc += (s8)t; 
		}
		return cycles;
	case 0x10: // bpl
		t=fetch(); 
		if(F.b.N==0) 
		{
			cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
			pc += (s8)t; 
		}
		return cycles;
	case 0x90: // bcc
		t=fetch(); 
		if(F.b.C==0) 
		{
			cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
			pc += (s8)t; 
		}
		return cycles;
	case 0xB0: // bcs
		t=fetch(); 
		if(F.b.C==1) 
		{
			cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
			pc += (s8)t; 
		}
		return cycles;
	case 0xF0: // beq
		t=fetch(); 
		if(F.b.Z==1) 
		{
			cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
			pc += (s8)t; 
		}
		return cycles;
	case 0xD0: // bne
		t=fetch(); 
		if(F.b.Z==0) 
		{
			cycles += 1 + ((pc&0xff00)!=((pc+(s8)t)&0xff00));
			pc += (s8)t; 
		}
		return cycles;
	
	case 0x64: write(0x2000+fetch(), 0); return 4; // stz zp
	case 0x74: write(0x2000+((fetch()+X)&0xff), 0); return 4; //stz zp,x
	case 0x9C: write(fetch16(), 0); return 5; // stz abs
	case 0x9E: write(fetch16()+X, 0); return 5; // stz abs, x

	case 0x89: t=fetch(); F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 2; //bit imm
	case 0x24: t=read(0x2000+fetch()); F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 4; //bit zp
	case 0x34: t=read(0x2000+((fetch()+X)&0xff)); F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 4; //bit zp,x
	case 0x2C: t=read(fetch16()); F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 5; //bit abs
	case 0x3C: t=read(fetch16()+X); F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 5; //bit abs,x

	case 0x85: write(0x2000+fetch(), A); return 4; // sta zp
	case 0x95: write(0x2000+((fetch()+X)&0xff), A); return 4; //sta zp, x
	case 0x92: t=fetch(); addr=read(0x2000+t); ++t; addr|=read(0x2000+t)<<8; write(addr,A); return 7; // sta (ind)
	case 0x81: t=fetch()+X; addr=read(0x2000+t); ++t; addr|=read(0x2000+t)<<8; write(addr,A); return 7; // sta (ind,x)
	case 0x91: t=fetch(); addr=read(0x2000+t); ++t; addr|=read(0x2000+t)<<8; write(addr+Y,A); return 7; // sta (ind),y
	case 0x8D: write(fetch16(), A); return 5; // sta abs
	case 0x9D: write(fetch16()+X, A); return 5; // sta abs,x
	case 0x99: write(fetch16()+Y, A); return 5; // sta abs,y
	
	case 0x86: write(0x2000+fetch(), X); return 4; // stx zp
	case 0x96: write(0x2000+((fetch()+Y)&0xff), X); return 4; // stx zp,y
	case 0x8E: write(fetch16(), X); return 5; // stx abs
	
	case 0x84: write(0x2000+fetch(), Y); return 4; // sty zp
	case 0x94: write(0x2000+((fetch()+X)&0xff), Y); return 4; // sty zp,x
	case 0x8C: write(fetch16(), Y); return 5; // sty abs

	case 0xA0: setnz(Y=fetch()); return 2; // ldy imm
	case 0xA4: setnz(Y=read(0x2000+fetch())); return 4; // ldy zp
	case 0xB4: setnz(Y=read(0x2000+((fetch()+X)&0xff))); return 4; //ldy zp,x
	case 0xAC: setnz(Y=read(fetch16())); return 5; // ldy abs
	case 0xBC: setnz(Y=read(fetch16()+X)); return 5; //ldy abs,x
	
	case 0xA2: setnz(X=fetch()); return 2; // ldx imm
	case 0xA6: setnz(X=read(0x2000+fetch())); return 4; // ldx zp
	case 0xB6: setnz(X=read(0x2000+((fetch()+Y)&0xff))); return 4; //ldx zp,y
	case 0xAE: setnz(X=read(fetch16())); return 5; // ldx abs
	case 0xBE: setnz(X=read(fetch16()+Y)); return 5; //ldx abs,y
	
	case 0xA9: setnz(A=fetch()); return 2; // lda imm
	case 0xA5: setnz(A=read(0x2000+fetch())); return 4; //lda zp
	case 0xB5: setnz(A=read(0x2000+((fetch()+X)&0xff))); return 4;//lda zp,x
	case 0xB2: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; setnz(A=read(addr)); return 7; //lda (ind)
	case 0xA1: t=fetch()+X; addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; setnz(A=read(addr)); return 7; //lda (ind,x)
	case 0xB1: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; setnz(A=read(addr+Y)); return 7; //lda (ind),y
	case 0xAD: setnz(A=read(fetch16())); return 5; // lda abs
	case 0xBD: setnz(A=read(fetch16()+X)); return 5; // lda abs,x
	case 0xB9: setnz(A=read(fetch16()+Y)); return 5; // lda abs,y

	case 0x20: addr=fetch16(); pc-=1; push(pc>>8); push(pc); pc=addr; return 7; // jsr
	case 0x44: addr=fetch(); pc-=1; push(pc>>8); push(pc); pc += 1 + (s8)addr; return 8; // bsr
	case 0x60: pc=pop(); pc|=pop()<<8; pc+=1; return 7; // rts
	case 0x40: F.v=pop(); nextT=F.b.T; pc=pop(); pc|=pop()<<8; return 7; // rti

	case 0xC9: cmp(A, fetch()); return 2; // cmp imm
	case 0xC5: cmp(A, read(0x2000+fetch())); return 4; //cmp zp
	case 0xD5: cmp(A, read(0x2000+((fetch()+X)&0xff))); return 4;//cmp zp,x
	case 0xD2: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; cmp(A, read(addr)); return 7; //cmp (ind)
	case 0xC1: t=fetch()+X; addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; cmp(A, read(addr)); return 7; //cmp (ind,x)
	case 0xD1: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; cmp(A, read(addr+Y)); return 7; //cmp (ind),y
	case 0xCD: cmp(A, read(fetch16())); return 5; // cmp abs
	case 0xDD: cmp(A, read(fetch16()+X)); return 5; // cmp abs,x
	case 0xD9: cmp(A, read(fetch16()+Y)); return 5; // cmp abs,y

	case 0x6A: // ror a
		F.b.C = A&1;
		A = (A>>1)|(oldC<<7);
		setnz(A);
		return 6;
	case 0x66: // ror zp
		addr = 0x2000+fetch();
		t = read(addr);
		F.b.C = t&1;
		t = (t>>1)|(oldC<<7);
		setnz(t);
		write(addr, t);
		return 6;
	case 0x76: // ror zp,x
		addr = 0x2000+((fetch()+X)&0xff);
		t = read(addr);
		F.b.C = t&1;
		t = (t>>1)|(oldC<<7);
		setnz(t);
		write(addr, t);
		return 6;
	case 0x6E: // ror abs
		addr = fetch16();
		t = read(addr);
		F.b.C = t&1;
		t = (t>>1)|(oldC<<7);
		setnz(t);
		write(addr, t);
		return 7;
	case 0x7E: // ror abs,x
		addr = fetch16()+X;
		t = read(addr);
		F.b.C = t&1;
		t = (t>>1)|(oldC<<7);
		setnz(t);
		write(addr, t);
		return 7;
	case 0x2A: // rol a
		F.b.C = (A>>7)&1;
		A = (A<<1)|oldC;
		setnz(A);
		return 6;
	case 0x26: // rol zp
		addr = 0x2000+fetch();
		t = read(addr);
		F.b.C = (t>>7)&1;
		t = (t<<1)|oldC;
		setnz(t);
		write(addr, t);
		return 6;
	case 0x36: // rol zp,x
		addr = 0x2000+((fetch()+X)&0xff);
		t = read(addr);
		F.b.C = (t>>7)&1;
		t = (t<<1)|oldC;
		setnz(t);
		write(addr, t);
		return 6;
	case 0x2E: // rol abs
		addr = fetch16();
		t = read(addr);
		F.b.C = (t>>7)&1;
		t = (t<<1)|oldC;
		setnz(t);
		write(addr, t);
		return 7;
	case 0x3E: // rol abs,x
		addr = fetch16()+X;
		t = read(addr);
		F.b.C = (t>>7)&1;
		t = (t<<1)|oldC;
		setnz(t);
		write(addr, t);
		return 7;	

	case 0x1A: // inc a (ina)
		setnz(A+=1);
		return 2;	
	case 0xE6: // inc zp
		addr=0x2000+fetch();
		setnz(t = read(addr) + 1);
		write(addr, t);
		return 6;
	case 0xF6: // inc zp,x
		addr=0x2000+((fetch()+X)&0xff);
		setnz(t = read(addr) + 1);
		write(addr, t);
		return 6;
	case 0xEE: // inc abs
		addr=fetch16();
		setnz(t = read(addr) + 1);
		write(addr, t);
		return 7;
	case 0xFE: // inc abs,x
		addr=fetch16()+X;
		setnz(t = read(addr) + 1);
		write(addr, t);
		return 7;
	case 0x3A: // dec a (ina)
		setnz(A-=1);
		return 2;	
	case 0xC6: // dec zp
		addr=0x2000+fetch();
		setnz(t = read(addr) - 1);
		write(addr, t);
		return 6;
	case 0xD6: // dec zp,x
		addr=0x2000+((fetch()+X)&0xff);
		setnz(t = read(addr) - 1);
		write(addr, t);
		return 6;
	case 0xCE: // dec abs
		addr=fetch16();
		setnz(t = read(addr) - 1);
		write(addr, t);
		return 7;
	case 0xDE: // dec abs,x
		addr=fetch16()+X;
		setnz(t = read(addr) - 1);
		write(addr, t);
		return 7;
	

	case 0x14: // trb zp
		addr = 0x2000+fetch();
		t=read(addr); 
		F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1;
		write(addr, t&~A);
		return 6;
	case 0x1C: // trb abs
		addr = fetch16();
		t=read(addr); 
		F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1;
		write(addr, t&~A);
		return 7;
	case 0x04: // tsb zp
		addr = 0x2000+fetch();
		t=read(addr); 
		F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1;
		write(addr, t|A);
		return 6;
	case 0x0C: // tsb abs
		addr = fetch16();
		t=read(addr); 
		F.b.Z=((A&t)==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1;
		write(addr, t|A);
		return 7;
		
	case 0x83: t=fetch(); t&=read(0x2000+fetch()); F.b.Z=(t==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 7; // tst zp
	case 0xA3: t=fetch(); t&=read(0x2000+((fetch()+X)&0xff)); F.b.Z=(t==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 7; // tst zp,x
	case 0x93: t=fetch(); t&=read(fetch16()); F.b.Z=(t==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 8; // tst abs
	case 0xB3: t=fetch(); t&=read(fetch16()+X); F.b.Z=(t==0); F.b.N=(t>>7)&1; F.b.V=(t>>6)&1; return 8; // tst abs,x

	case 0xE0: cmp(X, fetch()); return 2; // cpx imm
	case 0xE4: cmp(X, read(0x2000+fetch())); return 4; // cpx zp
	case 0xEC: cmp(X, read(fetch16())); return 5; // cpx abs
	case 0xC0: cmp(Y, fetch()); return 2; // cpy imm
	case 0xC4: cmp(Y, read(0x2000+fetch())); return 4; // cpy zp
	case 0xCC: cmp(Y, read(fetch16())); return 5; // cpy abs
	
	case 0xE9: A=sbc(A, fetch()); return 2; // sbc imm
	case 0xE5: A=sbc(A, read(0x2000+fetch())); return 4; // sbc zp
	case 0xF5: A=sbc(A, read(0x2000+((fetch()+X)&0xff))); return 4; // sbc zp,x
	case 0xF2: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; A=sbc(A, read(addr)); return 7; // sbc (ind)
	case 0xE1: t=fetch()+X; addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; A=sbc(A, read(addr)); return 7; // sbc (ind,x)
	case 0xF1: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; A=sbc(A, read(addr+Y)); return 7; // sbc (ind),y
	case 0xED: A=sbc(A, read(fetch16())); return 5; // sbc abs
	case 0xFD: A=sbc(A, read(fetch16()+X)); return 5; // sbc abs,x
	case 0xF9: A=sbc(A, read(fetch16()+Y)); return 5; // sbc abs,y
	
	case 0x4A: // lsr a
		F.b.C = A&1;
		A >>= 1;
		setnz(A);
		return 2;
	case 0x46: // lsr zp
		addr = 0x2000+fetch();
		t = read(addr);
		F.b.C = t&1;
		t >>= 1;
		setnz(t);
		write(addr, t);
		return 6;
	case 0x56: // lsr zp,x
		addr = 0x2000+((fetch()+X)&0xff);
		t = read(addr);
		F.b.C = t&1;
		t >>= 1;
		setnz(t);
		write(addr, t);
		return 6;
	case 0x4E: // lsr abs
		addr = fetch16();
		t = read(addr);
		F.b.C = t&1;
		t >>= 1;
		setnz(t);
		write(addr, t);
		return 7;
	case 0x5E: // lsr abs,x
		addr = fetch16()+X;
		t = read(addr);
		F.b.C = t&1;
		t >>= 1;
		setnz(t);
		write(addr, t);
		return 7;
	case 0x0A: // asl a
		F.b.C = (A>>7)&1;
		A <<= 1;
		setnz(A);
		return 2;
	case 0x06: // asl zp
		addr = 0x2000+fetch();
		t = read(addr);
		F.b.C = (t>>7)&1;
		t <<= 1;
		setnz(t);
		write(addr, t);
		return 6;
	case 0x16: // asl zp,x
		addr = 0x2000+((fetch()+X)&0xff);
		t = read(addr);
		F.b.C = (t>>7)&1;
		t <<= 1;
		setnz(t);
		write(addr, t);
		return 6;
	case 0x0E: // asl abs
		addr = fetch16();
		t = read(addr);
		F.b.C = (t>>7)&1;
		t <<= 1;
		setnz(t);
		write(addr, t);
		return 7;
	case 0x1E: // asl abs,x
		addr = fetch16()+X;
		t = read(addr);
		F.b.C = (t>>7)&1;
		t <<= 1;
		setnz(t);
		write(addr, t);
		return 7;

// ORA
	case 0x09: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)|fetch())); return 5; } else { setnz(A|=fetch()); return 2; } // ora imm
	case 0x05: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)|read(0x2000+fetch()))); return 7; } else { setnz(A|=read(0x2000+fetch())); return 4; } //ora zp
	case 0x15: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)|read(0x2000+((fetch()+X)&0xff)))); return 7; } else { setnz(A|=read(0x2000+((fetch()+X)&0xff))); return 4; }//ora zp,x
	case 0x12: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)|read(addr))); return 10; } else { setnz(A|=read(addr)); return 7; } //ora (ind)
	case 0x01: t=fetch()+X; addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)|read(addr))); return 10; } else { setnz(A|=read(addr)); return 7;} //ora (ind,x)
	case 0x11: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)|read(addr+Y))); return 10; } else { setnz(A|=read(addr+Y)); return 7;} //ora (ind),y
	case 0x0D: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)|read(fetch16()))); return 8; } else { setnz(A|=read(fetch16())); return 5;} // ora abs
	case 0x1D: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)|read(fetch16()+X))); return 8; } else { setnz(A|=read(fetch16()+X)); return 5;} // ora abs,x
	case 0x19: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)|read(fetch16()+Y))); return 8; } else { setnz(A|=read(fetch16()+Y)); return 5;} // ora abs,y
// AND
	case 0x29: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)&fetch())); return 5; } else { setnz(A&=fetch()); return 2; } // and imm
	case 0x25: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)&read(0x2000+fetch()))); return 7; } else { setnz(A&=read(0x2000+fetch())); return 4; } //and zp
	case 0x35: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)&read(0x2000+((fetch()+X)&0xff)))); return 7; } else { setnz(A&=read(0x2000+((fetch()+X)&0xff))); return 4; }//and zp,x
	case 0x32: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)&read(addr))); return 10; } else { setnz(A&=read(addr)); return 7; } //and (ind)
	case 0x21: t=fetch()+X; addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)&read(addr))); return 10; } else { setnz(A&=read(addr)); return 7;} //and (ind,x)
	case 0x31: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)&read(addr+Y))); return 10; } else { setnz(A&=read(addr+Y)); return 7;} //and (ind),y
	case 0x2D: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)&read(fetch16()))); return 8; } else { setnz(A&=read(fetch16())); return 5;} // and abs
	case 0x3D: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)&read(fetch16()+X))); return 8; } else { setnz(A&=read(fetch16()+X)); return 5;} // and abs,x
	case 0x39: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)&read(fetch16()+Y))); return 8; } else { setnz(A&=read(fetch16()+Y)); return 5;} // and abs,y
//EOR
	case 0x49: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)^fetch())); return 5; } else { setnz(A^=fetch()); return 2; } // eor imm
	case 0x45: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)^read(0x2000+fetch()))); return 7; } else { setnz(A^=read(0x2000+fetch())); return 4; } //eor zp
	case 0x55: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)^read(0x2000+((fetch()+X)&0xff)))); return 7; } else { setnz(A^=read(0x2000+((fetch()+X)&0xff))); return 4; }//eor zp,x
	case 0x52: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)^read(addr))); return 10; } else { setnz(A^=read(addr)); return 7; } //eor (ind)
	case 0x41: t=fetch()+X; addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)^read(addr))); return 10; } else { setnz(A^=read(addr)); return 7;} //eor (ind,x)
	case 0x51: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,setnz(read(0x2000+X)^read(addr+Y))); return 10; } else { setnz(A^=read(addr+Y)); return 7;} //eor (ind),y
	case 0x4D: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)^read(fetch16()))); return 8; } else { setnz(A^=read(fetch16())); return 5;} // eor abs
	case 0x5D: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)^read(fetch16()+X))); return 8; } else { setnz(A^=read(fetch16()+X)); return 5;} // eor abs,x
	case 0x59: if(F.b.T) { write(0x2000+X, setnz(read(0x2000+X)^read(fetch16()+Y))); return 8; } else { setnz(A^=read(fetch16()+Y)); return 5;} // eor abs,y
// ADC
	case 0x69: if(F.b.T) { write(0x2000+X, adc(read(0x2000+X),fetch())); return 5; } else { A=adc(A,fetch()); return 2; } // adc imm
	case 0x65: if(F.b.T) { write(0x2000+X, adc(read(0x2000+X),read(0x2000+fetch()))); return 7; } else { A=adc(A,read(0x2000+fetch())); return 4; } //adc zp
	case 0x75: if(F.b.T) { write(0x2000+X, adc(read(0x2000+X),read(0x2000+((fetch()+X)&0xff)))); return 7; } else { A=adc(A,read(0x2000+((fetch()+X)&0xff))); return 4; }//adc zp,x
	case 0x72: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,adc(read(0x2000+X),read(addr))); return 10; } else { A=adc(A,read(addr)); return 7; } //adc (ind)
	case 0x61: t=fetch()+X; addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,adc(read(0x2000+X),read(addr))); return 10; } else { A=adc(A,read(addr)); return 7;} //adc (ind,x)
	case 0x71: t=fetch(); addr=read(0x2000+t);++t;addr|=read(0x2000+t)<<8; if(F.b.T) { write(0x2000+X,adc(read(0x2000+X),read(addr+Y))); return 10; } else { A=adc(A,read(addr+Y)); return 7;} //adc (ind),y
	case 0x6D: if(F.b.T) { write(0x2000+X, adc(read(0x2000+X),read(fetch16()))); return 8; } else { A=adc(A,read(fetch16())); return 5;} // adc abs
	case 0x7D: if(F.b.T) { write(0x2000+X, adc(read(0x2000+X),read(fetch16()+X))); return 8; } else { A=adc(A,read(fetch16()+X)); return 5;} // adc abs,x
	case 0x79: if(F.b.T) { write(0x2000+X, adc(read(0x2000+X),read(fetch16()+Y))); return 8; } else { A=adc(A,read(fetch16()+Y)); return 5;} // adc abs,y


	case 0x07: case 0x17: case 0x27: case 0x37:
	case 0x47: case 0x57: case 0x67: case 0x77: // rmb (reset zero page bit)
		addr = 0x2000+fetch();
		t = read(addr);
		write(addr, t&~BIT((opc>>4)&7));
		return 7;	
	case 0x87: case 0x97: case 0xA7: case 0xB7:
	case 0xC7: case 0xD7: case 0xE7: case 0xF7: // smb (set zero page bit)
		addr = 0x2000+fetch();
		t = read(addr);
		write(addr, t|BIT((opc>>4)&7));
		return 7;
	case 0x53: // tam imm
		t = fetch();
		for(u32 i = 0; i < 8; ++i) { if(t&BIT(i)) lastmmr=mmr[i]=A; }
		return 5;
	case 0x43: // tma imm
		t = fetch();
		if( t==0 )
		{
			A = lastmmr;
		} else {
			A=0;
			for(u32 i = 0; i < 8; ++i) { if(t&BIT(i)) A|=mmr[i]; }
			lastmmr = A;
		}
		return 4;
	case 0x03: bus_write(0x1FE000, fetch()); return 4; // st0 imm
	case 0x13: bus_write(0x1FE002, fetch()); return 4; // st1 imm
	case 0x23: bus_write(0x1FE003, fetch()); return 4; // st2 imm
	case 0xD4: high_speed = true; return 2; // csh (todo: check cycles)
	case 0x54: high_speed = false; return 2; // csl (todo: check cycles)
	case 0xEA: return 2; // nop
	
	// undefs are all nops
	case 0x0B: case 0x1B: case 0x2B: 
	case 0x3B: case 0x4B: case 0x5B: 	
	case 0x6B: case 0x7B: case 0x8B: 
	case 0x9B: case 0xAB: case 0xBB: 
	case 0xCB: case 0xDB: case 0xEB:
	case 0xFB: //everything ending in xB are all nops
		 return 2;
	case 0x33: return 2;
	case 0x5C: return 2;
	case 0x63: return 2;
	case 0xDC: return 2;
	case 0xE2: return 2;
	case 0xFC: return 2;
	default: std::println("HuC6280::step(): Unimpl opc = ${:X}", opc); exit(1);
	}
}

void HuC6280::reset()
{
	pc = read(0xFFFE);
	pc |= read(0xFFFF)<<8;
	F.v = 0;
	F.b.I = 1;
	irq_blocked = irq_line = false;
	continueXfer = 0;
	mmr[7] = 0;
}

u8 HuC6280::adc(u8 a, u8 b)
{
	if( F.b.D )
	{
		std::println("ADC Decimal Mode Active");
		exit(1);
	}
	
	u16 r = a;
	r += b;
	r += F.b.C;
	F.b.C = (r>>8)&1;
	F.b.V = (((r^a)&(r^b)&0x80)? 1:0);
	F.b.N = (r>>7)&1;
	F.b.Z = (((r&0xff)==0)? 1:0);
	return r;
}

u8 HuC6280::sbc(u8 a, u8 b)
{
	if( F.b.D )
	{
		std::println("SBC Decimal Mode Active");
		exit(1);
	}
	
	return adc(a, b^0xff);
}

