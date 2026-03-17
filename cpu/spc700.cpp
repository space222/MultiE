#include <print>
#include "spc700.h"

#define cycle co_yield cc++

Yieldable spc700::run()
{
	u16 t16 = 0;
	u8 t = 0;
	while(1)
	{
		int cc = 0;
		opc = read(pc++);
		//std::println("SPC700 ${:X} opc ${:X}", pc-1, opc);
		cycle;
		switch( opc )
		{
		case 0x00: // nop
			cycle;
			break;
		case 0x04: // ORA dp
			t = read(pc++);
			cycle;
			A |= read(DP+t);
			setnz(A);
			cycle;
			break;
		case 0x05: // ora abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			A |= read(t16);
			setnz(A);
			cycle;
			break;
		case 0x06: // ORA A, (X)
			t = X;
			cycle;
			A |= read(DP+t);
			setnz(A);
			cycle;
			break;
		case 0x07: // ORA (dp, X)
			t = read(pc++);
			cycle;
			t += X;
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			A |= read(t16);
			setnz(A);
			cycle;
			break;
		case 0x08: // ora imm
			A |= read(pc++);
			setnz(A);
			cycle;
			break;
		case 0x09: // or dp, dp
			t = read(pc++);
			cycle;
			t = read(DP+t);
			cycle;
			opc = read(pc++);
			cycle;
			t16 = read(DP+opc);
			cycle;
			write(DP+opc, t|t16);
			setnz(t|t16);
			cycle;
			break;
		case 0x0A: // OR1 abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16&0x1fff);
			F.b.C |= ((t&BIT(t16>>13)) ? 1:0);
			cycle;
			break;
		case 0x0B: // asl dp
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			F.b.C = t16>>7;
			t16 <<= 1;
			setnz(t16);
			cycle;
			write(DP+t, t16);
			cycle;
			break;
		case 0x0C: // asl abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16);
			cycle;
			F.b.C = t>>7;
			t <<= 1;
			setnz(t);
			write(t16, t);
			cycle;
			break;
		case 0x0D: // php
			cycle;
			cycle;
			push(F.v);
			cycle;
			break;
		case 0x0E: // TSET1 !abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16);
			cycle;
			setnz(A-t);
			cycle;
			write(t16, A|t);
			cycle;
			break;
		case 0x0F: // brk
			push(pc>>8);
			cycle;
			push(pc);
			cycle;
			push(F.v);
			F.v |= 0x10;
			F.b.I = 0;
			cycle;
			pc = read(0xffde);
			cycle;
			pc |= read(0xffdf)<<8;
			cycle;
			break;
		case 0x10: // bpl rel
			t = read(pc++);
			cycle;
			if( F.b.N == 0 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0x14: // ora dp, x
			t = read(pc++);
			cycle;
			t += X;
			A |= read(DP+t);
			setnz(A);
			cycle;
			cycle;
			break;
		case 0x15: // ora abs, x
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += X;
			A |= read(t16);
			setnz(A);
			cycle;
			cycle;
			break;		
		case 0x16: // ora abs, y
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += Y;
			A |= read(t16);
			setnz(A);
			cycle;
			cycle;
			break;		
		case 0x17: // ora (dp),y
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			t16 += Y;
			A |= read(t16);
			setnz(A);
			cycle;
			cycle;
			break;
		case 0x18: // or dp, imm
			opc = read(pc++);
			cycle;
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			write(DP+t, t16|opc);
			setnz(t16|opc);
			cycle;
			break;
		case 0x19: // or (x), (y)
			t = read(DP+X);
			cycle;
			t16 = read(DP+Y);
			cycle;
			write(DP+X, t|t16);
			setnz(t|t16);
			cycle;
			cycle;		
			break;
		case 0x1A: // decw dp
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			t16 -= 1;
			F.b.N = t16>>15;
			F.b.Z = (t16==0 ? 1:0);
			cycle;
			write(DP+t, t16);
			cycle;
			write(DP+((t+1)&0xff), t16>>8);
			cycle;
			break;
		case 0x1B: // asl dp, x
			t = read(pc++);
			cycle;
			t += X;
			cycle;
			t16 = read(DP+t);
			cycle;
			F.b.C = t16>>7;
			t16 <<= 1;
			setnz(t16);
			write(DP+t, t16);
			cycle;
			break;
		case 0x1C: // asl a
			F.b.C = A>>7;
			A <<= 1;
			setnz(A);
			cycle;
			break;
		case 0x1D: // dec x
			X -= 1;
			setnz(X);
			cycle;
			break;
		case 0x1E: // cpx abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			cmp(X, read(t16));
			cycle;
			break;
		case 0x1F: // jmp (abs, x)  //todo: wrap with page like 6502?
			t16 = read(pc++);
			cycle;
			t16 |= read(pc)<<8;
			cycle;
			t16 += X;
			cycle;
			pc = read(t16);
			cycle;
			pc |= read(t16+1)<<8;
			cycle;		
			break;
		case 0x20: // clrp
			F.b.P = 0;
			DP = 0;
			cycle;
			break;
		case 0x24: // and dp
			t = read(pc++);
			cycle;
			A &= read(DP+t);
			setnz(A);
			cycle;
			break;		
		case 0x25: // and abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			A &= read(t16);
			setnz(A);
			cycle;
			break;
		case 0x26: // and (x)
			A &= read(DP+X);
			setnz(A);			
			cycle;
			cycle;		
			break;
		case 0x27: // and (dp, x)
			t = read(pc++)+X;
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			A &= read(t16);
			setnz(A);
			cycle;
			cycle;
			break;
		case 0x28: // AND imm
			A &= read(pc++);
			setnz(A);
			cycle;
			break;
		case 0x29: // and dp, dp
			t = read(pc++);
			cycle;
			t = read(DP+t);
			cycle;
			opc = read(pc++);
			cycle;
			t16 = read(DP+opc);
			cycle;
			write(DP+opc, t&t16);
			setnz(t&t16);
			cycle;
			break;
		case 0x2A: // OR1 /abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16&0x1fff);
			F.b.C |= ((t&BIT(t16>>13)) ? 0:1);
			cycle;
			break;
		case 0x2B:{ // rol dp
			t16 = read(pc++);
			cycle;
			t = read(DP+t16);
			cycle;
			u8 oldc = F.b.C;
			F.b.C = t>>7;
			t = (t<<1)|oldc;
			setnz(t);
			write(DP+t16, t);
			cycle;
			}break;
		case 0x2C:{ // rol abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16);
			cycle;
			u8 oldc = F.b.C;
			F.b.C = t>>7;
			t = (t<<1)|oldc;
			setnz(t);
			write(t16, t);
			cycle;
			}break;
		case 0x2D: // pha
			cycle;
			cycle;
			push(A);
			cycle;
			break;
		case 0x2E: // CBNE dp, rel
			t16 = read(pc++);
			cycle;
			t = read(pc++);
			cycle;
			cycle;
			t16 = read(DP+t16);
			cycle;
			if( t16 != A )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0x2F: // bra rel
			opc = read(pc++);
			cycle;
			pc += (s8)opc;
			cycle;
			cycle;
			break;
		case 0x30: // bmi rel
			t = read(pc++);
			cycle;
			if( F.b.N == 1 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0x34: // and dp, x
			t = read(pc++)+X;
			cycle;
			A &= read(DP+t);
			setnz(A);
			cycle;
			cycle;
			break;
		case 0x35: // and abs, x
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += X;
			A &= read(t16);
			setnz(A);
			cycle;
			cycle;
			break;
		case 0x36: // and abs, y
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += Y;
			A &= read(t16);
			setnz(A);
			cycle;
			cycle;
			break;
		case 0x37: // and (dp), y
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			A &= read(t16+Y);
			setnz(A);
			cycle;
			cycle;			
			break;
		case 0x38: // and dp, imm
			opc = read(pc++);
			cycle;
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			write(DP+t, t16&opc);
			setnz(t16&opc);
			cycle;
			break;
		case 0x39: // and (x), (y)
			t = read(DP+X);
			cycle;
			t16 = read(DP+Y);
			cycle;
			write(DP+X, t&t16);
			setnz(t&t16);
			cycle;
			cycle;		
			break;
		case 0x3A: // incw dp
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			t16 += 1;
			F.b.N = t16>>15;
			F.b.Z = (t16==0 ? 1:0);
			cycle;
			write(DP+t, t16);
			cycle;
			write(DP+((t+1)&0xff), t16>>8);
			cycle;
			break;
		case 0x3B:{ // rol dp, x
			t16 = (read(pc++)+X)&0xff;
			cycle;
			t = read(DP+t16);
			cycle;
			u8 oldc = F.b.C;
			F.b.C = t>>7;
			t = (t<<1)|oldc;
			setnz(t);
			write(DP+t16, t);
			cycle;
			cycle;
			}break;
		case 0x3C: // rol a
			t = F.b.C;
			F.b.C = A>>7;
			A = (A<<1)|t;
			setnz(A);
			cycle;
			break;
		case 0x3D: // inc x
			X += 1;
			setnz(X);
			cycle;
			break;
		case 0x3E: // cpx dp
			t = read(pc++);
			cycle;
			cmp(X, read(DP+t));
			cycle;
			break;
		case 0x3F: // call abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			push(pc>>8);
			cycle;
			push(pc);
			cycle;
			pc = t16;
			cycle; cycle; cycle;
			break;
		case 0x40: // setp
			F.b.P = 1;
			DP = 0x100;
			cycle;
			break;
		case 0x44: // eor dp
			t = read(pc++);
			cycle;
			A ^= read(DP+t);
			setnz(A);
			cycle;
			break;
		case 0x45: // eor abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			A ^= read(t16);
			setnz(A);
			cycle;
			break;			
		case 0x46: // eor a, (x)
			t = X;
			cycle;
			A ^= read(DP+t);
			setnz(A);
			cycle;
			break;
		case 0x47: // eor (dp, x)
			t = read(pc++)+X;
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			A ^= read(t16);
			setnz(A);
			cycle;
			cycle;
			break;
		case 0x48: // eor imm
			A ^= read(pc++);
			setnz(A);
			cycle;
			break;
		case 0x49: // eor dp, dp
			t = read(pc++);
			cycle;
			t = read(DP+t);
			cycle;
			opc = read(pc++);
			cycle;
			t16 = read(DP+opc);
			cycle;
			write(DP+opc, t^t16);
			setnz(t^t16);
			cycle;
			break;
		case 0x4A: // AND1 abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16&0x1fff);
			F.b.C &= ((t&BIT(t16>>13)) ? 1:0);
			cycle;
			break;
		case 0x4B: // lsr dp
			t16 = read(pc++);
			cycle;
			t = read(DP+(t16&0xff));
			cycle;
			F.b.C = t&1;
			t >>= 1;
			setnz(t);
			write(DP+(t16&0xff), t);
			cycle;		
			break;
		case 0x4C: // lsr abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16);
			cycle;
			F.b.C = t&1;
			t >>= 1;
			setnz(t);
			write(t16, t);
			cycle;
			break;
		case 0x4D: // phx
			cycle;
			cycle;
			push(X);
			cycle;
			break;
		case 0x4E: // TCLR1 !abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16);
			cycle;
			setnz(A-t);
			cycle;
			write(t16, t&~A);
			cycle;
			break;
		case 0x4F: // pcall up
			t16 = 0xff00 | read(pc++);
			cycle;
			push(pc>>8);
			cycle;
			push(pc);
			cycle;
			pc = t16;
			cycle;
			cycle;
			break;
		case 0x50: // bvc rel
			t = read(pc++);
			cycle;
			if( F.b.V == 0 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0x54: // eor dp, x
			t = read(pc++)+X;
			cycle;
			A ^= read(DP+t);
			setnz(A);
			cycle;
			cycle;
			break;
		case 0x55: // eor abs, x
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			A ^= read(t16+X);
			setnz(A);
			cycle;
			break;
		case 0x56: // eor abs, y
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			A ^= read(t16+Y);
			setnz(A);
			cycle;
			break;
		case 0x57: // eor (dp), y
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			A ^= read(t16+Y);
			setnz(A);
			cycle;
			cycle;			
			break;
		case 0x58: // eor dp, imm
			opc = read(pc++);
			cycle;
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			write(DP+t, t16^opc);
			setnz(t16^opc);
			cycle;
			break;
		case 0x59: // eor (x), (y)
			t = read(DP+X);
			cycle;
			t16 = read(DP+Y);
			cycle;
			write(DP+X, t^t16);
			setnz(t^t16);
			cycle;
			cycle;		
			break;
		case 0x5A:{ // cmpw dp, ya
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			u16 YA = (Y<<8)|A;
			F.b.C = YA >= t16;
			F.b.Z = t16 == YA;
			F.b.N = (YA - t16)>>15;
			cycle;
			}break;
		case 0x5B: // lsr dp, x
			t = read(pc++);
			cycle;
			t += X;
			cycle;
			t16 = read(DP+t);
			cycle;
			F.b.C = t16&1;
			t16 >>= 1;
			setnz(t16);
			write(DP+t, t16);
			cycle;
			break;
		case 0x5C: // lsr a
			F.b.C = A&1;
			A >>= 1;
			setnz(A);
			cycle;
			break;
		case 0x5D: // tax
			X = A;
			setnz(X);
			cycle;
			break;
		case 0x5E: // cpy abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			cmp(Y, read(t16));
			cycle;
			break;
		case 0x5F: // jmp abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc)<<8;
			cycle;
			pc = t16;
			break;
		case 0x60: // clc
			F.b.C = 0;
			cycle;
			break;
		case 0x64: // cmp a, dp
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			cmp(A, t16);
			cycle;
			break;
		case 0x65: // cmp abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			cmp(A, read(t16));
			cycle;
			break;
		case 0x66: // cmp (x)
			t = X;
			t16 = read(DP+t);
			cycle;
			cmp(A, t16);
			cycle;
			cycle;
			break;
		case 0x67: // cmp (dp, x)
			t = read(pc++)+X;
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			cmp(A, read(t16));
			cycle;
			cycle;
			break;
		case 0x68: // cmp a, imm
			cmp(A, read(pc++));
			cycle;
			break;
		case 0x69: // cmp dp, dp
			t = read(pc++);
			cycle;
			t = read(DP+t);
			cycle;
			opc = read(pc++);
			cycle;
			t16 = read(DP+opc);
			cycle;
			cmp(t16, t);
			cycle;
			break;
		case 0x6A: // AND1 /abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16&0x1fff);
			F.b.C &= ((t&BIT(t16>>13)) ? 0:1);
			cycle;
			break;
		case 0x6B:{ // ror dp
			t16 = read(pc++);
			cycle;
			t = read(DP+t16);
			cycle;
			u8 oldc = F.b.C;
			F.b.C = t&1;
			t = (t>>1)|(oldc<<7);
			setnz(t);
			write(DP+t16, t);
			cycle;
			}break;
		case 0x6C:{ // ror abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16);
			cycle;
			u8 oldc = F.b.C;
			F.b.C = t&1;
			t = (t>>1)|(oldc<<7);
			setnz(t);
			write(t16, t);
			cycle;
			}break;
		case 0x6D: // phy
			cycle;
			cycle;
			push(Y);
			cycle;
			break;
		case 0x6E: // dbnz dp, rel
			t = read(pc++);
			cycle;
			t16 = read(pc++);
			cycle;
			opc = read(DP+t);
			opc -= 1;
			cycle;
			write(DP+t, opc);
			cycle;
			if( opc != 0 )
			{
				pc += (s8)t16;
				cycle; cycle;
			}
			break;
		case 0x6F: // rts
			pc = pop();
			cycle;
			pc |= pop()<<8;
			cycle; cycle; cycle;
			break;
		case 0x70: // bvs rel
			t = read(pc++);
			cycle;
			if( F.b.V == 1 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0x74: // cmp a, dp+X
			t = read(pc++)+X;
			cycle;
			cycle;
			cmp(A, read(DP+t));
			cycle;
			break;
		case 0x75: // cmp abs, x
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += X;
			cycle;
			cmp(A, read(t16));
			cycle;
			break;
		case 0x76: // cmp abs, y
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += Y;
			cycle;
			cmp(A, read(t16));
			cycle;
			break;
		case 0x77: // cmp (dp), y
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			cmp(A, read(t16+Y));
			cycle;
			cycle;			
			break;
		case 0x78: // cmp dp, imm
			t = read(pc++);
			cycle;
			t16 = read(pc++);
			cycle;
			cycle;
			cmp(read(DP+t16), t);
			cycle;
			break;
		case 0x79: // cmp (x), (y)
			t = read(DP+X);
			cycle;
			t16 = read(DP+Y);
			cycle;
			cmp(t, t16);
			cycle;
			cycle;		
			break;
		case 0x7A:{ // addw ya, dp
			u16 YA = (Y<<8)|A;
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			u32 res = YA;
			res += t16;
			F.b.H = ((YA&0xfff)+(t16&0xfff))>0xfff;
			F.b.C = res>>16;
			F.b.V = ((res^t16)&(res^YA))>>15;
			YA = res;
			F.b.N = YA>>15;
			F.b.Z = (YA==0);
			Y = YA>>8;
			A = YA;
			cycle;			
			}break;
		case 0x7B:{ // ror dp, x
			t16 = (read(pc++)+X)&0xff;
			cycle;
			t = read(DP+t16);
			cycle;
			u8 oldc = F.b.C;
			F.b.C = t&1;
			t = (t>>1)|(oldc<<7);
			setnz(t);
			write(DP+t16, t);
			cycle;
			cycle;
			}break;
		case 0x7C: // ror a
			t = F.b.C;
			F.b.C = A&1;
			A = (A>>1)|(t<<7);
			setnz(A);
			cycle;
			break;
		case 0x7D: // txa
			A = X;
			setnz(A);
			cycle;
			break;
		case 0x7E: // cpy dp
			t = read(pc++);
			cycle;
			cmp(Y, read(DP+t));
			cycle;
			break;
		case 0x7F: // reti
			F.v = pop();
			cycle;
			pc = pop();
			cycle;
			pc |= pop()<<8;
			cycle; cycle;
			DP = (F.b.P ? 0x100:0);
			break;
		case 0x80: // sec
			F.b.C = 1;
			cycle;
			break;
		case 0x84: // ADC dp
			t = read(pc++);
			cycle;
			A = add(A, read(DP+t));
			cycle;
			break;
		case 0x85: // adc abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			A = add(A, read(t16));
			cycle;
			break;
		case 0x86: // adc (x)
			A = add(A, read(DP+X));
			cycle;
			cycle;
			break;
		case 0x87: // adc (dp, x)
			t = read(pc++)+X;
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			A = add(A, read(t16));
			cycle;
			cycle;
			break;
		case 0x88: // ADC imm
			A = add(A, read(pc++));
			cycle;
			break;
		case 0x89: // adc dp, dp
			t = read(pc++);
			cycle;
			t = read(DP+t);
			cycle;
			opc = read(pc++);
			cycle;
			t16 = read(DP+opc);
			cycle;
			write(DP+opc, add(t16, t));
			cycle;
			break;
		case 0x8A: // EOR1 abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16&0x1fff);
			F.b.C ^= ((t&BIT(t16>>13)) ? 1:0);
			cycle;
			break;
		case 0x8B: // dec dp
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16-=1;
			setnz(t16);
			write(DP+t, t16);
			cycle;
			break;
		case 0x8C: // dec abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16);
			t -= 1;
			setnz(t);
			cycle;
			write(t16, t);
			cycle;
			break;
		case 0x8D: // ldy imm
			Y = read(pc++);
			setnz(Y);
			cycle;
			break;
		case 0x8E: // plp
			cycle;
			cycle;
			F.v = pop();
			DP = (F.b.P ? 0x100 : 0);
			cycle;
			break;
		case 0x8F: // mov dp, imm
			opc = read(pc++);
			cycle;
			t = read(pc++);
			cycle;
			cycle;
			write(DP+t, opc);
			cycle;
			break;
		case 0x90: // bcc rel
			t = read(pc++);
			cycle;
			if( F.b.C == 0 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0x94: // ADC dp, x
			t = read(pc++)+X;
			cycle;
			A = add(A, read(DP+t));
			cycle;
			cycle;
			break;
		case 0x95: // adc abs, x
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += X;
			cycle;
			A = add(A, read(t16));
			cycle;
			break;
		case 0x96: // ADC abs, Y
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += Y;
			cycle;
			A = add(A, read(t16));
			cycle;
			break;
		case 0x97: // ADC (dp),y
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			cycle;
			A = add(A, read(t16+Y));
			cycle;
			break;
		case 0x98: // adc dp, imm
			opc = read(pc++);
			cycle;
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			write(DP+t, add(t16, opc));
			cycle;
			break;
		case 0x99: // adc (x), (y)
			t = read(DP+X);
			cycle;
			t16 = read(DP+Y);
			cycle;
			write(DP+X, add(t, t16));
			cycle;
			cycle;		
			break;
		case 0x9A:{ // subw ya, dp
			u16 YA = (Y<<8)|A;
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			t16 ^= 0xffff;
			cycle;
			u32 res = YA;
			res += t16;
			res += 1;
			F.b.C = res>>16;
			F.b.V = ((res^t16)&(res^YA))>>15;
			F.b.H = ((YA&0xfff)+(t16&0xfff))>0xfff;
			YA = res;
			F.b.Z = (YA==0);
			F.b.N = YA>>15;
			Y = YA>>8;
			A = YA;
			cycle;			
			}break;
		case 0x9B: // dec dp, x
			t = read(pc++)+X;
			cycle;
			cycle;
			t16 = read(DP+t);
			t16 -= 1;
			setnz(t16);
			cycle;
			write(DP+t, t16);
			cycle;
			break;
		case 0x9C: // dec a
			A -= 1;
			setnz(A);
			cycle;
			break;
		case 0x9D: // tsx
			X = S;
			setnz(X);
			cycle;
			break;
		case 0x9E:{ // div ya, x //from https://problemkaputt.de/fullsnes.htm#snesunpredictablethings
			u32 tmp = (Y<<8)|A;
			for(u32 i = 1; i < 10; ++i)
			{
				tmp=tmp*2;
				if( tmp & 0x20000) { tmp ^= 0x20001; }
				if( tmp >=(X*0x200) ) { tmp ^= 1; }
				if( tmp & 1 ) {  tmp=(tmp-(X*0x200)) & 0x1FFFF; }			
			}
			F.b.H = (X&15)<=(Y&15);
			A = tmp;
			F.b.V = tmp>>8;
			Y = (tmp/0x200);
			setnz(A);
			cycle; cycle; cycle; cycle;
			cycle; cycle; cycle; cycle;
			cycle; cycle; cycle; cycle;
			cycle; cycle; cycle;
			}break; // ^ 12 cycles!
		case 0x9F: // xcn a
			cycle; cycle;
			A = (A<<4)|(A>>4);
			setnz(A);
			cycle; cycle;
			break;
		case 0xA0: // cli
			F.b.I = 1;
			cycle;
			cycle;
			break;
		case 0xA4: // sbc dp
			t = read(pc++);
			cycle;
			A = add(A, 0xff^read(DP+t));
			cycle;
			break;
		case 0xA5: // sbc abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			A = add(A, read(t16)^0xff);
			cycle;
			break;
		case 0xA6: // sbc (x)
			A = add(A, read(DP+X)^0xff);
			cycle;
			cycle;
			break;
		case 0xA7: // sbc (dp, x)
			t = read(pc++)+X;
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			A = add(A, read(t16)^0xff);
			cycle;
			cycle;
			break;
		case 0xA8: // sbc imm
			A = add(A, read(pc++)^0xff);
			cycle;
			break;
		case 0xA9: // sbc dp, dp
			t = read(pc++);
			cycle;
			t = read(DP+t);
			cycle;
			opc = read(pc++);
			cycle;
			t16 = read(DP+opc);
			cycle;
			write(DP+opc, add(t16, t^0xff));
			cycle;
			break;
		case 0xAA: // MOV1 abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16&0x1fff);
			F.b.C = ((t&BIT(t16>>13)) ? 1:0);
			cycle;
			break;
		case 0xAB: // inc dp
			t16 = read(pc++);
			cycle;
			t = read(DP+t16);
			cycle;
			t += 1;
			setnz(t);
			write(DP+t16, t);
			cycle;
			break;
		case 0xAC: // inc abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16);
			cycle;
			t += 1;
			setnz(t);
			write(t16, t);
			cycle;
			break;
		case 0xAD: // cpy imm
			cmp(Y, read(pc++));
			cycle;
			break;
		case 0xAE: // pla
			cycle;
			cycle;
			A = pop();
			cycle;
			break;
		case 0xAF: // mov (x)+, a
			cycle;
			write(DP+X, A);
			X += 1;
			cycle;
			cycle;
			break;
		case 0xB0: // bcs rel
			t = read(pc++);
			cycle;
			if( F.b.C == 1 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0xB4: // sbc dp, x
			t = read(pc++)+X;
			cycle;
			A = add(A, read(DP+t)^0xff);
			cycle;
			cycle;
			break;
		case 0xB5: // sbc abs,x
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			cycle;
			A = add(A, read(t16+X)^0xff);
			cycle;
			break;
		case 0xB6: // sbc abs, y
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += Y;
			cycle;
			A = add(A, read(t16)^0xff);
			cycle;
			break;
		case 0xB7: // sbc (dp), y
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			A = add(A, read(t16+Y)^0xff);
			cycle;
			cycle;			
			break;
		case 0xB8: // sbc dp, imm
			opc = read(pc++);
			cycle;
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			write(DP+t, add(t16, opc^0xff));
			cycle;
			break;
		case 0xB9: // sbc (x), (y)
			t = read(DP+X);
			cycle;
			t16 = read(DP+Y);
			cycle;
			write(DP+X, add(t, t16^0xff));
			cycle;
			cycle;		
			break;
		case 0xBA: // movw ya, dp
			t = read(pc++);
			cycle;
			cycle;
			A = read(DP+t);
			cycle;
			Y = read(DP+((t+1)&0xff));
			F.b.Z = (Y|A)==0;
			F.b.N = Y>>7;
			cycle;
			break;
		case 0xBB: // inc dp+X
			t = read(pc++)+X;
			cycle;
			t16 = read(DP+t);
			t16 += 1;
			setnz(t16);
			cycle;
			cycle;
			write(DP+t, t16);
			cycle;
			break;
		case 0xBC: // inc a
			A += 1;
			setnz(A);
			cycle;
			break;
		case 0xBD: // txs
			S = X;
			cycle;
			break;
		case 0xBE: // das
			if( F.b.C == 0 || A > 0x99 )
			{
				F.b.C = 0;
				A -= 0x60;
			}
			if( F.b.H == 0 || (A&15)>9 )
			{
				A -= 6;
			}
			setnz(A);
			cycle;
			cycle;
			cycle;
			break;
		case 0xBF: // mov a, (x)+
			cycle;
			A = read(DP+X);
			X += 1;
			setnz(A);
			cycle;
			cycle;
			break;
		case 0xC0: // sei
			F.b.I = 0;
			cycle;
			cycle;
			break;
		case 0xC4: // sta zp
			t = read(pc++);
			cycle;
			cycle;
			write(DP+(t&0xff), A);
			cycle;
			break;
		case 0xC5: // sta abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			cycle;
			write(t16, A);
			cycle;
			break;
		case 0xC6: // mov (x), a
			cycle;
			write(DP+X, A);
			cycle;
			cycle;
			break;
		case 0xC7: // sta (dp, x)
			t = read(pc++)+X;
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			//read(t16);
			cycle;
			write(t16, A);
			cycle;
			cycle;
			break;
		case 0xC8: // cpx imm
			cmp(X, read(pc++));
			cycle;
			break;
		case 0xC9: // stx abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			cycle;
			write(t16, X);
			cycle;
			break;
		case 0xCA: // MOV1 abs, C
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16&0x1fff);
			t &= ~BIT(t16>>13);
			if( F.b.C ) t |= BIT(t16>>13);
			cycle;
			cycle;
			write(t16&0x1fff, t);
			cycle;
			break;
		case 0xCB: // sty db
			t = read(pc++);
			cycle;
			cycle;
			write(DP+t, Y);
			cycle;
			break;
		case 0xCC: // sty abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			cycle;
			write(t16, Y);
			cycle;
			break;
		case 0xCD: // ldx imm
			X = read(pc++);
			setnz(X);
			cycle;
			break;
		case 0xCE: // plx
			cycle;
			cycle;
			X = pop();
			cycle;
			break;
		case 0xCF: // mul ya
			t16 = Y;
			t16 *= A;
			Y = t16>>8;
			A = t16;
			setnz(Y);
			cycle; cycle; cycle; cycle;
			cycle; cycle; cycle; cycle;
			break;
		case 0xD0: // bne rel
			t = read(pc++);
			cycle;
			if( F.b.Z == 0 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0xD4: // sta dp,x
			t = read(pc++)+X;
			cycle;
			cycle;
			cycle;
			write(DP+t, A);
			cycle;
			break;
		case 0xD5: // sta abs, x
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			read(t16); //todo: dummy read where?
			cycle;
			t16+=X;
			cycle;
			write(t16, A);
			cycle;
			break;
		case 0xD6: // sta abs, y
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			read(t16); //todo: dummy read where?
			cycle;
			t16+=Y;
			cycle;
			write(t16, A);
			cycle;
			break;
		case 0xD7: // sta (dp),y
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			cycle;
			cycle;
			write(t16+Y, A);
			cycle;
			break;
		case 0xD8: // stx dp
			t = read(pc++);
			cycle;
			cycle;
			write(DP+t, X);
			cycle;
			break;
		case 0xD9: // stx dp, y
			t = read(pc++)+Y;
			cycle;
			write(DP+t, X);
			cycle;
			cycle;
			cycle;
			break;
		case 0xDA: // movw dp, ya
			t = read(pc++);
			cycle;
			write(DP+(t&0xff), A);
			cycle;
			write(DP+((t+1)&0xff), Y);
			cycle;
			break;
		case 0xDB: // sty db, X
			t = read(pc++);
			cycle;
			t += X;
			cycle;
			//read(t);
			cycle;
			write(DP+t, Y);
			cycle;
			break;
		case 0xDC: // dec y
			Y -= 1;
			setnz(Y);
			cycle;
			break;
		case 0xDD: // tya
			A = Y;
			setnz(A);
			cycle;
			break;
		case 0xDE: // CBNE dp+X, rel
			t16 = read(pc++);
			cycle;
			t = read(pc++);
			cycle;
			cycle;
			cycle;
			t16 = read(DP+((t16+X)&0xff));
			cycle;
			if( t16 != A )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0xDF: // daa
			if( F.b.C == 1 || A > 0x99 )
			{
				F.b.C = 1;
				A += 0x60;
			}
			if( F.b.H == 1 || (A&15)>9 )
			{
				A += 6;
			}
			setnz(A);
			cycle;
			cycle;
			cycle;
			break;
		case 0xE0: // clv
			F.b.H = F.b.V = 0;
			cycle;
			break;
		case 0xE4: // lda dp
			t = read(pc++);
			cycle;
			A = read(DP+(t&0xff));
			setnz(A);
			cycle;
			break;
		case 0xE5: // lda abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			A = read(t16);
			setnz(A);
			cycle;
			break;
		case 0xE6: // lda (x)
			t = X;
			cycle;
			A = read(DP+t);
			setnz(A);
			cycle;
			break;
		case 0xE7: // lda (dp, x)
			t = read(pc++)+X;
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			cycle;
			A = read(t16);
			setnz(A);
			cycle;
			break;
		case 0xE8: // lda imm
			A = read(pc++);
			setnz(A);
			cycle;
			break;		
		case 0xE9: // ldx abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			X = read(t16);
			setnz(X);
			cycle;
			break;		
		case 0xEA: // NOT1 abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t = read(t16&0x1fff);
			t ^= BIT(t16>>13);
			cycle;
			write(t16&0x1fff, t);
			cycle;
			break;
		case 0xEB: // ldy dp
			t = read(pc++);
			cycle;
			Y = read(DP+t);
			setnz(Y);
			cycle;
			break;
		case 0xEC: // ldy abs
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			Y = read(t16);
			setnz(Y);
			cycle;
			break;
		case 0xED: // notc
			F.b.C ^= 1;
			cycle;
			cycle;
			break;
		case 0xEE: // ply
			cycle;
			cycle;
			Y = pop();
			cycle;
			break;
		case 0xEF: // wai
			std::println("SPC700: WAI unimpl");
			cycle;
			cycle;
			break;
		case 0xF0: // beq rel
			t = read(pc++);
			cycle;
			if( F.b.Z == 1 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0xF4: // lda dp, X
			t = read(pc++);
			cycle;
			t += X;
			cycle;
			A = read(DP+(t&0xff));
			setnz(A);
			cycle;
			break;
		case 0xF5: // lda abs, x
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += X;
			cycle;
			A = read(t16);
			setnz(A);
			cycle;
			break;
		case 0xF6: // lda abs, y
			t16 = read(pc++);
			cycle;
			t16 |= read(pc++)<<8;
			cycle;
			t16 += Y;
			cycle;
			A = read(t16);
			setnz(A);
			cycle;
			break;
		case 0xF7: // lda (dp),y
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			t16 |= read(DP+((t+1)&0xff))<<8;
			cycle;
			//read(t16) // dummy read where?
			cycle;
			A = read(t16+Y);
			setnz(A);
			cycle;
			break;
		case 0xF8: // LDX dp
			t = read(pc++);
			cycle;
			X = read(DP+t);
			setnz(X);
			cycle;
			break;
		case 0xF9: // ldx dp, y
			t = read(pc++)+Y;
			cycle;
			X = read(DP+t);
			setnz(X);
			cycle;
			cycle;
			break;
		case 0xFA: // mov dp, dp
			t = read(pc++);
			cycle;
			t = read(DP+t);
			cycle;
			opc = read(pc++);
			cycle;
			write(DP+opc, t);
			cycle;
			break;
		case 0xFB: // ldy dp,x
			t = read(pc++)+X;
			cycle;
			cycle;
			Y = read(DP+t);
			setnz(Y);
			cycle;
			break;
		case 0xFC: // inc y
			Y += 1;
			setnz(Y);
			cycle;
			break;
		case 0xFD: // tay
			Y = A;
			setnz(Y);
			cycle;
			break;
		case 0xFE: // dbnz y, rel
			t = read(pc++);
			cycle;
			Y-=1;
			cycle;
			cycle;
			if( Y != 0 )
			{
				pc += (s8)t;
				cycle; cycle;
			}
			break;
		case 0xff: // stop
			std::println("SPC700: stop unimpl.");
			cycle;
			cycle;
			break;

		case 0x03:
		case 0x13:
		case 0x23:
		case 0x33:
		case 0x43:
		case 0x53:
		case 0x63:
		case 0x73:
		case 0x83:
		case 0x93:
		case 0xA3:
		case 0xB3:
		case 0xC3:
		case 0xD3:
		case 0xE3:
		case 0xF3: //bbc/bbs dp, rel
			t = read(pc++);
			cycle;
			t16 = read(pc++);
			cycle; 
			t = read(DP+t);
			cycle; cycle;
			if( ((opc&BIT(4)) && !(t&BIT((opc>>5)))) 
			|| (!(opc&BIT(4)) && (t&BIT((opc>>5)))) )
			{
				pc += (s8)t16;
				cycle; cycle;
			}
			break;
		case 0x01:
		case 0x11:
		case 0x21:
		case 0x31:
		case 0x41:
		case 0x51:
		case 0x61:
		case 0x71:
		case 0x81:
		case 0x91:
		case 0xA1:
		case 0xB1:
		case 0xC1:
		case 0xD1:
		case 0xE1:
		case 0xF1: // tcall
			t16 = 0xFFDE - (opc>>4)*2;
			//std::println("Tcall${:X} vector ${:X}", opc, t16);
			cycle;
			push(pc>>8);
			cycle;
			push(pc);
			cycle;
			pc = read(t16);
			cycle; 
			pc |= read(t16+1)<<8;
			cycle; cycle; cycle;
			break;
		case 0x02:
		case 0x12:
		case 0x22:
		case 0x32:
		case 0x42:
		case 0x52:
		case 0x62:
		case 0x72:
		case 0x82:
		case 0x92:
		case 0xA2:
		case 0xB2:
		case 0xC2:
		case 0xD2:
		case 0xE2:
		case 0xF2: // set1/clr1 dp
			t = read(pc++);
			cycle;
			t16 = read(DP+t);
			cycle;
			if( opc & BIT(4) )
			{
				t16 &= ~BIT(opc>>5);
			} else {
				t16 |= BIT(opc>>5);
			}
			write(DP+t, t16);
			cycle;
			break;
		default:
			std::println("SPC-700: Unimpl opc ${:X}", opc);
			exit(1);
		}
	}
}



















